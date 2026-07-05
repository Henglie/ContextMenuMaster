// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 真实右键菜单扫描 / 写回（实现）
//
//  从注册表把各「右键场景」的 verb / handler 项读出来交给前端 1:1 还原，
//  并实现改名 / 换图标 / 启停 / 批量后缀的写回。每个写操作前自动 .reg 备份。
//  全程宽字符 Win32 API；注册表读出的宽字符串统一转 UTF-8 再放进 std::string。
//
//  Scan the registry for each right-click "scene" (verb + handler entries),
//  expose them to the frontend, and support rename / icon / enable / batch
//  suffix writes. Every write makes a .reg backup first. All Win32 calls are
//  wide-char; strings returned to the frontend are UTF-8.
// ============================================================
#include "platform/windows/menu_scanner.h"

#include <windows.h>
#include <shlwapi.h>   // SHLoadIndirectString（解析 @dll,-id 间接字符串资源）
#include <string>
#include <vector>
#include <cwctype>   // towlower

// 主要靠 CMake 链接；此处再声明一次以防万一（重复声明无害）。
#pragma comment(lib, "shlwapi.lib")

namespace cmm {

namespace {

// —— UTF-8 <-> UTF-16 小工具（系统 API 全走宽字符，路径/名称含中文也安全）——
std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                        s.data(), n, nullptr, nullptr);
    return s;
}

// JSON 字符串值转义：\ -> \\，" -> \"，以及控制字符走 \u00XX。
// 项目无 JSON 库，手拼时所有字符串字段都要先过这里。
std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    wsprintfA(buf, "\\u%04x", (unsigned)c);
                    out += buf;
                } else {
                    out += (char)c;
                }
        }
    }
    return out;
}

// —— 注册表小工具 ——

// 读某键下某值（lpValueName=nullptr 取默认值）为宽字符串。失败返回 false。
// 自动按返回的真实长度截断，并去掉尾部多余的 NUL。
bool reg_read_string(HKEY hKey, const wchar_t* valueName, std::wstring* out) {
    DWORD type = 0;
    DWORD cb = 0;
    LSTATUS st = RegQueryValueExW(hKey, valueName, nullptr, &type, nullptr, &cb);
    if (st != ERROR_SUCCESS) return false;
    if (type != REG_SZ && type != REG_EXPAND_SZ) return false;
    if (cb == 0) { *out = L""; return true; }

    std::wstring buf(cb / sizeof(wchar_t) + 1, L'\0');
    st = RegQueryValueExW(hKey, valueName, nullptr, &type,
                          reinterpret_cast<LPBYTE>(buf.data()), &cb);
    if (st != ERROR_SUCCESS) return false;
    buf.resize(cb / sizeof(wchar_t));
    while (!buf.empty() && buf.back() == L'\0') buf.pop_back();
    *out = buf;
    return true;
}

// 判断某键下是否存在某值名（用于探测 LegacyDisable）。
bool reg_value_exists(HKEY hKey, const wchar_t* valueName) {
    LSTATUS st = RegQueryValueExW(hKey, valueName, nullptr, nullptr, nullptr, nullptr);
    return st == ERROR_SUCCESS;
}

// 展开环境变量（%SystemRoot% 之类）。无变量时原样返回。
std::wstring expand_env(const std::wstring& in) {
    if (in.empty()) return in;
    DWORD need = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
    if (need == 0) return in;
    std::wstring out(need, L'\0');
    DWORD got = ExpandEnvironmentStringsW(in.c_str(), out.data(), need);
    if (got == 0) return in;
    out.resize(got > 0 ? got - 1 : 0);  // got 含终止符
    return out;
}

// 解析「间接字符串资源」：MUIVerb / verb 默认值可能是 "@some.dll,-123" 形式，
// 指向 DLL 资源里的真实文案。SHLoadIndirectString 负责按当前 UI 语言解析。
// 非 @ 形式（已是明文）原样返回。失败返回空串。
// 注意：SHLoadIndirectString 大多数情况下能自己处理 %SystemRoot% 等环境变量，
// 但实测某些路径（含混合大小写 %systemroot%）解析会失败，提前 ExpandEnvironmentStringsW
// 一次更稳。
// Resolve an "indirect string" spec like "@some.dll,-123" used by MUIVerb /
// verb default values. Returns the localized string. Pre-expand env vars
// because SHLoadIndirectString fails on some mixed-case %systemroot% forms.
std::wstring resolve_indirect_string(const std::wstring& spec) {
    if (spec.empty()) return L"";
    if (spec[0] != L'@') return spec;  // 不是间接资源，本身即明文

    // 先把 @ 后面的路径段里环境变量展开（@ 本身保留作为间接资源前缀标识）。
    std::wstring expanded = spec;
    if (expanded.find(L'%') != std::wstring::npos) {
        std::wstring rest = expanded.substr(1);  // 去掉 '@'
        DWORD need = ExpandEnvironmentStringsW(rest.c_str(), nullptr, 0);
        if (need > 0) {
            std::wstring out(need, L'\0');
            DWORD got = ExpandEnvironmentStringsW(rest.c_str(), out.data(), need);
            if (got > 0) {
                out.resize(got - 1);
                expanded = L"@" + out;
            }
        }
    }

    wchar_t buf[512] = {0};
    HRESULT hr = SHLoadIndirectString(expanded.c_str(), buf, ARRAYSIZE(buf), nullptr);
    if (SUCCEEDED(hr) && buf[0] != L'\0') return std::wstring(buf);
    return L"";
}

// Humanize 一个 verb 键名作为兜底显示：
// - 去掉前导 '.'（隐藏 verb，如 .SpotlightNextImage）
// - 驼峰拆词：大写字母前插空格（如 DesktopSlideshow -> Desktop Slideshow）
// - 单纯 ASCII 才动；含中文/非 ASCII 字符就原样返回（避免破坏已经是明文的项）。
// Humanize a verb key as a last-resort display name when MUIVerb resolution fails.
std::wstring humanize_verb_name(const std::wstring& verb) {
    if (verb.empty()) return verb;
    // 含非 ASCII 直接返回，认为已是给人看的文字。
    for (wchar_t c : verb) {
        if (c > 127) return verb;
    }
    size_t start = 0;
    while (start < verb.size() && verb[start] == L'.') ++start;
    if (start >= verb.size()) return verb;

    std::wstring out;
    out.reserve(verb.size() + 4);
    for (size_t i = start; i < verb.size(); ++i) {
        wchar_t c = verb[i];
        if (i > start && c >= L'A' && c <= L'Z') {
            wchar_t prev = verb[i - 1];
            bool prevLower = (prev >= L'a' && prev <= L'z');
            bool prevDigit = (prev >= L'0' && prev <= L'9');
            if (prevLower || prevDigit) out += L' ';
        }
        out += c;
    }
    return out;
}

