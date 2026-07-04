// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 新建菜单（ShellNew）管理（实现）
//
//  遍历 HKCR 下所有 "." 开头的扩展名键，看它（或其关联 ProgID）下是否有 ShellNew
//  子键——有即「新建」菜单一项。启停 = ShellNew 子键整树搬到同级 "-ShellNew"（可逆）。
//  搬家在项真实所在的 hive（HKLM\SOFTWARE\Classes 或 HKCU\Software\Classes）里做，
//  两 hive 都尝试（同 menu_scanner 的 handler 搬家思路）。写前自动 reg 备份。
// ============================================================
#include "platform/windows/shellnew_menu.h"

#include <windows.h>
#include <shlwapi.h>     // SHLoadIndirectString
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace cmm {

namespace {

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

std::string result_json(bool ok, const char* key) {
    std::string s = "{\"ok\":";
    s += ok ? "true" : "false";
    s += ",\"key\":\"";
    s += key;
    s += "\"}";
    return s;
}

// 读某注册表值（REG_SZ / REG_EXPAND_SZ）为宽字符串。不存在返回空。
std::wstring read_reg_string(HKEY hKey, const wchar_t* valueName) {
    DWORD type = 0, cb = 0;
    if (RegQueryValueExW(hKey, valueName, nullptr, &type, nullptr, &cb) != ERROR_SUCCESS)
        return L"";
    if (type != REG_SZ && type != REG_EXPAND_SZ) return L"";
    std::wstring buf(cb / sizeof(wchar_t) + 1, L'\0');
    if (RegQueryValueExW(hKey, valueName, nullptr, nullptr,
                reinterpret_cast<BYTE*>(buf.data()), &cb) != ERROR_SUCCESS)
        return L"";
    buf.resize(wcslen(buf.c_str()));
    return buf;
}

// @dll,-id 间接资源字符串解析（同 menu_scanner 思路，预展开 %systemroot%）。
std::wstring resolve_indirect_string(const std::wstring& raw) {
    if (raw.empty() || raw[0] != L'@') return raw;
    std::wstring expanded(1024, L'\0');
    DWORD n = ExpandEnvironmentStringsW(raw.c_str(), expanded.data(), 1024);
    if (n > 0 && n <= 1024) expanded.resize(n - 1);
    else expanded = raw;
    wchar_t out[512] = {0};
    if (SUCCEEDED(SHLoadIndirectString(expanded.c_str(), out, 512, nullptr))
        && out[0] != L'\0')
        return out;
    return L"";
}

// 某子键是否存在。
bool subkey_exists(HKEY hParent, const wchar_t* sub) {
    HKEY h = nullptr;
    if (RegOpenKeyExW(hParent, sub, 0, KEY_READ, &h) == ERROR_SUCCESS) {
        RegCloseKey(h);
        return true;
    }
    return false;
}

// 读扩展名关联类型的友好名：.txt -> 其 ProgID 的 FriendlyTypeName / ProgID 键默认值。
// 取不到回落扩展名本身。extKey 为已打开的 HKCR\.ext。
std::wstring friendly_type_name(const std::wstring& ext, const std::wstring& progId) {
    // 1) ProgID 的 FriendlyTypeName（常是 @dll,-id）或默认值。
    if (!progId.empty()) {
        HKEY hp = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, progId.c_str(), 0,
                          KEY_QUERY_VALUE, &hp) == ERROR_SUCCESS) {
            std::wstring ftn = read_reg_string(hp, L"FriendlyTypeName");
            std::wstring resolved = resolve_indirect_string(ftn);
            if (resolved.empty() || resolved[0] == L'@') {
                std::wstring def = read_reg_string(hp, nullptr);
                if (!def.empty() && def[0] != L'@') resolved = def;
            }
            RegCloseKey(hp);
            if (!resolved.empty() && resolved[0] != L'@') return resolved;
        }
    }
    return L"";
}

// 从 ProgID 的 DefaultIcon 取图标源（可空）。
std::wstring progid_icon(const std::wstring& progId) {
    if (progId.empty()) return L"";
    std::wstring diKey = progId + L"\\DefaultIcon";
    HKEY h = nullptr;
    std::wstring spec;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, diKey.c_str(), 0,
                      KEY_QUERY_VALUE, &h) == ERROR_SUCCESS) {
        std::wstring di = read_reg_string(h, nullptr);
        if (!di.empty() && di != L"%1") {
            std::wstring exp(1024, L'\0');
            DWORD n = ExpandEnvironmentStringsW(di.c_str(), exp.data(), 1024);
            if (n > 0 && n <= 1024) { exp.resize(n - 1); spec = exp; }
            else spec = di;
        }
        RegCloseKey(h);
    }
    return spec;
}

