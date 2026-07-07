// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 文件类型右键菜单（实现）
//
//  见 filetype_menu.h 的接口契约。本模块自带一份精简 helper（utf8_to_wide /
//  reg_read_string / enum_subkeys / resolve_indirect / humanize / copy_key_tree
//  等），不复用 menu_scanner.cpp 的匿名 namespace 版本——那些跨编译单元不可见，
//  且本项目既定风格是每个独立模块（sendto/win11menu/…）各自带 helper。
// ============================================================
#include "platform/windows/filetype_menu.h"

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>

#include <algorithm>
#include <map>
#include <set>

#pragma comment(lib, "shlwapi.lib")

namespace cmm {
namespace {

// ---- 基础编码 helper --------------------------------------------------------

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                        &s[0], n, nullptr, nullptr);
    return s;
}

std::string json_escape(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char buf[8];
                    wsprintfA(buf, "\\u%04x", (unsigned)(unsigned char)c);
                    o += buf;
                } else {
                    o += c;
                }
        }
    }
    return o;
}

std::string result_json(bool ok, const char* key) {
    std::string o = "{\"ok\":";
    o += ok ? "true" : "false";
    o += ",\"key\":\"";
    o += json_escape(key ? key : "");
    o += "\"}";
    return o;
}

// ---- 注册表读 helper --------------------------------------------------------

// 读某键某值为字符串。valueName 为 nullptr 读默认值。
bool reg_read_string(HKEY hKey, const wchar_t* valueName, std::wstring* out) {
    DWORD type = 0, cb = 0;
    if (RegQueryValueExW(hKey, valueName, nullptr, &type, nullptr, &cb) != ERROR_SUCCESS)
        return false;
    if (type != REG_SZ && type != REG_EXPAND_SZ) return false;
    std::wstring buf(cb / sizeof(wchar_t) + 1, L'\0');
    if (RegQueryValueExW(hKey, valueName, nullptr, &type,
                         reinterpret_cast<BYTE*>(&buf[0]), &cb) != ERROR_SUCCESS)
        return false;
    buf.resize(wcslen(buf.c_str()));
    *out = buf;
    return true;
}

// 枚举某打开键的直接子键名。
std::vector<std::wstring> enum_subkeys(HKEY hKey) {
    std::vector<std::wstring> out;
    for (DWORD i = 0; ; ++i) {
        wchar_t name[512];
        DWORD cch = ARRAYSIZE(name);
        LSTATUS st = RegEnumKeyExW(hKey, i, name, &cch, nullptr, nullptr, nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st != ERROR_SUCCESS) break;
        out.emplace_back(name, cch);
    }
    return out;
}

// 判某键是否存在（HKCR 合并视图）。
bool key_exists(HKEY root, const std::wstring& path) {
    HKEY h = nullptr;
    if (RegOpenKeyExW(root, path.c_str(), 0, KEY_READ, &h) == ERROR_SUCCESS) {
        RegCloseKey(h);
        return true;
    }
    return false;
}

// 解析间接字符串资源 "@dll,-id" → 真实本地化文案。非 @ 开头原样返回。
std::wstring resolve_indirect(const std::wstring& spec) {
    if (spec.empty() || spec[0] != L'@') return spec;
    wchar_t buf[512];
    if (SUCCEEDED(SHLoadIndirectString(spec.c_str(), buf, ARRAYSIZE(buf), nullptr)) && buf[0])
        return buf;
    return spec;
}

// 驼峰/下划线键名转人类可读（DesktopSlideshow -> Desktop Slideshow）。
std::wstring humanize(const std::wstring& s) {
    std::wstring o;
    for (size_t i = 0; i < s.size(); ++i) {
        wchar_t c = s[i];
        if (c == L'_' || c == L'.') { o += L' '; continue; }
        if (i > 0 && iswupper(c) && !iswupper(s[i - 1]) && s[i - 1] != L' ')
            o += L' ';
        o += c;
    }
    return o;
}

bool ends_with_ci(const std::wstring& s, const std::wstring& suf) {
    if (s.size() < suf.size()) return false;
    return CompareStringOrdinal(s.c_str() + (s.size() - suf.size()), (int)suf.size(),
                                suf.c_str(), (int)suf.size(), TRUE) == CSTR_EQUAL;
}

// verb 名是否本工具自建（cmm_ 前缀）。
bool is_cmm_verb(const std::wstring& verb) {
    return verb.size() > 4 &&
           CompareStringOrdinal(verb.c_str(), 4, L"cmm_", 4, TRUE) == CSTR_EQUAL;
}

