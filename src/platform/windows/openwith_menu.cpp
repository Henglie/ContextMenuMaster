// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 打开方式（OpenWith）应用列表（实现）
//
//  只读枚举 HKLM\SOFTWARE\Classes\Applications 与 HKCU\Software\Classes\Applications
//  下注册的应用（各 <app.exe> 子键），读友好名与 open 命令。两 hive 合并去重
//  （同名可执行以先见者为准，HKLM 优先）。不提供任何写操作。
// ============================================================
#include "platform/windows/openwith_menu.h"

#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

namespace cmm {

namespace {

std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                        s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

std::string result_json(bool ok, const char* key) {
    std::string s = "{\"ok\":";
    s += ok ? "true" : "false";
    s += ",\"key\":\""; s += key; s += "\"}";
    return s;
}

// 递归复制一棵注册表键树（含所有值与子键）。srcKey/dstKey 均已打开。
// 用于 OpenWith 软删除的整树搬家（<app> -> <app>.deleted）。
bool copy_key_tree(HKEY srcKey, HKEY dstKey) {
    // 1) 复制本键所有值。
    DWORD idx = 0;
    wchar_t valName[16384];
    BYTE data[8192];
    for (;;) {
        DWORD cchName = ARRAYSIZE(valName);
        DWORD cbData = sizeof(data);
        DWORD type = 0;
        LSTATUS st = RegEnumValueW(srcKey, idx, valName, &cchName, nullptr,
                                   &type, data, &cbData);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st == ERROR_SUCCESS)
            RegSetValueExW(dstKey, valName, 0, type, data, cbData);
        ++idx;
    }
    // 2) 递归复制所有子键。
    idx = 0;
    wchar_t subName[256];
    for (;;) {
        DWORD cch = ARRAYSIZE(subName);
        LSTATUS st = RegEnumKeyExW(srcKey, idx, subName, &cch, nullptr, nullptr,
                                   nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st == ERROR_SUCCESS) {
            HKEY srcSub = nullptr, dstSub = nullptr;
            if (RegOpenKeyExW(srcKey, subName, 0, KEY_READ, &srcSub) == ERROR_SUCCESS) {
                if (RegCreateKeyExW(dstKey, subName, 0, nullptr, 0,
                                    KEY_WRITE, nullptr, &dstSub, nullptr) == ERROR_SUCCESS) {
                    copy_key_tree(srcSub, dstSub);
                    RegCloseKey(dstSub);
                }
                RegCloseKey(srcSub);
            }
        }
        ++idx;
    }
    return true;
}

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

// 展开 %var%（FriendlyAppName / DefaultIcon 常带 @dll,-id 或 %systemroot% 等）。
std::wstring expand_env(const std::wstring& s) {
    if (s.empty()) return s;
    wchar_t buf[1024] = {0};
    DWORD n = ExpandEnvironmentStringsW(s.c_str(), buf, 1024);
    if (n == 0 || n > 1024) return s;
    return buf;
}

// 读某注册表值（REG_SZ / REG_EXPAND_SZ）。不存在返回空。
std::wstring read_reg_string(HKEY hKey, const wchar_t* sub, const wchar_t* valueName) {
    HKEY h = hKey;
    bool opened = false;
    if (sub && sub[0]) {
        if (RegOpenKeyExW(hKey, sub, 0, KEY_QUERY_VALUE, &h) != ERROR_SUCCESS)
            return L"";
        opened = true;
    }
    std::wstring out;
    DWORD type = 0, cb = 0;
    if (RegQueryValueExW(h, valueName, nullptr, &type, nullptr, &cb) == ERROR_SUCCESS
        && (type == REG_SZ || type == REG_EXPAND_SZ) && cb > 0) {
        std::wstring buf(cb / sizeof(wchar_t) + 1, L'\0');
        if (RegQueryValueExW(h, valueName, nullptr, nullptr,
                reinterpret_cast<BYTE*>(buf.data()), &cb) == ERROR_SUCCESS) {
            buf.resize(wcslen(buf.c_str()));
            out = buf;
        }
    }
    if (opened) RegCloseKey(h);
    return out;
}

// 解析 @dll,-id 间接字符串资源为明文（FriendlyAppName 常是这种）。
std::wstring resolve_indirect(const std::wstring& spec) {
    if (spec.empty() || spec[0] != L'@') return spec;
    // SHLoadIndirectString 需展开 %systemroot% 后才可靠。
    std::wstring expanded = expand_env(spec);
    wchar_t buf[1024] = {0};
    typedef HRESULT (WINAPI *PFN)(PCWSTR, PWSTR, UINT, void**);
    static PFN pfn = []() -> PFN {
        HMODULE h = LoadLibraryW(L"shlwapi.dll");
        return h ? reinterpret_cast<PFN>(GetProcAddress(h, "SHLoadIndirectString")) : nullptr;
    }();
    if (pfn && SUCCEEDED(pfn(expanded.c_str(), buf, 1024, nullptr)) && buf[0]) {
        return buf;
    }
    return L"";
}

// 读一个 Applications\<app> 子键为 OpenWithItem。hApps 已打开到某 hive 的 Applications。
// fromHklm 标记该项来自机器级（HKLM），前端给二次确认、del 拒绝。
OpenWithItem read_app(HKEY hApps, const std::wstring& appName, bool fromHklm) {
    OpenWithItem item;
    item.id = wide_to_utf8(appName);
    item.enabled = true;
    item.canToggle = true;   // 启停 = 写/删 NoOpenWith 值，纯值操作，安全可逆
    item.system = fromHklm;  // 机器级项：前端二次确认，不允许 del

    std::wstring appSub = appName;

    // 停用态：应用键下有 NoOpenWith 值即被排除出「打开方式」列表。
    HKEY hApp = nullptr;
    if (RegOpenKeyExW(hApps, appSub.c_str(), 0, KEY_QUERY_VALUE, &hApp) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hApp, L"NoOpenWith", nullptr, nullptr, nullptr, nullptr)
            == ERROR_SUCCESS)
            item.enabled = false;
        RegCloseKey(hApp);
    }

    // 友好名：FriendlyAppName（可能是 @dll,-id）→ 解析 → 回退可执行名。
    std::wstring friendly = read_reg_string(hApps, appSub.c_str(), L"FriendlyAppName");
    if (!friendly.empty() && friendly[0] == L'@') {
        std::wstring r = resolve_indirect(friendly);
        friendly = (!r.empty() && r[0] != L'@') ? r : L"";
    }
    if (friendly.empty()) friendly = appName;  // 回退可执行名
    item.label = wide_to_utf8(friendly);

    // open 命令：<app>\shell\open\command 默认值。
    std::wstring cmdKey = appSub + L"\\shell\\open\\command";
    std::wstring cmd = read_reg_string(hApps, cmdKey.c_str(), nullptr);
    if (!cmd.empty()) item.command = wide_to_utf8(cmd);

    // 图标：DefaultIcon 默认值 → 回退命令里的 exe。
    std::wstring diKey = appSub + L"\\DefaultIcon";
    std::wstring di = read_reg_string(hApps, diKey.c_str(), nullptr);
    if (!di.empty() && di != L"%1") {
        item.iconSource = wide_to_utf8(expand_env(di));
    } else if (!cmd.empty()) {
        // 从命令里抠出 exe 路径（首个引号包裹段或首 token），作图标源。
        std::wstring c = expand_env(cmd);
        std::wstring exe;
        if (c[0] == L'"') {
            size_t end = c.find(L'"', 1);
            if (end != std::wstring::npos) exe = c.substr(1, end - 1);
        } else {
            size_t sp = c.find(L' ');
            exe = (sp == std::wstring::npos) ? c : c.substr(0, sp);
        }
        if (!exe.empty()) item.iconSource = wide_to_utf8(exe + L",0");
    }
    return item;
}