// 清洗最终显示名 label（语言无关）。注册表里的真实 label 常带噪音：
//   1) 前后空白 / 全角空格（如 " FileSyncEx" / "    sgshellext2"，键名本身带空格）；
//   2) 内嵌零宽字符 U+200B / U+FEFF（如回收站的 "清空​​回收站" 中间藏着零宽空格）；
//   3) COM ProgID 默认描述的 " Class" 噪音后缀（"AccExt Class" → "AccExt"）。
// 这些都与语言无关，归后端清；本地化友好名（裸键名如 find/removeproperties）交前端三语映射。
// Language-neutral label cleanup: trim whitespace, drop zero-width chars,
// strip the COM " Class" suffix. Localized friendly names are handled front-end.
std::wstring clean_label(std::wstring s) {
    // 去内嵌零宽字符（U+200B 零宽空格、U+FEFF 零宽不换行空格/BOM）。
    std::wstring t;
    t.reserve(s.size());
    for (wchar_t c : s) {
        if (c == 0x200B || c == 0xFEFF) continue;
        t += c;
    }
    s.swap(t);
    // trim 前后空白（含全角空格 U+3000）。
    const wchar_t* ws = L" \t\r\n　";
    size_t b = s.find_first_not_of(ws);
    if (b == std::wstring::npos) return L"";
    size_t e = s.find_last_not_of(ws);
    s = s.substr(b, e - b + 1);
    // 去尾部 " Class"（COM 类名噪音后缀），去后仍非空才采纳。
    static const std::wstring kClassSuffix = L" Class";
    if (s.size() > kClassSuffix.size() &&
        s.compare(s.size() - kClassSuffix.size(), kClassSuffix.size(), kClassSuffix) == 0) {
        std::wstring trimmed = s.substr(0, s.size() - kClassSuffix.size());
        size_t te = trimmed.find_last_not_of(ws);
        if (te != std::wstring::npos) s = trimmed.substr(0, te + 1);
    }
    return s;
}

// 转小写（宽字符）。is_system_handler_by_clsid 与 is_system_verb 都依赖它，
// 故放最前；原本在 is_system_verb 之后定义，新代码先用到就提前到这里。
std::wstring to_lower(std::wstring s) {
    for (wchar_t& c : s) c = (wchar_t)towlower(c);
    return s;
}

// 已知系统内置 verb（小写比较）。命中给前端更强的二次确认。
bool is_system_verb(const std::wstring& verbLower) {
    static const wchar_t* kSystem[] = {
        L"open", L"opennewwindow", L"opennewprocess", L"opennewtab",
        L"pintohome", L"pintostartscreen", L"cmd", L"powershell",
        L"runas", L"print", L"printto", L"find", L"explore",
        L"properties", L"delete", L"rename", L"copy", L"cut", L"paste",
    };
    for (const wchar_t* s : kSystem) {
        if (verbLower == s) return true;
    }
    return false;
}

// 判定一个 handler 的 CLSID 是否为系统内置 COM 扩展。
// 系统扩展的 InprocServer32 DLL 通常位于 %SystemRoot%\System32\ 或 %SystemRoot%\
// 下；装在 Program Files / 用户目录下的第三方扩展（7-Zip、IDE 的 shell 集成等）
// 不算系统项，避免误标 SYS 让用户做不必要的二次确认。
//
// 路径比较用展开后的绝对路径前缀匹配，大小写不敏感。
// Classify a handler CLSID as system vs third-party by inspecting its
// InprocServer32 DLL path. System DLLs live under %SystemRoot%\System32 or
// %SystemRoot%; third-party (7-Zip, IDE integrations, ...) live under
// Program Files / user dirs and should NOT be flagged system.
bool is_system_handler_by_clsid(const std::wstring& clsid) {
    if (clsid.empty()) return false;

    // 优先 HKCR\CLSID\<clsid>\InprocServer32 的默认值（DLL 路径）。
    std::wstring dllPath;
    std::wstring inprocKey = L"CLSID\\" + clsid + L"\\InprocServer32";
    HKEY hInp = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, inprocKey.c_str(), 0,
                      KEY_QUERY_VALUE, &hInp) == ERROR_SUCCESS) {
        reg_read_string(hInp, nullptr, &dllPath);
        RegCloseKey(hInp);
    }
    // 兜底：HKCR\Wow64Node\CLSID\<clsid>\InprocServer32（32 位 COM 在 64 位系统）。
    if (dllPath.empty()) {
        std::wstring wowKey = L"Wow64Node\\CLSID\\" + clsid + L"\\InprocServer32";
        HKEY hWow = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wowKey.c_str(), 0,
                          KEY_QUERY_VALUE, &hWow) == ERROR_SUCCESS) {
            reg_read_string(hWow, nullptr, &dllPath);
            RegCloseKey(hWow);
        }
    }
    if (dllPath.empty()) return false;  // 找不到 DLL，保守不算系统

    std::wstring expanded = expand_env(dllPath);
    // 转小写做前缀比较。
    std::wstring lower = to_lower(expanded);

    // 取 %SystemRoot% 展开后的路径，小写化。
    wchar_t sysRootBuf[MAX_PATH] = {0};
    DWORD got = GetEnvironmentVariableW(L"SystemRoot", sysRootBuf, MAX_PATH);
    if (got == 0 || got >= MAX_PATH) return false;
    std::wstring sysRoot = to_lower(std::wstring(sysRootBuf));

    // 必须形如 <sysRoot>\... 才算系统。System32 / SysWOW64 / System32\... 都覆盖。
    // 比较时给 sysRoot 末尾加 '\'，避免 "\Windows" 误命中 "\WindowsOld" 之类。
    if (sysRoot.empty()) return false;
    if (sysRoot.back() != L'\\') sysRoot += L'\\';
    if (lower.size() <= sysRoot.size()) return false;
    return lower.compare(0, sysRoot.size(), sysRoot) == 0;
}

}  // namespace