// ---- 扩展名解析 -------------------------------------------------------------

// 规范化：去首尾空白、补前导点、转小写。含非法字符（\ / 空格 : *）判非法。
FtAssoc resolve_assoc_w(const std::string& extRaw) {
    FtAssoc a;
    std::wstring w = utf8_to_wide(extRaw);
    // trim
    while (!w.empty() && iswspace(w.front())) w.erase(w.begin());
    while (!w.empty() && iswspace(w.back())) w.pop_back();
    if (w.empty()) return a;
    // 补点
    if (w.front() != L'.') w = L"." + w;
    // 非法字符
    for (wchar_t c : w) {
        if (c == L'\\' || c == L'/' || c == L' ' || c == L':' ||
            c == L'*' || c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|')
            return a;
    }
    // 只有一个点也非法
    if (w.size() < 2) return a;
    std::transform(w.begin(), w.end(), w.begin(), ::towlower);

    a.valid = true;
    a.ext = wide_to_utf8(w);

    // ProgID：HKCR\.ext 默认值。
    HKEY h = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, w.c_str(), 0, KEY_QUERY_VALUE, &h) == ERROR_SUCCESS) {
        std::wstring pid;
        if (reg_read_string(h, nullptr, &pid) && !pid.empty()) a.progId = wide_to_utf8(pid);
        std::wstring pt;
        if (reg_read_string(h, L"PerceivedType", &pt) && !pt.empty())
            a.perceivedType = wide_to_utf8(pt);
        RegCloseKey(h);
    }
    return a;
}

}  // namespace

FtAssoc resolve_filetype(const std::string& extRaw) {
    return resolve_assoc_w(extRaw);
}

namespace {

// ---- 根路径推导 -------------------------------------------------------------

// rootTag 字符串。
const char* root_tag(FtRoot r) {
    switch (r) {
        case FtRoot::Ext:          return "ext";
        case FtRoot::ProgId:       return "progid";
        case FtRoot::SfaExt:       return "sfaext";
        case FtRoot::SfaPerceived: return "sfaperceived";
    }
    return "";
}

bool root_from_tag(const std::string& tag, FtRoot* out) {
    if (tag == "ext")          { *out = FtRoot::Ext;          return true; }
    if (tag == "progid")       { *out = FtRoot::ProgId;       return true; }
    if (tag == "sfaext")       { *out = FtRoot::SfaExt;       return true; }
    if (tag == "sfaperceived") { *out = FtRoot::SfaPerceived; return true; }
    return false;
}

// 某根的「基路径」（相对 Classes 根，即 HKCR 直接子路径 / Software\Classes\<此>）。
// 无对应关联（如无 ProgID）返回空串。
std::wstring root_base_path(FtRoot r, const FtAssoc& a) {
    std::wstring ext = utf8_to_wide(a.ext);
    switch (r) {
        case FtRoot::Ext:    return ext;
        case FtRoot::ProgId:
            return a.progId.empty() ? L"" : utf8_to_wide(a.progId);
        case FtRoot::SfaExt: return L"SystemFileAssociations\\" + ext;
        case FtRoot::SfaPerceived:
            return a.perceivedType.empty() ? L""
                 : L"SystemFileAssociations\\" + utf8_to_wide(a.perceivedType);
    }
    return L"";
}

// ---- 读一个 verb 项 ---------------------------------------------------------

// shellRootPath 相对 Classes（如 ".txt\shell" / "SystemFileAssociations\.txt\shell"）。
// verb 为子键名。从 HKCR 合并视图读，供显示。
FtItem read_verb(FtRoot r, const std::wstring& shellRootPath, const std::wstring& verb) {
    FtItem it;
    it.kind = "verb";
    it.rootTag = root_tag(r);
    it.id = std::string(root_tag(r)) + "|verb:" + wide_to_utf8(verb);
    it.custom = is_cmm_verb(verb);
    it.canRename = false;      // 文件类型 verb 改名本轮不做（涉全局关联，风险高）
    it.canChangeIcon = false;
    it.canToggle = true;

    std::wstring verbPath = shellRootPath + L"\\" + verb;
    it.regPath = "HKEY_CLASSES_ROOT\\" + wide_to_utf8(verbPath);

    HKEY h = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, verbPath.c_str(), 0, KEY_QUERY_VALUE, &h)
            == ERROR_SUCCESS) {
        // 显示名：MUIVerb 优先，回退默认值，再回退 humanize 键名。
        std::wstring label;
        std::wstring mui;
        if (reg_read_string(h, L"MUIVerb", &mui) && !mui.empty())
            label = resolve_indirect(mui);
        if (label.empty()) {
            std::wstring def;
            if (reg_read_string(h, nullptr, &def) && !def.empty())
                label = resolve_indirect(def);
        }
        if (label.empty()) label = humanize(verb);
        it.label = wide_to_utf8(label);

        std::wstring icon;
        if (reg_read_string(h, L"Icon", &icon) && !icon.empty())
            it.iconSource = wide_to_utf8(icon);

        // 停用判定：有 LegacyDisable 值即停用。
        std::wstring dummy;
        it.enabled = !reg_read_string(h, L"LegacyDisable", &dummy);
        // 也认 ProgrammaticAccessOnly（隐藏，视作停用样式）。
        if (it.enabled && reg_read_string(h, L"ProgrammaticAccessOnly", &dummy))
            it.enabled = false;

        RegCloseKey(h);
    } else {
        it.label = wide_to_utf8(humanize(verb));
    }

    // 命令：command 子键默认值。
    std::wstring cmdPath = verbPath + L"\\command";
    HKEY hc = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, cmdPath.c_str(), 0, KEY_QUERY_VALUE, &hc)
            == ERROR_SUCCESS) {
        std::wstring cmd;
        if (reg_read_string(hc, nullptr, &cmd)) it.command = wide_to_utf8(cmd);
        RegCloseKey(hc);
    }

    // 系统项：非 cmm_ 自建的都当系统项（文件类型 verb 多为系统/第三方，给强确认）。
    it.system = !it.custom;
    return it;
}