// 枚举某 hive 的 Applications 下全部 <app> 子键名。
std::vector<std::wstring> enum_apps(HKEY hive) {
    std::vector<std::wstring> out;
    HKEY hApps = nullptr;
    if (RegOpenKeyExW(hive, L"Software\\Classes\\Applications", 0,
                      KEY_ENUMERATE_SUB_KEYS, &hApps) != ERROR_SUCCESS)
        return out;
    DWORD i = 0;
    wchar_t name[256];
    for (;;) {
        DWORD cch = ARRAYSIZE(name);
        LSTATUS st = RegEnumKeyExW(hApps, i, name, &cch, nullptr, nullptr, nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st == ERROR_SUCCESS) out.push_back(name);
        ++i;
    }
    RegCloseKey(hApps);
    return out;
}

}  // namespace

std::vector<OpenWithItem> scan_openwith() {
    std::vector<OpenWithItem> items;

    // 两 hive 合并去重（同名 app 以先见者为准，HKLM 优先）。
    std::vector<std::wstring> seen;
    for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
        HKEY hApps = nullptr;
        if (RegOpenKeyExW(hive, L"Software\\Classes\\Applications", 0,
                          KEY_ENUMERATE_SUB_KEYS, &hApps) != ERROR_SUCCESS)
            continue;
        bool fromHklm = (hive == HKEY_LOCAL_MACHINE);
        std::vector<std::wstring> names = enum_apps(hive);
        for (const std::wstring& n : names) {
            // 软删除镜像（<app>.deleted）不列入主列表。
            if (n.size() >= 8 &&
                CompareStringOrdinal(n.c_str() + (n.size() - 8), 8, L".deleted", 8, TRUE)
                    == CSTR_EQUAL)
                continue;
            // 只列真正是应用（有 shell\open\command）的项，过滤纯配置壳。
            std::wstring cmdKey = n + L"\\shell\\open\\command";
            std::wstring cmd = read_reg_string(hApps, cmdKey.c_str(), nullptr);
            if (cmd.empty()) continue;

            // 去重（大小写不敏感）。
            bool dup = false;
            for (const std::wstring& s : seen)
                if (CompareStringOrdinal(s.c_str(), -1, n.c_str(), -1, TRUE) == CSTR_EQUAL) {
                    dup = true; break;
                }
            if (dup) continue;
            seen.push_back(n);

            items.push_back(read_app(hApps, n, fromHklm));
        }
        RegCloseKey(hApps);
    }

    std::sort(items.begin(), items.end(),
              [](const OpenWithItem& a, const OpenWithItem& b) {
                  return a.label < b.label;
              });
    return items;
}

