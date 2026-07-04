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

// 读一个 Applications\<app> 子键为 OpenWithItem。appKeyRoot 已打开到 Applications。
OpenWithItem read_app(HKEY hApps, const std::wstring& appName) {
    OpenWithItem item;
    item.id = wide_to_utf8(appName);
    item.enabled = true;
    item.canToggle = false;

    std::wstring appSub = appName;

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
        std::vector<std::wstring> names = enum_apps(hive);
        for (const std::wstring& n : names) {
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

            items.push_back(read_app(hApps, n));
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
        out += ",\"enabled\":true";
        out += ",\"canToggle\":false";
        out += "}";
    }
    out += "]";
    return out;
}

}  // namespace cmm