// ---- 读一个 handler 项 ------------------------------------------------------

FtItem read_handler(FtRoot r, const std::wstring& handlerRootPath,
                    const std::wstring& name, bool enabled) {
    FtItem it;
    it.kind = "handler";
    it.rootTag = root_tag(r);
    it.id = std::string(root_tag(r)) + "|handler:" + wide_to_utf8(name);
    it.enabled = enabled;
    it.canToggle = true;
    it.canRename = false;
    it.canChangeIcon = false;
    it.regPath = "HKEY_CLASSES_ROOT\\" + wide_to_utf8(handlerRootPath + L"\\" + name);

    std::wstring handlerPath = handlerRootPath + L"\\" + name;
    std::wstring clsid;
    HKEY h = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, handlerPath.c_str(), 0, KEY_QUERY_VALUE, &h)
            == ERROR_SUCCESS) {
        reg_read_string(h, nullptr, &clsid);
        RegCloseKey(h);
    }

    std::wstring candidate = (name.size() >= 2 && name[0] == L'{') ? name
                           : (clsid.size() >= 2 && clsid[0] == L'{') ? clsid
                           : L"";
    it.clsid = wide_to_utf8(candidate);

    // label：CLSID → HKCR\CLSID\{...}\(LocalizedString / 默认值) → 可读键名 → humanize。
    std::wstring label;
    if (!candidate.empty()) {
        std::wstring clsidPath = L"CLSID\\" + candidate;
        HKEY hClsid = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, clsidPath.c_str(), 0, KEY_QUERY_VALUE, &hClsid)
                == ERROR_SUCCESS) {
            std::wstring loc;
            if (reg_read_string(hClsid, L"LocalizedString", &loc) && !loc.empty())
                label = resolve_indirect(loc);
            if (label.empty()) {
                std::wstring def;
                if (reg_read_string(hClsid, nullptr, &def) && !def.empty())
                    label = resolve_indirect(def);
            }
            RegCloseKey(hClsid);
        }
    }
    if (label.empty()) {
        // 键名不是 CLSID 的话当可读名用，否则 humanize。
        if (!(name.size() >= 2 && name[0] == L'{'))
            label = name;
        else
            label = humanize(name);
    }
    it.label = wide_to_utf8(label);
    it.system = true;  // 文件类型 handler 一律当系统/第三方项（强确认后再停用）
    return it;
}