std::string scan_openwith_json() {
    std::vector<OpenWithItem> items = scan_openwith();
    std::string out = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        const OpenWithItem& it = items[i];
        if (i) out += ",";
        out += "{\"id\":\"";        out += json_escape(it.id);         out += "\"";
        out += ",\"label\":\"";     out += json_escape(it.label);      out += "\"";
        out += ",\"command\":\"";   out += json_escape(it.command);    out += "\"";
        out += ",\"iconSource\":\"";out += json_escape(it.iconSource); out += "\"";
        out += ",\"enabled\":";     out += it.enabled ? "true" : "false";
        out += ",\"canToggle\":";   out += it.canToggle ? "true" : "false";
        out += ",\"system\":";      out += it.system ? "true" : "false";
        out += "}";
    }
    out += "]";
    return out;
}

namespace {

// app 子键名校验：禁路径穿越/分隔符。
bool valid_app_name(const std::wstring& n) {
    if (n.empty()) return false;
    if (n.find(L"..") != std::wstring::npos ||
        n.find(L'\\') != std::wstring::npos ||
        n.find(L'/') != std::wstring::npos)
        return false;
    return true;
}

// 在某 hive 下打开 Applications\<app> 键。create=true 时不存在则建。
bool open_app_key(HKEY hive, const std::wstring& app, REGSAM access,
                  bool create, HKEY* out) {
    std::wstring path = L"Software\\Classes\\Applications\\" + app;
    if (create) {
        return RegCreateKeyExW(hive, path.c_str(), 0, nullptr, 0,
                               access, nullptr, out, nullptr) == ERROR_SUCCESS;
    }
    return RegOpenKeyExW(hive, path.c_str(), 0, access, out) == ERROR_SUCCESS;
}

// 判某 hive 下 app 是否存在。
bool app_exists(HKEY hive, const std::wstring& app) {
    HKEY h = nullptr;
    if (open_app_key(hive, app, KEY_READ, false, &h)) { RegCloseKey(h); return true; }
    return false;
}

}  // namespace