namespace {

// 场景 -> shell 根（verb 来源，相对 HKCR）。
const wchar_t* scene_shell_root(MenuScene scene) {
    switch (scene) {
        case MenuScene::DesktopBackground:   return L"DesktopBackground\\Shell";
        case MenuScene::AllFiles:            return L"*\\shell";
        case MenuScene::Directory:           return L"Directory\\shell";
        case MenuScene::DirectoryBackground: return L"Directory\\Background\\shell";
        case MenuScene::Drive:               return L"Drive\\shell";
        case MenuScene::RecycleBin:
            return L"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\shell";
        case MenuScene::AllFileSystemObjects: return L"AllFilesystemObjects\\shell";
        case MenuScene::Computer:
            return L"CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\shell";
        case MenuScene::Library:             return L"LibraryFolder\\shell";
    }
    return L"";
}

// 场景 -> shellex\ContextMenuHandlers 根（handler 来源）。回收站无 handler，返回空。
const wchar_t* scene_handler_root(MenuScene scene) {
    switch (scene) {
        case MenuScene::DesktopBackground:
            return L"DesktopBackground\\ShellEx\\ContextMenuHandlers";
        case MenuScene::AllFiles:            return L"*\\shellex\\ContextMenuHandlers";
        case MenuScene::Directory:           return L"Directory\\shellex\\ContextMenuHandlers";
        case MenuScene::DirectoryBackground:
            return L"Directory\\Background\\shellex\\ContextMenuHandlers";
        case MenuScene::Drive:               return L"Drive\\shellex\\ContextMenuHandlers";
        case MenuScene::RecycleBin:          return L"";
        case MenuScene::AllFileSystemObjects:
            return L"AllFilesystemObjects\\shellex\\ContextMenuHandlers";
        case MenuScene::Computer:            return L"";
        case MenuScene::Library:             return L"LibraryFolder\\shellex\\ContextMenuHandlers";
    }
    return L"";
}

// 场景 -> 备份导出根（含 shell 与 shellex 的共同父键，相对 HKCR）。
// 导出父键能把 verb 与 handler 一起完整备份，还原最忠实。
const wchar_t* scene_backup_root(MenuScene scene) {
    switch (scene) {
        case MenuScene::DesktopBackground:   return L"DesktopBackground";
        case MenuScene::AllFiles:            return L"*";
        case MenuScene::Directory:           return L"Directory";
        case MenuScene::DirectoryBackground: return L"Directory\\Background";
        case MenuScene::Drive:               return L"Drive";
        case MenuScene::RecycleBin:
            return L"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}";
        case MenuScene::AllFileSystemObjects: return L"AllFilesystemObjects";
        case MenuScene::Computer:
            return L"CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
        case MenuScene::Library:             return L"LibraryFolder";
    }
    return L"";
}

// 枚举某打开键下的全部子键名。
std::vector<std::wstring> enum_subkeys(HKEY hKey) {
    std::vector<std::wstring> out;
    DWORD i = 0;
    wchar_t name[256];
    for (;;) {
        DWORD cch = ARRAYSIZE(name);
        LSTATUS st = RegEnumKeyExW(hKey, i, name, &cch, nullptr, nullptr,
                                   nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st != ERROR_SUCCESS) break;
        out.emplace_back(name, cch);
        ++i;
    }
    return out;
}

// 给 HKEY 根起个可读名字，拼 regPath 用。
const wchar_t* hroot_name(HKEY hRoot) {
    if (hRoot == HKEY_CLASSES_ROOT)  return L"HKEY_CLASSES_ROOT";
    if (hRoot == HKEY_LOCAL_MACHINE) return L"HKEY_LOCAL_MACHINE";
    if (hRoot == HKEY_CURRENT_USER)  return L"HKEY_CURRENT_USER";
    return L"HKEY_CLASSES_ROOT";
}

// 前置声明：递归读 verb（折叠菜单的子项要回头调它）。
MenuItem read_verb_generic(HKEY hRoot, const std::wstring& verbKeyPath,
                           const std::wstring& verbName, int depth);

// 展开级联子菜单（折叠菜单）。两种注册方式：
//   1) ExtendedSubCommandsKey：值是相对 HKCR 的键路径，其下 shell\ 子键放子 verb
//      （现代第三方软件常用，如夸克AI、WinRAR 折叠组）。
//   2) SubCommands：值是分号分隔的 token，每个指向
//      HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\CommandStore\shell\<token>
//      （系统/微软风格）。SubCommands 值即便为空串，存在本身就标记此项为折叠飞出。
// hSub 是父 verb 已打开的键（KEY_QUERY_VALUE 即可）。命中任一即返回 true。
// Expand a cascading (flyout) submenu via ExtendedSubCommandsKey or SubCommands.
bool expand_submenu(HKEY hSub, int depth, std::vector<MenuItem>* outChildren) {
    if (depth > 4) return false;  // 递归深度护栏，防环

    std::wstring ext;
    bool hasExt = reg_read_string(hSub, L"ExtendedSubCommandsKey", &ext) && !ext.empty();
    bool hasSubc = reg_value_exists(hSub, L"SubCommands");
    if (!hasExt && !hasSubc) return false;

    // 法一：ExtendedSubCommandsKey -> HKCR\<ext>\shell 下枚举子 verb。
    if (hasExt) {
        std::wstring childShell = ext + L"\\shell";
        HKEY hChildShell = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, childShell.c_str(), 0,
                          KEY_ENUMERATE_SUB_KEYS, &hChildShell) == ERROR_SUCCESS) {
            std::vector<std::wstring> kids = enum_subkeys(hChildShell);
            RegCloseKey(hChildShell);
            for (const std::wstring& kid : kids) {
                outChildren->push_back(read_verb_generic(
                    HKEY_CLASSES_ROOT, childShell + L"\\" + kid, kid, depth + 1));
            }
        }
    }

    // 法二：SubCommands 列出的 token，去 CommandStore 查（ExtendedSubCommandsKey
    // 没展开出东西时才走这条；两者同时存在时优先 ExtendedSubCommandsKey）。
    if (outChildren->empty() && hasSubc) {
        std::wstring subcVal;
        reg_read_string(hSub, L"SubCommands", &subcVal);
        std::vector<std::wstring> tokens;
        std::wstring cur;
        for (wchar_t c : subcVal) {
            if (c == L';') { if (!cur.empty()) tokens.push_back(cur); cur.clear(); }
            else cur += c;
        }
        if (!cur.empty()) tokens.push_back(cur);

        const std::wstring storeBase =
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell\\";
        for (const std::wstring& tok : tokens) {
            std::wstring t = tok;
            size_t a = t.find_first_not_of(L" \t");
            size_t b = t.find_last_not_of(L" \t");
            if (a == std::wstring::npos) continue;
            t = t.substr(a, b - a + 1);
            outChildren->push_back(read_verb_generic(
                HKEY_LOCAL_MACHINE, storeBase + t, t, depth + 1));
        }
    }
    return true;
}

// 通用 verb 读取核心：从 hRoot\verbKeyPath 读出一个菜单项（label/icon/enabled/command），
// 并探测/展开折叠子菜单。verbName 用于 humanize 兜底与 id。
// 默认产出「只读」项（canRename/Icon/Toggle 全 false）——折叠子项写回不在本期范围
// （CommandStore 在 HKLM 需提权、ExtendedSubCommandsKey 路径多样）；顶层项由
// read_verb_item 包装后再放开可写标记。
MenuItem read_verb_generic(HKEY hRoot, const std::wstring& verbKeyPath,
                           const std::wstring& verbName, int depth) {
    MenuItem item;
    item.kind = MenuItemKind::Verb;
    item.id = wide_to_utf8(verbName);
    item.canRename = false;
    item.canChangeIcon = false;
    item.canToggle = false;
    item.system = is_system_verb(to_lower(verbName));
    // 本工具自建项：verb 键名以 "cmm_" 开头。前端据此给「编辑/删除自建项」入口，
    // 与第三方/系统 verb 区分（自建项可编辑执行命令，第三方项只能改名/图标/启停）。
    item.custom = verbName.rfind(L"cmm_", 0) == 0;
    item.regPath = wide_to_utf8(std::wstring(hroot_name(hRoot)) + L"\\" + verbKeyPath);

    HKEY hSub = nullptr;
    if (RegOpenKeyExW(hRoot, verbKeyPath.c_str(), 0, KEY_QUERY_VALUE, &hSub)
        == ERROR_SUCCESS) {
        // label 解析顺序：
        //   1) MUIVerb（明确的本地化资源指针，最权威）
        //   2) 默认值（也可能是 @dll,-id 间接资源，要 resolve）
        //   3) 子键名 humanize（DesktopSlideshow -> "Desktop Slideshow"）
        // 每步若拿到的是 @-资源就走 SHLoadIndirectString；解析失败立刻进下一步，
        // 而不是把 @%systemroot%\...,-10 这种原始 spec 当成 label 显示出去。
        std::wstring label;
        std::wstring mui;
        if (reg_read_string(hSub, L"MUIVerb", &mui) && !mui.empty()) {
            std::wstring resolved = resolve_indirect_string(mui);
            if (!resolved.empty() && resolved[0] != L'@') label = resolved;
            else if (mui[0] != L'@') label = mui;
        }
        if (label.empty()) {
            std::wstring def;
            if (reg_read_string(hSub, nullptr, &def) && !def.empty()) {
                std::wstring resolved = resolve_indirect_string(def);
                if (!resolved.empty() && resolved[0] != L'@') label = resolved;
                else if (def[0] != L'@') label = def;
            }
        }
        if (label.empty()) label = humanize_verb_name(verbName);
        item.label = wide_to_utf8(clean_label(label));

        // 图标源：Icon 值，含环境变量则展开。
        std::wstring icon;
        if (reg_read_string(hSub, L"Icon", &icon) && !icon.empty()) {
            item.iconSource = wide_to_utf8(expand_env(icon));
        }

        // 启用判据：存在 LegacyDisable 值即为停用。
        item.enabled = !reg_value_exists(hSub, L"LegacyDisable");

        // 折叠子菜单展开。
        std::vector<MenuItem> kids;
        if (expand_submenu(hSub, depth, &kids)) {
            item.hasSubmenu = true;
            item.children = std::move(kids);
        }
        RegCloseKey(hSub);
    } else {
        item.label = wide_to_utf8(clean_label(humanize_verb_name(verbName)));
    }

    // 命令：子键\command 的默认值。
    std::wstring cmdPath = verbKeyPath + L"\\command";
    HKEY hCmd = nullptr;
    if (RegOpenKeyExW(hRoot, cmdPath.c_str(), 0, KEY_QUERY_VALUE, &hCmd)
        == ERROR_SUCCESS) {
        std::wstring cmd;
        if (reg_read_string(hCmd, nullptr, &cmd)) {
            item.command = wide_to_utf8(cmd);
        }
        RegCloseKey(hCmd);
    }
    return item;
}