// —— 递归复制注册表键树（自持一份，同 menu_scanner:copy_key_tree）——
bool copy_key_tree(HKEY srcParent, const std::wstring& srcSub,
                   HKEY dstParent, const std::wstring& dstSub) {
    HKEY hSrc = nullptr;
    if (RegOpenKeyExW(srcParent, srcSub.c_str(), 0, KEY_READ, &hSrc) != ERROR_SUCCESS)
        return false;
    HKEY hDst = nullptr;
    if (RegCreateKeyExW(dstParent, dstSub.c_str(), 0, nullptr, 0,
                        KEY_WRITE, nullptr, &hDst, nullptr) != ERROR_SUCCESS) {
        RegCloseKey(hSrc);
        return false;
    }
    // 1) 复制值。
    DWORD idx = 0;
    wchar_t valName[16384];
    BYTE data[8192];
    for (;;) {
        DWORD cchName = ARRAYSIZE(valName);
        DWORD cbData = sizeof(data);
        DWORD type = 0;
        LSTATUS st = RegEnumValueW(hSrc, idx, valName, &cchName, nullptr,
                                   &type, data, &cbData);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st == ERROR_SUCCESS)
            RegSetValueExW(hDst, valName, 0, type, data, cbData);
        ++idx;
    }
    // 2) 递归子键。
    idx = 0;
    wchar_t subName[256];
    for (;;) {
        DWORD cch = ARRAYSIZE(subName);
        LSTATUS st = RegEnumKeyExW(hSrc, idx, subName, &cch, nullptr, nullptr,
                                   nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st == ERROR_SUCCESS)
            copy_key_tree(hSrc, subName, hDst, subName);
        ++idx;
    }
    RegCloseKey(hDst);
    RegCloseKey(hSrc);
    return true;
}

// 在某 hive 下把 relParent\ShellNew 搬到 relParent\-ShellNew（或反向）。
// relParent 为相对该 hive 的父键路径（如 "SOFTWARE\\Classes\\.txt"）。
// 成功搬动返回 true；源不存在返回 false（调用方换 hive 再试）。
bool move_shellnew(HKEY hive, const std::wstring& relParent, bool enable) {
    const wchar_t* srcName = enable ? L"-ShellNew" : L"ShellNew";
    const wchar_t* dstName = enable ? L"ShellNew" : L"-ShellNew";
    std::wstring srcPath = relParent + L"\\" + srcName;
    std::wstring dstPath = relParent + L"\\" + dstName;

    HKEY hProbe = nullptr;
    if (RegOpenKeyExW(hive, srcPath.c_str(), 0, KEY_READ, &hProbe) != ERROR_SUCCESS)
        return false;
    RegCloseKey(hProbe);

    if (!copy_key_tree(hive, srcPath, hive, dstPath)) return false;
    RegDeleteTreeW(hive, srcPath.c_str());
    return true;
}

// 目标位置是否已存在（幂等判定）。
bool shellnew_at_target(HKEY hive, const std::wstring& relParent, bool enable) {
    std::wstring p = relParent + L"\\" + (enable ? L"ShellNew" : L"-ShellNew");
    HKEY h = nullptr;
    if (RegOpenKeyExW(hive, p.c_str(), 0, KEY_READ, &h) == ERROR_SUCCESS) {
        RegCloseKey(h);
        return true;
    }
    return false;
}

// 读某扩展名的默认 ProgID（HKCR\.ext 默认值）。
std::wstring ext_progid(const std::wstring& ext) {
    HKEY h = nullptr;
    std::wstring progId;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, ext.c_str(), 0,
                      KEY_QUERY_VALUE, &h) == ERROR_SUCCESS) {
        progId = read_reg_string(h, nullptr);
        RegCloseKey(h);
    }
    return progId;
}

// 判断某扩展名的 ShellNew 挂在哪：直接挂 .ext 下、挂 ProgID 下、或（停用镜像）-ShellNew。
// 返回相对 .ext（HKCR）的父键子路径：""（直接）或 ProgID 名（挂 ProgID）。
// found=是否找到（含启用/停用镜像）；enabledState=当前是否启用。
// 只探测「直接挂 .ext 下」与「挂默认 ProgID 下」两种最常见位置。
struct ShellNewLoc {
    bool found = false;
    bool enabled = false;
    bool underProgId = false;   // true=挂 ProgID 下，false=直接挂 .ext 下
    std::wstring progId;
};

ShellNewLoc locate_shellnew(const std::wstring& ext) {
    ShellNewLoc loc;
    HKEY hExt = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, ext.c_str(), 0, KEY_READ, &hExt)
        != ERROR_SUCCESS)
        return loc;
    // 直接挂 .ext 下？
    if (subkey_exists(hExt, L"ShellNew")) {
        loc.found = true; loc.enabled = true; loc.underProgId = false;
    } else if (subkey_exists(hExt, L"-ShellNew")) {
        loc.found = true; loc.enabled = false; loc.underProgId = false;
    }
    RegCloseKey(hExt);
    if (loc.found) return loc;

    // 挂默认 ProgID 下？
    std::wstring progId = ext_progid(ext);
    if (!progId.empty()) {
        HKEY hp = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, progId.c_str(), 0, KEY_READ, &hp)
            == ERROR_SUCCESS) {
            if (subkey_exists(hp, L"ShellNew")) {
                loc.found = true; loc.enabled = true;
                loc.underProgId = true; loc.progId = progId;
            } else if (subkey_exists(hp, L"-ShellNew")) {
                loc.found = true; loc.enabled = false;
                loc.underProgId = true; loc.progId = progId;
            }
            RegCloseKey(hp);
        }
    }
    return loc;
}

}  // namespace