// 扫某个 shell 根下全部 verb，追加进 out。
void scan_verbs(FtRoot r, const std::wstring& basePath, std::vector<FtItem>* out,
                std::set<std::string>* seenIds) {
    if (basePath.empty()) return;
    std::wstring shellPath = basePath + L"\\shell";
    HKEY h = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, shellPath.c_str(), 0,
                      KEY_ENUMERATE_SUB_KEYS, &h) != ERROR_SUCCESS)
        return;
    std::vector<std::wstring> verbs = enum_subkeys(h);
    RegCloseKey(h);
    for (const std::wstring& v : verbs) {
        FtItem it = read_verb(r, shellPath, v);
        if (seenIds->insert(it.id).second) out->push_back(std::move(it));
    }
}

// 扫某个 shellex\ContextMenuHandlers（含 -ContextMenuHandlers 停用镜像），追加进 out。
void scan_handlers(FtRoot r, const std::wstring& basePath, std::vector<FtItem>* out,
                   std::set<std::string>* seenIds) {
    if (basePath.empty()) return;
    const std::wstring enRoot  = basePath + L"\\shellex\\ContextMenuHandlers";
    const std::wstring disRoot = basePath + L"\\shellex\\-ContextMenuHandlers";

    auto scanOne = [&](const std::wstring& rootPath, bool enabled) {
        HKEY h = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, rootPath.c_str(), 0,
                          KEY_ENUMERATE_SUB_KEYS, &h) != ERROR_SUCCESS)
            return;
        std::vector<std::wstring> names = enum_subkeys(h);
        RegCloseKey(h);
        for (const std::wstring& n : names) {
            FtItem it = read_handler(r, rootPath, n, enabled);
            if (seenIds->insert(it.id).second) out->push_back(std::move(it));
        }
    };
    scanOne(enRoot, true);
    scanOne(disRoot, false);
}

}  // namespace

// ---- 扫描聚合 ---------------------------------------------------------------

std::vector<FtItem> scan_filetype(const std::string& ext) {
    std::vector<FtItem> items;
    FtAssoc a = resolve_assoc_w(ext);
    if (!a.valid) return items;

    std::set<std::string> seen;  // 按 id 去重（同一 handler 可能被多根看到）
    const FtRoot roots[] = { FtRoot::Ext, FtRoot::ProgId,
                             FtRoot::SfaExt, FtRoot::SfaPerceived };
    for (FtRoot r : roots) {
        std::wstring base = root_base_path(r, a);
        if (base.empty()) continue;
        scan_verbs(r, base, &items, &seen);
        scan_handlers(r, base, &items, &seen);
    }
    return items;
}

std::string scan_filetype_json(const std::string& ext) {
    FtAssoc a = resolve_assoc_w(ext);
    std::vector<FtItem> items = scan_filetype(ext);

    std::string o = "{\"assoc\":{";
    o += "\"ext\":\"";           o += json_escape(a.ext);           o += "\"";
    o += ",\"progId\":\"";       o += json_escape(a.progId);        o += "\"";
    o += ",\"perceivedType\":\"";o += json_escape(a.perceivedType); o += "\"";
    o += ",\"valid\":";          o += a.valid ? "true" : "false";
    o += "},\"items\":[";
    bool first = true;
    for (const auto& it : items) {
        if (!first) o += ",";
        first = false;
        o += "{\"id\":\"";          o += json_escape(it.id);          o += "\"";
        o += ",\"kind\":\"";        o += json_escape(it.kind);        o += "\"";
        o += ",\"label\":\"";       o += json_escape(it.label);       o += "\"";
        o += ",\"iconSource\":\"";  o += json_escape(it.iconSource);  o += "\"";
        o += ",\"command\":\"";     o += json_escape(it.command);     o += "\"";
        o += ",\"clsid\":\"";       o += json_escape(it.clsid);       o += "\"";
        o += ",\"rootTag\":\"";     o += json_escape(it.rootTag);     o += "\"";
        o += ",\"regPath\":\"";     o += json_escape(it.regPath);     o += "\"";
        o += ",\"enabled\":";       o += it.enabled ? "true" : "false";
        o += ",\"system\":";        o += it.system ? "true" : "false";
        o += ",\"canToggle\":";     o += it.canToggle ? "true" : "false";
        o += ",\"canRename\":";     o += it.canRename ? "true" : "false";
        o += ",\"canChangeIcon\":"; o += it.canChangeIcon ? "true" : "false";
        o += ",\"custom\":";        o += it.custom ? "true" : "false";
        o += "}";
    }
    o += "]}";
    return o;
}