// 解析顶层 verb 子键，填充一个 MenuItem。shellRootW 形如 L"Directory\\shell"。
// 顶层项可写（改名/图标/启停），故在通用核心基础上放开可写标记。折叠子菜单的
// 父项本身也是 HKCR 下的明文 verb，其 label 仍可改名（改默认值即可），故同样放开。
MenuItem read_verb_item(MenuScene scene, const std::wstring& shellRootW,
                        const std::wstring& verb) {
    (void)scene;  // 保留签名兼容调用方；当前实现不需要 scene。
    MenuItem item = read_verb_generic(HKEY_CLASSES_ROOT, shellRootW + L"\\" + verb,
                                      verb, 0);
    item.canRename = true;
    item.canChangeIcon = true;
    item.canToggle = true;
    return item;
}

}  // namespace

const char* scene_id(MenuScene scene) {
    switch (scene) {
        case MenuScene::DesktopBackground:   return "desktop";
        case MenuScene::AllFiles:            return "file";
        case MenuScene::Directory:           return "folder";
        case MenuScene::DirectoryBackground: return "folder_bg";
        case MenuScene::Drive:               return "drive";
        case MenuScene::RecycleBin:          return "recycle";
        case MenuScene::AllFileSystemObjects: return "allfsobjects";
        case MenuScene::Computer:            return "computer";
        case MenuScene::Library:             return "library";
    }
    return "";
}

bool scene_from_id(const std::string& id, MenuScene* out) {
    if (!out) return false;
    if (id == "desktop")      { *out = MenuScene::DesktopBackground;   return true; }
    if (id == "file")         { *out = MenuScene::AllFiles;            return true; }
    if (id == "folder")       { *out = MenuScene::Directory;           return true; }
    if (id == "folder_bg")    { *out = MenuScene::DirectoryBackground; return true; }
    if (id == "drive")        { *out = MenuScene::Drive;               return true; }
    if (id == "recycle")      { *out = MenuScene::RecycleBin;          return true; }
    if (id == "allfsobjects") { *out = MenuScene::AllFileSystemObjects; return true; }
    if (id == "computer")     { *out = MenuScene::Computer;            return true; }
    if (id == "library")      { *out = MenuScene::Library;             return true; }
    return false;
}