std::vector<ShellNewItem> scan_shellnew() {
    std::vector<ShellNewItem> items;

    HKEY hRoot = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"", 0, KEY_ENUMERATE_SUB_KEYS, &hRoot)
        != ERROR_SUCCESS)
        return items;

    DWORD i = 0;
    wchar_t name[256];
    for (;;) {
        DWORD cch = ARRAYSIZE(name);
        LSTATUS st = RegEnumKeyExW(hRoot, i, name, &cch, nullptr, nullptr,
                                   nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        ++i;
        if (st != ERROR_SUCCESS) continue;
        std::wstring ext = name;
        if (ext.empty() || ext[0] != L'.') continue;   // 只看扩展名键

        ShellNewLoc loc = locate_shellnew(ext);
        if (!loc.found) continue;

        ShellNewItem item;
        item.ext = wide_to_utf8(ext);
        item.id = item.ext;   // 直接挂：id=".ext"
        item.enabled = loc.enabled;

        std::wstring progId = loc.underProgId ? loc.progId : ext_progid(ext);
        std::wstring friendly = friendly_type_name(ext, progId);
        std::wstring label = friendly.empty()
            ? (ext + L" 文件")   // 无友好名回落（前端会用 label 直接显示；此处仅兜底）
            : friendly;
        // 显示名统一带上扩展名，便于区分（如「文本文档 (.txt)」）。
        item.label = wide_to_utf8(label + L" (" + ext + L")");

        std::wstring icon = progid_icon(progId);
        if (!icon.empty()) item.iconSource = wide_to_utf8(icon);

        items.push_back(std::move(item));
    }
    RegCloseKey(hRoot);

    std::sort(items.begin(), items.end(),
              [](const ShellNewItem& a, const ShellNewItem& b) {
                  return a.label < b.label;
              });
    return items;
}

std::string scan_shellnew_json() {
    std::vector<ShellNewItem> items = scan_shellnew();
    std::string out = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        const ShellNewItem& it = items[i];
        if (i) out += ",";
        out += "{\"id\":\"";        out += json_escape(it.id);         out += "\"";
        out += ",\"ext\":\"";       out += json_escape(it.ext);        out += "\"";
        out += ",\"label\":\"";     out += json_escape(it.label);      out += "\"";
        out += ",\"iconSource\":\"";out += json_escape(it.iconSource); out += "\"";
        out += ",\"enabled\":";     out += it.enabled ? "true" : "false";
        out += "}";
    }
    out += "]";
    return out;
}

std::string set_shellnew_enabled(const std::string& id, bool enabled) {
    std::wstring ext = utf8_to_wide(id);
    if (ext.empty() || ext[0] != L'.') return result_json(false, "bad_id");
    if (ext.find(L"..") != std::wstring::npos ||
        ext.find(L'/') != std::wstring::npos)
        return result_json(false, "bad_id");

    // 定位 ShellNew 挂在 .ext 直接下还是 ProgID 下。
    ShellNewLoc loc = locate_shellnew(ext);
    if (!loc.found) return result_json(false, "not_found");

    // 相对各 hive 的父键子路径。ShellNew 挂 .ext 下 -> "Classes\\.ext"；
    // 挂 ProgID 下 -> "Classes\\<ProgID>"。两 hive 大小写不敏感，统一写法。
    std::wstring relTail = loc.underProgId ? loc.progId : ext;
    std::wstring relHklm = L"SOFTWARE\\Classes\\" + relTail;
    std::wstring relHkcu = L"Software\\Classes\\" + relTail;

    // 两 hive 都尝试搬（同 handler 搬家：合并视图下项可能在任一 hive）。
    bool movedHklm = move_shellnew(HKEY_LOCAL_MACHINE, relHklm, enabled);
    bool movedHkcu = move_shellnew(HKEY_CURRENT_USER, relHkcu, enabled);
    if (!movedHklm && !movedHkcu) {
        // 未搬动：可能已在目标态（幂等）或真找不到。
        if (shellnew_at_target(HKEY_LOCAL_MACHINE, relHklm, enabled) ||
            shellnew_at_target(HKEY_CURRENT_USER, relHkcu, enabled))
            return result_json(true, "ok");
        return result_json(false, "registry_failed");
    }
    return result_json(true, "ok");
}

}  // namespace cmm