std::string set_openwith_enabled(const std::string& id, bool enabled) {
    std::wstring app = utf8_to_wide(id);
    if (!valid_app_name(app)) return result_json(false, "bad_id");

    // 项真实所在 hive：HKLM 优先（与 scan 去重规则一致），两 hive 都尝试写，
    // 保证合并视图里该 app 被正确启停（同 menu_scanner verb 双 hive 对称思路）。
    bool anyOk = false, anyExist = false;
    for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
        if (!app_exists(hive, app)) continue;
        anyExist = true;
        HKEY h = nullptr;
        if (!open_app_key(hive, app, KEY_SET_VALUE, false, &h)) continue;
        if (enabled) {
            LSTATUS st = RegDeleteValueW(h, L"NoOpenWith");
            if (st == ERROR_SUCCESS || st == ERROR_FILE_NOT_FOUND) anyOk = true;
        } else {
            const wchar_t empty[] = L"";
            LSTATUS st = RegSetValueExW(h, L"NoOpenWith", 0, REG_SZ,
                                        reinterpret_cast<const BYTE*>(empty),
                                        sizeof(wchar_t));
            if (st == ERROR_SUCCESS) anyOk = true;
        }
        RegCloseKey(h);
    }
    if (!anyExist) return result_json(false, "not_found");
    return anyOk ? result_json(true, "ok") : result_json(false, "registry_failed");
}

std::string add_openwith_item(const std::string& targetPath, const std::string& name) {
    std::wstring target = utf8_to_wide(targetPath);
    std::wstring disp = utf8_to_wide(name);
    if (target.empty()) return result_json(false, "bad_arg");
    if (GetFileAttributesW(target.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json(false, "target_not_found");

    // app 子键名取目标 exe 文件名（如 "myapp.exe"）。
    std::wstring app = target;
    size_t slash = app.find_last_of(L"\\/");
    if (slash != std::wstring::npos) app = app.substr(slash + 1);
    if (!valid_app_name(app)) return result_json(false, "bad_name");
    if (disp.empty()) disp = app;

    // 新增只写用户 hive（HKCU），不碰 HKLM。已存在则拒（避免覆盖系统/他人项）。
    if (app_exists(HKEY_CURRENT_USER, app)) return result_json(false, "already_exists");

    HKEY hApp = nullptr;
    if (!open_app_key(HKEY_CURRENT_USER, app, KEY_WRITE, true, &hApp))
        return result_json(false, "registry_failed");
    // FriendlyAppName = 显示名。
    RegSetValueExW(hApp, L"FriendlyAppName", 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(disp.c_str()),
                   (DWORD)((disp.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hApp);

    // shell\open\command = "<target>" "%1"
    std::wstring cmdPath = L"Software\\Classes\\Applications\\" + app +
                           L"\\shell\\open\\command";
    HKEY hCmd = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, cmdPath.c_str(), 0, nullptr, 0,
                        KEY_WRITE, nullptr, &hCmd, nullptr) != ERROR_SUCCESS)
        return result_json(false, "registry_failed");
    std::wstring cmd = L"\"" + target + L"\" \"%1\"";
    RegSetValueExW(hCmd, nullptr, 0, REG_SZ,
                   reinterpret_cast<const BYTE*>(cmd.c_str()),
                   (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(hCmd);
    return result_json(true, "ok");
}

std::string del_openwith_item(const std::string& id) {
    std::wstring app = utf8_to_wide(id);
    if (!valid_app_name(app)) return result_json(false, "bad_id");

    // 只允许删用户 hive（HKCU）项；HKLM 系统项拒绝（避免破坏系统关联）。
    if (!app_exists(HKEY_CURRENT_USER, app)) {
        if (app_exists(HKEY_LOCAL_MACHINE, app))
            return result_json(false, "system_readonly");
        return result_json(false, "not_found");
    }

    // 软删除：<app> 整树搬到 <app>.deleted（可恢复，不真删）。
    std::wstring base = L"Software\\Classes\\Applications\\";
    std::wstring src = base + app;
    std::wstring dst = base + app + L".deleted";
    // 目标已存在（之前删过同名）先清掉旧的 .deleted。
    RegDeleteTreeW(HKEY_CURRENT_USER, dst.c_str());
    HKEY hDst = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, dst.c_str(), 0, nullptr, 0,
                        KEY_WRITE, nullptr, &hDst, nullptr) != ERROR_SUCCESS)
        return result_json(false, "registry_failed");
    HKEY hSrc = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, src.c_str(), 0, KEY_READ, &hSrc)
        != ERROR_SUCCESS) {
        RegCloseKey(hDst);
        return result_json(false, "not_found");
    }
    bool ok = copy_key_tree(hSrc, hDst);
    RegCloseKey(hSrc);
    RegCloseKey(hDst);
    if (!ok) return result_json(false, "registry_failed");
    RegDeleteTreeW(HKEY_CURRENT_USER, src.c_str());
    return result_json(true, "ok");
}

}  // namespace cmm