std::vector<MenuItem> scan_scene(MenuScene scene) {
    std::vector<MenuItem> items;

    // —— verb 项：枚举 shell 根下的子键 ——
    std::wstring shellRoot = scene_shell_root(scene);
    if (!shellRoot.empty()) {
        HKEY hShell = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, shellRoot.c_str(), 0,
                          KEY_ENUMERATE_SUB_KEYS, &hShell) == ERROR_SUCCESS) {
            std::vector<std::wstring> verbs = enum_subkeys(hShell);
            RegCloseKey(hShell);
            for (const std::wstring& verb : verbs) {
                items.push_back(read_verb_item(scene, shellRoot, verb));
            }
        }
    }

    // —— handler 项：枚举 shellex\ContextMenuHandlers 根下的子键 ——
    // Handler 的 label 解析三档兜底（之前直接用键名导致 CLSID 串、英文驼峰大量出现）：
    //   1) 若键名/默认值是 CLSID，则去 HKCR\CLSID\{...}\ 取默认值（明文）+
    //      LocalizedString（@dll,-id 间接资源）；
    //   2) 若键名是普通可读字符串（如 "7-Zip"），原样用；
    //   3) 兜底 humanize（DesktopSlideshow -> Desktop Slideshow）。
    // Handler label resolution: CLSID → HKCR\CLSID\{...}\(default + LocalizedString)
    // → readable key name → humanized key name. Never expose raw CLSID/PascalCase.
    // —— handler 项：枚举 shellex\ContextMenuHandlers（启用）与 -ContextMenuHandlers
    //    （停用镜像）两个根。停用的 handler 被搬进 '-' 前缀根，若不扫这里，用户一旦
    //    停用某 handler 它就从列表消失、再也开不回来。故两个根都扫，disabled 的标
    //    enabled=false，前端开关据此还原。
    std::wstring handlerRoot = scene_handler_root(scene);
    if (!handlerRoot.empty()) {
        // 推出停用镜像根：ContextMenuHandlers -> -ContextMenuHandlers。
        std::wstring disHandlerRoot = handlerRoot;
        size_t dpos = disHandlerRoot.rfind(L"ContextMenuHandlers");
        if (dpos != std::wstring::npos) disHandlerRoot.insert(dpos, L"-");

        // 读一个 handler 子键为 MenuItem。enabled 标记该 handler 是否处于启用位置。
        auto read_handler = [&](const std::wstring& rootPath, const std::wstring& name,
                                bool enabled) -> MenuItem {
            MenuItem item;
            item.kind = MenuItemKind::Handler;
            item.id = "handler:" + wide_to_utf8(name);
            item.enabled = enabled;
            item.canRename = false;
            item.canChangeIcon = false;
            item.canToggle = true;
            item.regPath = "HKEY_CLASSES_ROOT\\" + wide_to_utf8(rootPath + L"\\" + name);

            std::wstring clsid;
            std::wstring handlerPath = rootPath + L"\\" + name;
            HKEY hHand = nullptr;
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, handlerPath.c_str(), 0,
                              KEY_QUERY_VALUE, &hHand) == ERROR_SUCCESS) {
                reg_read_string(hHand, nullptr, &clsid);
                RegCloseKey(hHand);
            }

            std::wstring candidate = (name.size() >= 2 && name[0] == L'{') ? name
                                   : (clsid.size() >= 2 && clsid[0] == L'{') ? clsid
                                   : L"";
            item.system = is_system_handler_by_clsid(candidate);
            item.clsid = wide_to_utf8(candidate);

            std::wstring label;
            if (!candidate.empty()) {
                std::wstring clsidKey = L"CLSID\\" + candidate;
                HKEY hClsid = nullptr;
                if (RegOpenKeyExW(HKEY_CLASSES_ROOT, clsidKey.c_str(), 0,
                                  KEY_QUERY_VALUE, &hClsid) == ERROR_SUCCESS) {
                    std::wstring loc;
                    if (reg_read_string(hClsid, L"LocalizedString", &loc) && !loc.empty()) {
                        std::wstring r = resolve_indirect_string(loc);
                        if (!r.empty() && r[0] != L'@') label = r;
                    }
                    if (label.empty()) {
                        std::wstring def;
                        if (reg_read_string(hClsid, nullptr, &def) && !def.empty()) {
                            if (def[0] != L'@') label = def;
                            else {
                                std::wstring r = resolve_indirect_string(def);
                                if (!r.empty() && r[0] != L'@') label = r;
                            }
                        }
                    }
                    RegCloseKey(hClsid);
                }
            }
            if (label.empty() && (name.empty() || name[0] != L'{')) label = name;
            if (label.empty() || (label.size() >= 2 && label[0] == L'{')) {
                label = humanize_verb_name(name);
                if (!label.empty() && label[0] == L'{') label = L"";
            }
            if (label.empty()) label = L"COM Extension";
            item.label = wide_to_utf8(clean_label(label));

            // handler 图标源：注册表 handler 项本身无 Icon 值，故从 CLSID 找——
            // 优先 HKCR\CLSID\{..}\DefaultIcon 默认值（明文图标 spec），
            // 退回 InprocServer32 的 DLL 路径（用其第 0 个图标）。前端拿去 extract_icon
            // 提取真实 PNG，取不到再回落几何字符。修「handler 全是 ▸ 占位」。
            if (!candidate.empty()) {
                std::wstring iconSpec;
                std::wstring diKey = L"CLSID\\" + candidate + L"\\DefaultIcon";
                HKEY hDi = nullptr;
                if (RegOpenKeyExW(HKEY_CLASSES_ROOT, diKey.c_str(), 0,
                                  KEY_QUERY_VALUE, &hDi) == ERROR_SUCCESS) {
                    std::wstring di;
                    if (reg_read_string(hDi, nullptr, &di) && !di.empty() && di != L"%1")
                        iconSpec = expand_env(di);
                    RegCloseKey(hDi);
                }
                if (iconSpec.empty()) {
                    std::wstring inpKey = L"CLSID\\" + candidate + L"\\InprocServer32";
                    HKEY hInp = nullptr;
                    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, inpKey.c_str(), 0,
                                      KEY_QUERY_VALUE, &hInp) == ERROR_SUCCESS) {
                        std::wstring dll;
                        if (reg_read_string(hInp, nullptr, &dll) && !dll.empty())
                            iconSpec = expand_env(dll) + L",0";
                        RegCloseKey(hInp);
                    }
                }
                if (!iconSpec.empty()) item.iconSource = wide_to_utf8(iconSpec);
            }
            return item;
        };

        // 扫某根下全部 handler 子键。
        auto scan_handler_root = [&](const std::wstring& rootPath, bool enabled) {
            HKEY hHandlers = nullptr;
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT, rootPath.c_str(), 0,
                              KEY_ENUMERATE_SUB_KEYS, &hHandlers) != ERROR_SUCCESS) return;
            std::vector<std::wstring> names = enum_subkeys(hHandlers);
            RegCloseKey(hHandlers);
            for (const std::wstring& name : names) {
                items.push_back(read_handler(rootPath, name, enabled));
            }
        };

        scan_handler_root(handlerRoot, true);
        if (dpos != std::wstring::npos) scan_handler_root(disHandlerRoot, false);
    }

    return items;
}

namespace {

// 把单个 MenuItem 序列化为 JSON 对象，递归带上 children（折叠子菜单）。
// Serialize a MenuItem to a JSON object, recursing into children (submenu).
void item_to_json(const MenuItem& it, std::string& out) {
    out += "{\"id\":\"";        out += json_escape(it.id);         out += "\"";
    out += ",\"kind\":\"";
    out += (it.kind == MenuItemKind::Verb) ? "verb" : "handler";   out += "\"";
    out += ",\"label\":\"";     out += json_escape(it.label);      out += "\"";
    out += ",\"iconSource\":\"";out += json_escape(it.iconSource); out += "\"";
    out += ",\"command\":\"";   out += json_escape(it.command);    out += "\"";
    out += ",\"regPath\":\"";   out += json_escape(it.regPath);    out += "\"";
    out += ",\"enabled\":";     out += it.enabled ? "true" : "false";
    out += ",\"system\":";      out += it.system ? "true" : "false";
    out += ",\"canRename\":";   out += it.canRename ? "true" : "false";
    out += ",\"canChangeIcon\":";out += it.canChangeIcon ? "true" : "false";
    out += ",\"canToggle\":";   out += it.canToggle ? "true" : "false";
    out += ",\"custom\":";      out += it.custom ? "true" : "false";
    out += ",\"clsid\":\"";     out += json_escape(it.clsid);      out += "\"";
    out += ",\"hasSubmenu\":";  out += it.hasSubmenu ? "true" : "false";
    out += ",\"children\":[";
    for (size_t i = 0; i < it.children.size(); ++i) {
        if (i) out += ",";
        item_to_json(it.children[i], out);
    }
    out += "]";
    out += "}";
}

}  // namespace

std::string scan_scene_json(MenuScene scene) {
    std::vector<MenuItem> items = scan_scene(scene);
    std::string out = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) out += ",";
        item_to_json(items[i], out);
    }
    out += "]";
    return out;
}