// ============================================================
//  写操作（启停 / 新建 / 删除 / 备份）
//
//  与 menu_scanner 同策略：写前 reg export 备份、verb 走 LegacyDisable、
//  handler 走 ContextMenuHandlers <-> -ContextMenuHandlers 搬家，两个 hive
//  （HKLM/HKCU）都尝试——同一项可能两处都注册，只改一处合并视图仍不变。
//  本工具 requireAdministrator，两 hive 均可写。
// ============================================================
namespace {

// Classes 下相对路径 → 某 hive 完整子路径。
std::wstring hive_classes_path(const std::wstring& rel) {
    return L"Software\\Classes\\" + rel;
}

std::string result_json_id(bool ok, const char* key, const std::string& id) {
    std::string o = "{\"ok\":";
    o += ok ? "true" : "false";
    o += ",\"key\":\"";
    o += json_escape(key ? key : "");
    o += "\",\"id\":\"";
    o += json_escape(id);
    o += "\"}";
    return o;
}

std::string result_json_path(bool ok, const char* key, const std::string& path) {
    std::string o = "{\"ok\":";
    o += ok ? "true" : "false";
    o += ",\"key\":\"";
    o += json_escape(key ? key : "");
    o += "\",\"path\":\"";
    o += json_escape(path);
    o += "\"}";
    return o;
}

bool set_str(HKEY h, const wchar_t* name, const std::wstring& val) {
    return RegSetValueExW(h, name, 0, REG_SZ,
                          reinterpret_cast<const BYTE*>(val.c_str()),
                          (DWORD)((val.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

// 递归复制注册表键树（值 + 子键）。用于 handler 搬家。与 menu_scanner 同实现。
bool copy_key_tree(HKEY srcRoot, const std::wstring& srcPath,
                   HKEY dstRoot, const std::wstring& dstPath) {
    HKEY hSrc = nullptr;
    if (RegOpenKeyExW(srcRoot, srcPath.c_str(), 0, KEY_READ, &hSrc) != ERROR_SUCCESS)
        return false;
    HKEY hDst = nullptr;
    DWORD disp = 0;
    if (RegCreateKeyExW(dstRoot, dstPath.c_str(), 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                        &hDst, &disp) != ERROR_SUCCESS) {
        RegCloseKey(hSrc);
        return false;
    }
    for (DWORD i = 0; ; ++i) {
        wchar_t name[512]; DWORD cchName = ARRAYSIZE(name);
        DWORD type = 0, cbData = 0;
        LSTATUS st = RegEnumValueW(hSrc, i, name, &cchName, nullptr, &type, nullptr, &cbData);
        if (st == ERROR_NO_MORE_ITEMS || st != ERROR_SUCCESS) break;
        std::vector<BYTE> data(cbData ? cbData : 1);
        cchName = ARRAYSIZE(name);
        if (RegEnumValueW(hSrc, i, name, &cchName, nullptr, &type,
                          data.data(), &cbData) == ERROR_SUCCESS) {
            RegSetValueExW(hDst, name, 0, type, data.data(), cbData);
        }
    }
    for (DWORD i = 0; ; ++i) {
        wchar_t sub[512]; DWORD cch = ARRAYSIZE(sub);
        LSTATUS st = RegEnumKeyExW(hSrc, i, sub, &cch, nullptr, nullptr, nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS || st != ERROR_SUCCESS) break;
        copy_key_tree(srcRoot, srcPath + L"\\" + sub, dstRoot, dstPath + L"\\" + sub);
    }
    RegCloseKey(hSrc);
    RegCloseKey(hDst);
    return true;
}

// ---- 备份 -------------------------------------------------------------------

// %LOCALAPPDATA%\ContextMenuMaster\backups\，逐级创建。失败返回空。
std::wstring ensure_backup_dir() {
    wchar_t base[MAX_PATH] = {0};
    DWORD got = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    if (got == 0 || got >= MAX_PATH) return L"";
    std::wstring dir = base;
    dir += L"\\ContextMenuMaster";
    CreateDirectoryW(dir.c_str(), nullptr);
    dir += L"\\backups";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

std::wstring timestamp_now() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[32];
    wsprintfW(buf, L"%04u%02u%02u_%02u%02u%02u",
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

// 扩展名去前导点作文件名片段（".txt" -> "txt"）。
std::string ext_slug(const std::string& ext) {
    std::string s = ext;
    while (!s.empty() && s.front() == '.') s.erase(s.begin());
    return s.empty() ? "ext" : s;
}

// reg.exe export "HKCR\<relPath>" "<file>" /y。relPath 空或键不存在则跳过（返回 true 不报错）。
bool export_hkcr_key(const std::wstring& relPath, const std::string& ext,
                     const char* tag, std::string* outPath) {
    if (relPath.empty()) return true;
    if (!key_exists(HKEY_CLASSES_ROOT, relPath)) return true;  // 无此根，无需备份

    std::wstring dir = ensure_backup_dir();
    if (dir.empty()) return false;

    std::wstring file = dir + L"\\filetype_" + utf8_to_wide(ext_slug(ext)) +
                        L"_" + utf8_to_wide(tag ? tag : "") + L"_" + timestamp_now() + L".reg";

    wchar_t sysDir[MAX_PATH] = {0};
    if (GetSystemDirectoryW(sysDir, MAX_PATH) == 0) return false;
    std::wstring regExe = std::wstring(sysDir) + L"\\reg.exe";

    std::wstring cmd = L"\"" + regExe + L"\" export \"HKCR\\" + relPath +
                       L"\" \"" + file + L"\" /y";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};
    std::wstring mutableCmd = cmd;
    if (!CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        return false;
    DWORD waitRc = WaitForSingleObject(pi.hProcess, 30000);
    if (waitRc == WAIT_TIMEOUT) {
        CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
        return false;
    }
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    if (exitCode != 0) return false;
    if (outPath) *outPath = wide_to_utf8(file);
    return true;
}

// 写前备份「即将改动的那个根」（够用即可，非全树）。relBase 为该根基路径。
bool backup_one_root(const std::wstring& relBase, const std::string& ext, const char* tag) {
    std::string dummy;
    return export_hkcr_key(relBase, ext, tag, &dummy);
}

// ---- id 解析 ----------------------------------------------------------------

// "<tag>|verb:<name>" / "<tag>|handler:<name>" 拆解。成功返回 true。
bool parse_ft_id(const std::string& id, std::string* tag, bool* isHandler,
                 std::wstring* name) {
    size_t bar = id.find('|');
    if (bar == std::string::npos) return false;
    *tag = id.substr(0, bar);
    std::string rest = id.substr(bar + 1);
    const std::string vp = "verb:", hp = "handler:";
    if (rest.compare(0, vp.size(), vp) == 0) {
        *isHandler = false;
        *name = utf8_to_wide(rest.substr(vp.size()));
    } else if (rest.compare(0, hp.size(), hp) == 0) {
        *isHandler = true;
        *name = utf8_to_wide(rest.substr(hp.size()));
    } else {
        return false;
    }
    return !name->empty();
}

// ---- 命令组装（文件类型恒作用于选中文件 %1） -------------------------------

std::wstring build_ft_command(const std::string& kind, const std::wstring& target) {
    if (kind == "folder") return L"explorer.exe \"" + target + L"\"";
    if (kind == "url")    return L"cmd /c start \"\" \"" + target + L"\"";
    if (kind == "command") return target;  // 原样，用户自带占位符
    return L"\"" + target + L"\" \"%1\"";   // program（默认）
}

constexpr wchar_t kCmmPrefix[] = L"cmm_";

bool is_cmm_name(const std::wstring& name) {
    return name.rfind(kCmmPrefix, 0) == 0;
}

}  // namespace

// ---- 启用 / 停用 ------------------------------------------------------------

std::string set_filetype_enabled(const std::string& ext, const std::string& id, bool enabled) {
    FtAssoc a = resolve_assoc_w(ext);
    if (!a.valid) return result_json(false, "bad_ext");

    std::string tag;
    bool isHandler = false;
    std::wstring name;
    if (!parse_ft_id(id, &tag, &isHandler, &name)) return result_json(false, "bad_id");

    FtRoot r;
    if (!root_from_tag(tag, &r)) return result_json(false, "bad_id");
    std::wstring base = root_base_path(r, a);
    if (base.empty()) return result_json(false, "not_found");

    if (!backup_one_root(base, a.ext, tag.c_str())) return result_json(false, "backup_failed");

    if (isHandler) {
        // handler：启用根 <-> 停用根 搬家，两 hive 都试。
        const std::wstring enRel  = base + L"\\shellex\\ContextMenuHandlers\\" + name;
        const std::wstring disRel = base + L"\\shellex\\-ContextMenuHandlers\\" + name;
        auto moveIn = [&](HKEY hive) -> bool {
            std::wstring src = hive_classes_path(enabled ? disRel : enRel);
            std::wstring dst = hive_classes_path(enabled ? enRel : disRel);
            HKEY probe = nullptr;
            if (RegOpenKeyExW(hive, src.c_str(), 0, KEY_READ, &probe) != ERROR_SUCCESS)
                return false;
            RegCloseKey(probe);
            if (!copy_key_tree(hive, src, hive, dst)) return false;
            RegDeleteTreeW(hive, src.c_str());
            return true;
        };
        bool m1 = moveIn(HKEY_LOCAL_MACHINE);
        bool m2 = moveIn(HKEY_CURRENT_USER);
        if (m1 || m2) return result_json(true, "ok");
        // 没搬动：可能已在目标态（幂等），检查目标位置是否存在。
        std::wstring target = hive_classes_path(enabled ? enRel : disRel);
        if (key_exists(HKEY_LOCAL_MACHINE, enabled ? enRel : disRel) ||
            key_exists(HKEY_CURRENT_USER,  enabled ? enRel : disRel))
            return result_json(true, "ok");
        return result_json(false, "registry_failed");
    }

    // verb：删/加 LegacyDisable，两 hive 都处理。
    const std::wstring shellRel = base + L"\\shell\\" + name;
    bool anyOk = false;
    if (enabled) {
        for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
            HKEY h = nullptr;
            if (RegOpenKeyExW(hive, hive_classes_path(shellRel).c_str(), 0,
                              KEY_SET_VALUE, &h) == ERROR_SUCCESS) {
                LSTATUS st = RegDeleteValueW(h, L"LegacyDisable");
                RegCloseKey(h);
                if (st == ERROR_SUCCESS || st == ERROR_FILE_NOT_FOUND) anyOk = true;
            }
        }
    } else {
        for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
            HKEY h = nullptr;
            if (RegOpenKeyExW(hive, hive_classes_path(shellRel).c_str(), 0,
                              KEY_SET_VALUE, &h) == ERROR_SUCCESS) {
                const wchar_t empty[] = L"";
                LSTATUS st = RegSetValueExW(h, L"LegacyDisable", 0, REG_SZ,
                                            reinterpret_cast<const BYTE*>(empty),
                                            sizeof(wchar_t));
                RegCloseKey(h);
                if (st == ERROR_SUCCESS) anyOk = true;
            }
        }
        if (!anyOk) {
            // 两 hive 都无此 verb 键（仅合并视图可见）：HKCU 建键写标记，保证停用有落点。
            HKEY h = nullptr;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, hive_classes_path(shellRel).c_str(),
                                0, nullptr, 0, KEY_SET_VALUE, nullptr, &h, nullptr)
                    == ERROR_SUCCESS) {
                const wchar_t empty[] = L"";
                LSTATUS st = RegSetValueExW(h, L"LegacyDisable", 0, REG_SZ,
                                            reinterpret_cast<const BYTE*>(empty),
                                            sizeof(wchar_t));
                RegCloseKey(h);
                anyOk = (st == ERROR_SUCCESS);
            }
        }
    }
    return anyOk ? result_json(true, "ok") : result_json(false, "registry_failed");
}

// ---- 新建自建项（写 HKCU SFA\.ext\shell\cmm_xxx） ---------------------------

std::string add_filetype_item(const std::string& ext, const std::string& label,
                              const std::string& kind, const std::string& target,
                              const std::string& iconSource, const std::string& position) {
    FtAssoc a = resolve_assoc_w(ext);
    if (!a.valid) return result_json(false, "bad_ext");
    if (label.empty() || target.empty()) return result_json(false, "bad_arg");

    std::wstring extW = utf8_to_wide(a.ext);
    // 挂载点：HKCU\Software\Classes\SystemFileAssociations\.ext\shell（用户级，全局免污染）。
    std::wstring shellRel = L"SystemFileAssociations\\" + extW + L"\\shell";

    if (!backup_one_root(L"SystemFileAssociations\\" + extW, a.ext, "sfaext"))
        return result_json(false, "backup_failed");

    // 唯一 verb id：cmm_<tick>，冲突递增。
    auto verb_exists = [&](const std::wstring& v) -> bool {
        HKEY h = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
                          hive_classes_path(shellRel + L"\\" + v).c_str(),
                          0, KEY_READ, &h) == ERROR_SUCCESS) { RegCloseKey(h); return true; }
        return false;
    };
    std::wstring verbId = kCmmPrefix + std::to_wstring(GetTickCount64());
    for (int i = 0; verb_exists(verbId) && i < 1000; ++i)
        verbId = kCmmPrefix + std::to_wstring(GetTickCount64()) + L"_" + std::to_wstring(i);

    std::wstring verbRel = shellRel + L"\\" + verbId;
    HKEY hVerb = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, hive_classes_path(verbRel).c_str(),
                        0, nullptr, 0, KEY_SET_VALUE, nullptr, &hVerb, nullptr)
            != ERROR_SUCCESS)
        return result_json(false, "registry_failed");

    std::wstring wLabel = utf8_to_wide(label);
    set_str(hVerb, L"MUIVerb", wLabel);
    set_str(hVerb, nullptr, wLabel);
    std::wstring wIcon = utf8_to_wide(iconSource);
    if (!wIcon.empty()) set_str(hVerb, L"Icon", wIcon);
    if (position == "top")         set_str(hVerb, L"Position", L"Top");
    else if (position == "bottom") set_str(hVerb, L"Position", L"Bottom");
    RegCloseKey(hVerb);

    std::wstring cmd = build_ft_command(kind, utf8_to_wide(target));
    HKEY hCmd = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, hive_classes_path(verbRel + L"\\command").c_str(),
                        0, nullptr, 0, KEY_SET_VALUE, nullptr, &hCmd, nullptr)
            != ERROR_SUCCESS)
        return result_json(false, "registry_failed");
    bool ok = set_str(hCmd, nullptr, cmd);
    RegCloseKey(hCmd);
    if (!ok) return result_json(false, "registry_failed");

    std::string id = "sfaext|verb:" + wide_to_utf8(verbId);
    return result_json_id(true, "ok", id);
}