namespace {

// 简易 JSON 结果：{"ok":bool,"key":"..."}。
std::string result_json(bool ok, const char* key) {
    std::string s = "{\"ok\":";
    s += ok ? "true" : "false";
    s += ",\"key\":\"";
    s += key;
    s += "\"}";
    return s;
}

// 取本地备份目录 %LOCALAPPDATA%\ContextMenuMaster\backups\，逐级创建。失败返回空。
std::wstring ensure_backup_dir() {
    wchar_t base[MAX_PATH] = {0};
    DWORD got = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    if (got == 0 || got >= MAX_PATH) return L"";

    std::wstring dir = base;
    dir += L"\\ContextMenuMaster";
    CreateDirectoryW(dir.c_str(), nullptr);  // 已存在则忽略
    dir += L"\\backups";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

// 生成时间戳 yyyyMMdd_HHmmss（本地时间）。
std::wstring timestamp_now() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[32];
    wsprintfW(buf, L"%04u%02u%02u_%02u%02u%02u",
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

// 用 reg.exe export 把场景根键导出为 .reg。成功时 outPathUtf8 填备份文件路径。
// reg.exe 接受 HKCR 前缀（HKEY_CLASSES_ROOT 的简写）。
bool do_backup(MenuScene scene, std::string* outPathUtf8) {
    std::wstring dir = ensure_backup_dir();
    if (dir.empty()) return false;

    std::wstring file = dir + L"\\" + utf8_to_wide(scene_id(scene)) +
                        L"_" + timestamp_now() + L".reg";

    std::wstring regRoot = L"HKCR\\";
    regRoot += scene_backup_root(scene);

    // reg.exe 路径（系统目录下，避免 PATH 依赖）。
    wchar_t sysDir[MAX_PATH] = {0};
    if (GetSystemDirectoryW(sysDir, MAX_PATH) == 0) return false;
    std::wstring regExe = std::wstring(sysDir) + L"\\reg.exe";

    // 命令行：reg.exe export "HKCR\<root>" "<file>" /y
    std::wstring cmd = L"\"" + regExe + L"\" export \"" + regRoot +
                       L"\" \"" + file + L"\" /y";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};
    std::wstring mutableCmd = cmd;  // CreateProcessW 需可写缓冲

    BOOL ok = CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (!ok) return false;

    // 最多等 30 秒。* 场景（AllFiles）导出 HKCR\* 整树较慢，5 秒会未导完就超时
    // → exitCode 拿到 259(STILL_ACTIVE) 被当失败误报 backup_failed 挡住写操作。
    // 用 WAIT_TIMEOUT 判「还在跑」，只有真超 30 秒才放弃。
    DWORD waitRc = WaitForSingleObject(pi.hProcess, 30000);
    if (waitRc == WAIT_TIMEOUT) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return false;
    }
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (exitCode != 0) return false;

    if (outPathUtf8) *outPathUtf8 = wide_to_utf8(file);
    return true;
}

// 把写操作传入的 id 解析为 verb 子键名（去掉可能的前缀）。handler 的 id 带
// "handler:" 前缀，verb 不带。isHandler 返回该 id 是否是 handler。
std::wstring id_to_verb(const std::string& id, bool* isHandler) {
    const std::string prefix = "handler:";
    if (id.compare(0, prefix.size(), prefix) == 0) {
        if (isHandler) *isHandler = true;
        return utf8_to_wide(id.substr(prefix.size()));
    }
    if (isHandler) *isHandler = false;
    return utf8_to_wide(id);
}

// ============================================================
//  写真实 hive（提权版）
//
//  经典菜单项分散在两处：HKLM\Software\Classes（装机级，所有用户，第三方软件
//  多写这里）与 HKCU\Software\Classes（用户级）。工具以管理员身份运行（见
//  app.manifest），可直接写真实位置。
//
//  旧版走「纯 HKCU 覆盖层」有个致命缺陷：像 123云盘 / ToDesk 这类第三方项，
//  其 LegacyDisable 停用标记写在 HKLM，纯 HKCU 删不掉 → 合并视图仍禁用 →
//  用户「怎么点都开不了」。提权后直接改真实 hive，才能真正启停 / 改名 / 换图标。
// ============================================================

// 打开某 hive（HKLM/HKCU）下的 verb 键。create=true 时按需创建。失败返回 false。
bool open_verb_in_hive(HKEY hive, MenuScene scene, const std::wstring& verb,
                       REGSAM access, bool create, HKEY* outKey) {
    std::wstring shellRoot = scene_shell_root(scene);
    if (shellRoot.empty()) return false;
    // HKLM 用 "SOFTWARE\Classes"、HKCU 用 "Software\Classes"，大小写不敏感，统一写法。
    std::wstring subPath = L"Software\\Classes\\" + shellRoot + L"\\" + verb;
    if (create) {
        DWORD disp = 0;
        return RegCreateKeyExW(hive, subPath.c_str(), 0, nullptr,
                               REG_OPTION_NON_VOLATILE, access, nullptr,
                               outKey, &disp) == ERROR_SUCCESS;
    }
    return RegOpenKeyExW(hive, subPath.c_str(), 0, access, outKey) == ERROR_SUCCESS;
}

// verb 在各 hive 的存在情况（决定往哪写、删哪里）。
struct VerbHives { bool hklm = false; bool hkcu = false; };
VerbHives locate_verb(MenuScene scene, const std::wstring& verb) {
    VerbHives v;
    HKEY h = nullptr;
    if (open_verb_in_hive(HKEY_LOCAL_MACHINE, scene, verb, KEY_QUERY_VALUE, false, &h)) {
        v.hklm = true; RegCloseKey(h);
    }
    if (open_verb_in_hive(HKEY_CURRENT_USER, scene, verb, KEY_QUERY_VALUE, false, &h)) {
        v.hkcu = true; RegCloseKey(h);
    }
    return v;
}

// 决定「写入主 hive」：verb 若在 HKLM 存在就写 HKLM（覆盖装机级，对所有用户生效，
// 与项真实所在一致）；否则写 HKCU（纯用户项，或全新建项落用户级更安全）。
HKEY primary_write_hive(MenuScene scene, const std::wstring& verb) {
    VerbHives v = locate_verb(scene, verb);
    return v.hklm ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
}

// 打开某 verb 子键只读（读当前值）。HKCU 覆盖优先，再 HKLM，最后 HKCR 合并视图兜底。
bool open_verb_key(MenuScene scene, const std::wstring& verb, REGSAM access,
                   HKEY* outKey) {
    if (open_verb_in_hive(HKEY_CURRENT_USER, scene, verb, access, false, outKey))
        return true;
    if (open_verb_in_hive(HKEY_LOCAL_MACHINE, scene, verb, access, false, outKey))
        return true;
    std::wstring shellRoot = scene_shell_root(scene);
    if (shellRoot.empty()) return false;
    std::wstring subPath = shellRoot + L"\\" + verb;
    return RegOpenKeyExW(HKEY_CLASSES_ROOT, subPath.c_str(), 0, access, outKey)
           == ERROR_SUCCESS;
}

// 读某 verb 的当前显示名（默认值 -> MUIVerb 解析 -> 子键名），用于批量后缀。
std::wstring current_verb_label(MenuScene scene, const std::wstring& verb) {
    HKEY hSub = nullptr;
    std::wstring label = verb;
    if (open_verb_key(scene, verb, KEY_QUERY_VALUE, &hSub)) {
        std::wstring def;
        if (reg_read_string(hSub, nullptr, &def) && !def.empty()) {
            label = def;
        } else {
            std::wstring mui;
            if (reg_read_string(hSub, L"MUIVerb", &mui) && !mui.empty()) {
                std::wstring resolved = resolve_indirect_string(mui);
                label = resolved.empty() ? mui : resolved;
            }
        }
    }
    return label;
}

// 把某 verb 子键的默认值（REG_SZ）设为给定宽字符串，写真实主 hive。
// 关键：若该项原本有 MUIVerb，Windows 显示时 MUIVerb 优先于默认值 → 只改默认值
// 改名不生效。故写默认值的同时删掉 MUIVerb（改名后以我们写的默认值为准）。
bool set_verb_default(MenuScene scene, const std::wstring& verb,
                      const std::wstring& value) {
    HKEY hive = primary_write_hive(scene, verb);
    HKEY hSub = nullptr;
    if (!open_verb_in_hive(hive, scene, verb, KEY_SET_VALUE, true, &hSub)) return false;
    // cbData 含终止符。
    LSTATUS st = RegSetValueExW(
        hSub, nullptr, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        (DWORD)((value.size() + 1) * sizeof(wchar_t)));
    // 删 MUIVerb（不存在也无所谓），让默认值成为唯一显示来源。
    RegDeleteValueW(hSub, L"MUIVerb");
    RegCloseKey(hSub);
    return st == ERROR_SUCCESS;
}

}  // namespace

std::string rename_item(MenuScene scene, const std::string& id,
                        const std::string& newLabel) {
    bool isHandler = false;
    std::wstring verb = id_to_verb(id, &isHandler);
    // 只允许 verb 改名；handler 真实文字由 COM 决定，改不了。
    if (isHandler || verb.empty()) {
        return result_json(false, "cannot_rename");
    }

    // 定位失败（子键不存在）也按不可改名处理。
    HKEY hCheck = nullptr;
    if (!open_verb_key(scene, verb, KEY_QUERY_VALUE, &hCheck)) {
        return result_json(false, "cannot_rename");
    }
    RegCloseKey(hCheck);

    // 写前自动备份。
    std::string backupPath;
    if (!do_backup(scene, &backupPath)) {
        return result_json(false, "backup_failed");
    }

    // 设默认值即覆盖显示（即便原本走的是 MUIVerb，默认值优先）。
    if (!set_verb_default(scene, verb, utf8_to_wide(newLabel))) {
        return result_json(false, "registry_failed");
    }
    return result_json(true, "ok");
}

// —— handler 启停（shellex\ContextMenuHandlers）——
// COM 扩展 handler 的启停不是改值，而是把 handler 子键在两个位置间搬家：
//   启用位置： shellex\ContextMenuHandlers\<name>
//   停用位置： shellex\-ContextMenuHandlers\<name>   （前缀 '-'，shell 不枚举这里）
// 这是 Windows 惯例的 disabled mirror（参考项目 ContextMenuMgr 亦同）。整键复制后
// 删源，保留 CLSID 值不丢。搬 HKLM 需管理员（已提权）。找不到就退回 HKCU 侧尝试。
// scene_handler_root 给的是启用根，把最后一段 "ContextMenuHandlers" 换成
// "-ContextMenuHandlers" 即停用根。
namespace {

// 递归把 src 键树整体复制到 dst（含所有值与子键）。dst 按需创建。
bool copy_key_tree(HKEY srcRoot, const std::wstring& srcPath,
                   HKEY dstRoot, const std::wstring& dstPath) {
    HKEY hSrc = nullptr;
    if (RegOpenKeyExW(srcRoot, srcPath.c_str(), 0,
                      KEY_READ, &hSrc) != ERROR_SUCCESS) return false;
    HKEY hDst = nullptr;
    DWORD disp = 0;
    if (RegCreateKeyExW(dstRoot, dstPath.c_str(), 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                        &hDst, &disp) != ERROR_SUCCESS) {
        RegCloseKey(hSrc);
        return false;
    }
    // 复制值。
    for (DWORD i = 0; ; ++i) {
        wchar_t name[512]; DWORD cchName = ARRAYSIZE(name);
        DWORD type = 0, cbData = 0;
        LSTATUS st = RegEnumValueW(hSrc, i, name, &cchName, nullptr,
                                   &type, nullptr, &cbData);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st != ERROR_SUCCESS) break;
        std::vector<BYTE> data(cbData ? cbData : 1);
        cchName = ARRAYSIZE(name);
        if (RegEnumValueW(hSrc, i, name, &cchName, nullptr, &type,
                          data.data(), &cbData) == ERROR_SUCCESS) {
            RegSetValueExW(hDst, name, 0, type, data.data(), cbData);
        }
    }
    // 递归复制子键。
    for (DWORD i = 0; ; ++i) {
        wchar_t sub[512]; DWORD cch = ARRAYSIZE(sub);
        LSTATUS st = RegEnumKeyExW(hSrc, i, sub, &cch, nullptr,
                                   nullptr, nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st != ERROR_SUCCESS) break;
        copy_key_tree(srcRoot, srcPath + L"\\" + sub,
                      dstRoot, dstPath + L"\\" + sub);
    }
    RegCloseKey(hSrc);
    RegCloseKey(hDst);
    return true;
}

// 判某 hive 下 handler 是否已处于目标位置（enable→在启用根，disable→在停用根）。
// 用于「已在目标态」幂等判定：move_handler 因源不存在返回 false 时，区分「本就到位」
// 与「真找不到」，前者视为成功不报假失败。
bool handler_at_target(HKEY hive, MenuScene scene, const std::wstring& rawName,
                       bool enable) {
    std::wstring enRoot = scene_handler_root(scene);
    if (enRoot.empty()) return false;
    std::wstring disRoot = enRoot;
    size_t pos = disRoot.rfind(L"ContextMenuHandlers");
    if (pos == std::wstring::npos) return false;
    disRoot.insert(pos, L"-");
    // 目标根：启用态在启用根，停用态在停用根。
    std::wstring targetPath = L"Software\\Classes\\" +
                              (enable ? enRoot : disRoot) + L"\\" + rawName;
    HKEY h = nullptr;
    if (RegOpenKeyExW(hive, targetPath.c_str(), 0, KEY_READ, &h) == ERROR_SUCCESS) {
        RegCloseKey(h);
        return true;
    }
    return false;
}

// 把 handler（原始子键名 rawName，可能带前导空格）在启用/停用位置间搬家。
// 在某 hive 下：启用根 = scene_handler_root，停用根 = 同级 "-ContextMenuHandlers"。
// 返回是否成功搬动（源不存在算失败，让调用方换 hive 再试）。
bool move_handler(HKEY hive, MenuScene scene, const std::wstring& rawName, bool enable) {
    std::wstring enRoot = scene_handler_root(scene);   // 如 *\shellex\ContextMenuHandlers
    if (enRoot.empty()) return false;
    // 推出停用根：把结尾 "ContextMenuHandlers" 换成 "-ContextMenuHandlers"。
    std::wstring disRoot = enRoot;
    size_t pos = disRoot.rfind(L"ContextMenuHandlers");
    if (pos == std::wstring::npos) return false;
    disRoot.insert(pos, L"-");

    std::wstring base = L"Software\\Classes\\";
    std::wstring srcRootPath = base + (enable ? disRoot : enRoot) + L"\\" + rawName;
    std::wstring dstRootPath = base + (enable ? enRoot : disRoot) + L"\\" + rawName;

    // 源必须存在。
    HKEY hProbe = nullptr;
    if (RegOpenKeyExW(hive, srcRootPath.c_str(), 0, KEY_READ, &hProbe) != ERROR_SUCCESS)
        return false;
    RegCloseKey(hProbe);

    if (!copy_key_tree(hive, srcRootPath, hive, dstRootPath)) return false;
    // 复制成功后删源整树。
    RegDeleteTreeW(hive, srcRootPath.c_str());
    return true;
}

}  // namespace