// ---- 删除自建项（仅 cmm_ 前缀、仅 HKCU SFA\.ext） --------------------------

std::string del_filetype_item(const std::string& ext, const std::string& id) {
    FtAssoc a = resolve_assoc_w(ext);
    if (!a.valid) return result_json(false, "bad_ext");

    std::string tag;
    bool isHandler = false;
    std::wstring name;
    if (!parse_ft_id(id, &tag, &isHandler, &name)) return result_json(false, "bad_id");
    if (isHandler) return result_json(false, "not_custom");
    if (!is_cmm_name(name)) return result_json(false, "not_custom");

    std::wstring extW = utf8_to_wide(a.ext);
    std::wstring verbRel = L"SystemFileAssociations\\" + extW + L"\\shell\\" + name;

    // 仅删 HKCU（本工具唯一写入位置），不碰 HKLM/全局。
    HKEY h = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, hive_classes_path(verbRel).c_str(),
                      0, KEY_READ, &h) != ERROR_SUCCESS)
        return result_json(false, "not_found");
    RegCloseKey(h);

    if (!backup_one_root(L"SystemFileAssociations\\" + extW, a.ext, "sfaext"))
        return result_json(false, "backup_failed");

    LSTATUS st = RegDeleteTreeW(HKEY_CURRENT_USER, hive_classes_path(verbRel).c_str());
    return st == ERROR_SUCCESS ? result_json(true, "ok") : result_json(false, "registry_failed");
}

// ---- 备份整棵关联树（用户主动备份） ----------------------------------------

std::string backup_filetype(const std::string& ext) {
    FtAssoc a = resolve_assoc_w(ext);
    if (!a.valid) return result_json(false, "bad_ext");

    std::wstring extW = utf8_to_wide(a.ext);
    bool anyOk = false;
    std::string firstPath;

    struct { std::wstring rel; const char* tag; } roots[] = {
        { extW,                                         "ext" },
        { a.progId.empty() ? L"" : utf8_to_wide(a.progId), "progid" },
        { L"SystemFileAssociations\\" + extW,           "sfaext" },
        { a.perceivedType.empty() ? L""
            : L"SystemFileAssociations\\" + utf8_to_wide(a.perceivedType), "sfaperceived" },
    };
    for (auto& rt : roots) {
        if (rt.rel.empty()) continue;
        if (!key_exists(HKEY_CLASSES_ROOT, rt.rel)) continue;
        std::string path;
        if (export_hkcr_key(rt.rel, a.ext, rt.tag, &path) && !path.empty()) {
            anyOk = true;
            if (firstPath.empty()) firstPath = path;
        }
    }
    if (!anyOk) return result_json(false, "backup_failed");
    return result_json_path(true, "ok", firstPath);
}

}  // namespace cmm