std::string set_item_enabled(MenuScene scene, const std::string& id, bool enabled) {
    bool isHandler = false;
    std::wstring verb = id_to_verb(id, &isHandler);
    if (verb.empty()) return result_json(false, "not_found");

    std::string backupPath;
    if (!do_backup(scene, &backupPath)) {
        return result_json(false, "backup_failed");
    }

    // —— handler：在启用/停用两个 mirror 位置间搬家 ——
    if (isHandler) {
        // verb 变量此处实为 handler 原始子键名（含可能的前导空格，与 scan 时一致）。
        // 关键：不能用 || 短路。同一 handler 可能 HKLM、HKCU 两处都注册，scan 走
        // HKCR 合并视图只显示一个。若停用只搬了 HKLM，HKCU 那份仍在启用根 → 合并
        // 视图仍启用（看着「停用没生效」）。故两个 hive 都尝试搬，任一成功即成功。
        bool movedHklm = move_handler(HKEY_LOCAL_MACHINE, scene, verb, enabled);
        bool movedHkcu = move_handler(HKEY_CURRENT_USER, scene, verb, enabled);
        if (!movedHklm && !movedHkcu) {
            // 没搬动任何一份：可能是「已在目标态」（重复点同方向 / 前端竞态），
            // 也可能是「真找不到」。已在目标态视为幂等成功，不报假失败。
            if (handler_at_target(HKEY_LOCAL_MACHINE, scene, verb, enabled) ||
                handler_at_target(HKEY_CURRENT_USER, scene, verb, enabled)) {
                return result_json(true, "ok");
            }
            return result_json(false, "registry_failed");
        }
        return result_json(true, "ok");
    }

    // —— verb：删/加 LegacyDisable。关键修复「怎么点都开不了」——
    // 停用标记可能写在 HKLM（第三方装机项，如 123云盘/ToDesk）也可能 HKCU。
    // 启用时必须把两个 hive 的 LegacyDisable 都删掉，否则合并视图里只要有一处
    // 停用标记项就仍然禁用。提权后 HKLM 可写。
    VerbHives loc = locate_verb(scene, verb);
    if (!loc.hklm && !loc.hkcu) {
        // HKCR 合并视图能看到但两 hive 都定位不到（罕见），按未找到。
        HKEY hCheck = nullptr;
        if (!open_verb_key(scene, verb, KEY_QUERY_VALUE, &hCheck))
            return result_json(false, "not_found");
        RegCloseKey(hCheck);
    }

    bool anyOk = false;
    if (enabled) {
        // 启用：两个 hive 都删 LegacyDisable（存在才删）。
        for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
            HKEY hSub = nullptr;
            if (open_verb_in_hive(hive, scene, verb, KEY_SET_VALUE, false, &hSub)) {
                LSTATUS st = RegDeleteValueW(hSub, L"LegacyDisable");
                RegCloseKey(hSub);
                if (st == ERROR_SUCCESS || st == ERROR_FILE_NOT_FOUND) anyOk = true;
            }
        }
    } else {
        // 停用：项在哪个 hive 存在就往哪个 hive 写 LegacyDisable。
        // 只写主 hive 不够——若 HKCU 覆盖层也有同名 verb 且无停用标记，合并视图
        // HKCU 优先，仍显示启用（看着「停用没生效」）。与启用分支「两 hive 都删」
        // 对称：两 hive 都写，谁在合并视图优先都被停用。至少一处存在才算成功。
        for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
            HKEY hSub = nullptr;
            // 只对「该 hive 里真实存在的 verb」写，不用 create——避免在没有该 verb
            // 的 hive 凭空建空壳键。
            if (open_verb_in_hive(hive, scene, verb, KEY_SET_VALUE, false, &hSub)) {
                const wchar_t empty[] = L"";
                LSTATUS st = RegSetValueExW(hSub, L"LegacyDisable", 0, REG_SZ,
                                            reinterpret_cast<const BYTE*>(empty),
                                            sizeof(wchar_t));
                RegCloseKey(hSub);
                if (st == ERROR_SUCCESS) anyOk = true;
            }
        }
        // 两 hive 都不存在该 verb（仅 HKCR 合并视图可见的罕见情形）：退回主 hive
        // 建键写标记，保证停用有落点。
        if (!anyOk) {
            HKEY hive = primary_write_hive(scene, verb);
            HKEY hSub = nullptr;
            if (open_verb_in_hive(hive, scene, verb, KEY_SET_VALUE, true, &hSub)) {
                const wchar_t empty[] = L"";
                LSTATUS st = RegSetValueExW(hSub, L"LegacyDisable", 0, REG_SZ,
                                            reinterpret_cast<const BYTE*>(empty),
                                            sizeof(wchar_t));
                RegCloseKey(hSub);
                anyOk = (st == ERROR_SUCCESS);
            }
        }
    }
    if (!anyOk) return result_json(false, "registry_failed");
    return result_json(true, "ok");
}

std::string set_item_icon(MenuScene scene, const std::string& id,
                          const std::string& iconSource) {
    bool isHandler = false;
    std::wstring verb = id_to_verb(id, &isHandler);
    if (isHandler || verb.empty()) return result_json(false, "not_found");

    HKEY hCheck = nullptr;
    if (!open_verb_key(scene, verb, KEY_QUERY_VALUE, &hCheck)) {
        return result_json(false, "not_found");
    }
    RegCloseKey(hCheck);

    std::string backupPath;
    if (!do_backup(scene, &backupPath)) {
        return result_json(false, "backup_failed");
    }

    // 写真实主 hive（项在 HKLM 就写 HKLM，否则 HKCU），已提权可写。
    HKEY hive = primary_write_hive(scene, verb);
    HKEY hSub = nullptr;
    if (!open_verb_in_hive(hive, scene, verb, KEY_SET_VALUE, true, &hSub)) {
        return result_json(false, "registry_failed");
    }
    std::wstring icon = utf8_to_wide(iconSource);
    LSTATUS st = RegSetValueExW(
        hSub, L"Icon", 0, REG_SZ,
        reinterpret_cast<const BYTE*>(icon.c_str()),
        (DWORD)((icon.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hSub);
    if (st != ERROR_SUCCESS) return result_json(false, "registry_failed");
    return result_json(true, "ok");
}

std::string batch_append_suffix(MenuScene scene, const std::string& idsCsv,
                                const std::string& suffix) {
    // 写前自动备份（整批共一次，足够回滚）。
    std::string backupPath;
    if (!do_backup(scene, &backupPath)) {
        return result_json(false, "backup_failed");
    }

    std::wstring wsuffix = utf8_to_wide(suffix);
    int count = 0;

    // 解析逗号分隔的 id 列表。
    size_t start = 0;
    while (start <= idsCsv.size()) {
        size_t comma = idsCsv.find(',', start);
        std::string id = (comma == std::string::npos)
                             ? idsCsv.substr(start)
                             : idsCsv.substr(start, comma - start);
        // 去首尾空白。
        size_t a = id.find_first_not_of(" \t");
        size_t b = id.find_last_not_of(" \t");
        if (a != std::string::npos) id = id.substr(a, b - a + 1);
        else id.clear();

        if (!id.empty()) {
            bool isHandler = false;
            std::wstring verb = id_to_verb(id, &isHandler);
            if (!isHandler && !verb.empty()) {
                std::wstring label = current_verb_label(scene, verb) + wsuffix;
                if (set_verb_default(scene, verb, label)) ++count;
            }
        }

        if (comma == std::string::npos) break;
        start = comma + 1;
    }

    std::string s = "{\"ok\":true,\"key\":\"batch_done\",\"count\":";
    s += std::to_string(count);
    s += "}";
    return s;
}

std::string backup_scene(MenuScene scene) {
    std::string path;
    if (!do_backup(scene, &path)) {
        return result_json(false, "backup_failed");
    }
    std::string s = "{\"ok\":true,\"key\":\"backed_up\",\"path\":\"";
    s += json_escape(path);
    s += "\"}";
    return s;
}

}  // namespace cmm
