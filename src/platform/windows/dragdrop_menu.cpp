// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 拖放处理程序（DragDropHandlers）管理（实现）
//
//  聚合枚举 Directory / Folder / Drive / AllFilesystemObjects 四个对象类的
//  shellex\DragDropHandlers（启用）与 -DragDropHandlers（停用镜像）。启停 = 子键
//  整树在两根间搬家（复用 ContextMenuHandlers 同套思路，两 hive 都尝试搬）。
//  label/icon 从 CLSID 简化解析（DragDrop 项通常寥寥几个系统项，不上 menu_scanner
//  那套三档兜底）。
// ============================================================
#include "platform/windows/dragdrop_menu.h"

#include <windows.h>
#include <shlwapi.h>     // SHLoadIndirectString
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace cmm {

namespace {

// —— UTF-8 <-> UTF-16（本模块自持，避免跨 TU 依赖）——
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

// 展开 %var%（混合大小写会让 SHLoadIndirectString 失败，先展开）。
std::wstring expand_env(const std::wstring& s) {
    if (s.find(L'%') == std::wstring::npos) return s;
    wchar_t buf[1024] = {0};
    DWORD n = ExpandEnvironmentStringsW(s.c_str(), buf, ARRAYSIZE(buf));
    if (n == 0 || n > ARRAYSIZE(buf)) return s;
    return buf;
}

// 解 @dll,-id 间接资源串（简化版：预展开 %var% 再 SHLoadIndirectString）。
std::wstring resolve_indirect(const std::wstring& raw) {
    if (raw.empty() || raw[0] != L'@') return raw;
    std::wstring expanded = expand_env(raw);
    wchar_t out[512] = {0};
    if (SUCCEEDED(SHLoadIndirectString(expanded.c_str(), out, ARRAYSIZE(out), nullptr))
        && out[0] != L'\0')
        return out;
    return raw;
}

// 从键名/CLSID 推显示名：优先 CLSID 的 LocalizedString / 默认值，退回键名。
std::wstring resolve_label(const std::wstring& name, const std::wstring& clsid) {
    std::wstring candidate = (!name.empty() && name[0] == L'{') ? name
                           : (!clsid.empty() && clsid[0] == L'{') ? clsid
                           : L"";
    std::wstring label;
    if (!candidate.empty()) {
        std::wstring clsidKey = L"CLSID\\" + candidate;
        HKEY hClsid = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, clsidKey.c_str(), 0,
                          KEY_QUERY_VALUE, &hClsid) == ERROR_SUCCESS) {
            std::wstring loc = read_reg_string(hClsid, L"LocalizedString");
            if (!loc.empty()) {
                std::wstring r = resolve_indirect(loc);
                if (!r.empty() && r[0] != L'@') label = r;
            }
            if (label.empty()) {
                std::wstring def = read_reg_string(hClsid, nullptr);
                if (!def.empty()) {
                    if (def[0] != L'@') label = def;
                    else {
                        std::wstring r = resolve_indirect(def);
                        if (!r.empty() && r[0] != L'@') label = r;
                    }
                }
            }
            RegCloseKey(hClsid);
        }
    }
    // 键名非 CLSID 形态则用键名；否则兜底 "COM Extension"。
    if (label.empty() && (name.empty() || name[0] != L'{')) label = name;
    if (label.empty() || (label.size() >= 2 && label[0] == L'{')) label = L"COM Extension";
    // 去尾部 " Class" COM 噪音。
    const std::wstring suffix = L" Class";
    if (label.size() > suffix.size() &&
        CompareStringOrdinal(label.c_str() + (label.size() - suffix.size()),
                             (int)suffix.size(), suffix.c_str(),
                             (int)suffix.size(), TRUE) == CSTR_EQUAL)
        label = label.substr(0, label.size() - suffix.size());
    return label;
}

// 从 CLSID 取图标源（DefaultIcon / InprocServer32 DLL，交前端 extract_icon 提取）。
std::wstring resolve_icon(const std::wstring& clsid) {
    if (clsid.empty() || clsid[0] != L'{') return L"";
    std::wstring diKey = L"CLSID\\" + clsid + L"\\DefaultIcon";
    HKEY hDi = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, diKey.c_str(), 0,
                      KEY_QUERY_VALUE, &hDi) == ERROR_SUCCESS) {
        std::wstring di = read_reg_string(hDi, nullptr);
        RegCloseKey(hDi);
        if (!di.empty() && di != L"%1") return expand_env(di);
    }
    std::wstring inpKey = L"CLSID\\" + clsid + L"\\InprocServer32";
    HKEY hInp = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, inpKey.c_str(), 0,
                      KEY_QUERY_VALUE, &hInp) == ERROR_SUCCESS) {
        std::wstring dll = read_reg_string(hInp, nullptr);
        RegCloseKey(hInp);
        if (!dll.empty()) return expand_env(dll) + L",0";
    }
    return L"";
}

// 判系统内置：InprocServer32 的 DLL 在 %SystemRoot% 下即系统项。
bool is_system_handler(const std::wstring& clsid) {
    if (clsid.empty() || clsid[0] != L'{') return false;
    std::wstring inpKey = L"CLSID\\" + clsid + L"\\InprocServer32";
    HKEY hInp = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, inpKey.c_str(), 0,
                      KEY_QUERY_VALUE, &hInp) != ERROR_SUCCESS) return false;
    std::wstring dll = expand_env(read_reg_string(hInp, nullptr));
    RegCloseKey(hInp);
    if (dll.empty()) return false;
    wchar_t sysRoot[MAX_PATH] = {0};
    if (GetEnvironmentVariableW(L"SystemRoot", sysRoot, MAX_PATH) == 0) return false;
    std::wstring sr = sysRoot;
    return dll.size() > sr.size() &&
           CompareStringOrdinal(dll.c_str(), (int)sr.size(),
                                sr.c_str(), (int)sr.size(), TRUE) == CSTR_EQUAL;
}

// 四个对象类的 DragDropHandlers 根（相对 HKCR）。
struct ClassRoot { const wchar_t* cls; const wchar_t* root; };
const ClassRoot kRoots[] = {
    { L"Directory",            L"Directory\\shellex\\DragDropHandlers" },
    { L"Folder",               L"Folder\\shellex\\DragDropHandlers" },
    { L"Drive",                L"Drive\\shellex\\DragDropHandlers" },
    { L"AllFilesystemObjects", L"AllFilesystemObjects\\shellex\\DragDropHandlers" },
};

std::vector<std::wstring> enum_subkeys(HKEY hKey) {
    std::vector<std::wstring> out;
    DWORD i = 0;
    wchar_t name[256];
    for (;;) {
        DWORD cch = ARRAYSIZE(name);
        LSTATUS st = RegEnumKeyExW(hKey, i, name, &cch, nullptr, nullptr, nullptr, nullptr);
        if (st != ERROR_SUCCESS) break;
        out.push_back(name);
        ++i;
    }
    return out;
}

// 递归复制注册表子树（子键+值）。
bool copy_key_tree(HKEY hive, const std::wstring& srcPath, const std::wstring& dstPath) {
    HKEY hSrc = nullptr;
    if (RegOpenKeyExW(hive, srcPath.c_str(), 0, KEY_READ, &hSrc) != ERROR_SUCCESS)
        return false;
    HKEY hDst = nullptr;
    if (RegCreateKeyExW(hive, dstPath.c_str(), 0, nullptr, 0,
                        KEY_WRITE, nullptr, &hDst, nullptr) != ERROR_SUCCESS) {
        RegCloseKey(hSrc);
        return false;
    }
    // 复制值。
    DWORD idx = 0;
    wchar_t valName[16384];
    std::vector<BYTE> data(8192);
    for (;;) {
        DWORD cchName = ARRAYSIZE(valName);
        DWORD cbData = (DWORD)data.size();
        DWORD type = 0;
        LSTATUS st = RegEnumValueW(hSrc, idx, valName, &cchName, nullptr,
                                   &type, data.data(), &cbData);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st == ERROR_SUCCESS)
            RegSetValueExW(hDst, valName, 0, type, data.data(), cbData);
        ++idx;
    }
    // 递归子键。
    idx = 0;
    wchar_t subName[256];
    for (;;) {
        DWORD cch = ARRAYSIZE(subName);
        LSTATUS st = RegEnumKeyExW(hSrc, idx, subName, &cch, nullptr, nullptr, nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st == ERROR_SUCCESS)
            copy_key_tree(hive, srcPath + L"\\" + subName, dstPath + L"\\" + subName);
        ++idx;
    }
    RegCloseKey(hDst);
    RegCloseKey(hSrc);
    return true;
}

// 从对象类根推启用根 / 停用根（相对 Software\Classes）。
std::wstring class_root_for(const std::wstring& cls) {
    for (const ClassRoot& r : kRoots)
        if (cls == r.cls) return r.root;
    return L"";
}

// 在某 hive 下把 handler 在 DragDropHandlers <-> -DragDropHandlers 间搬。
bool move_ddh(HKEY hive, const std::wstring& cls, const std::wstring& name, bool enable) {
    std::wstring enRoot = class_root_for(cls);
    if (enRoot.empty()) return false;
    std::wstring disRoot = enRoot;
    size_t pos = disRoot.rfind(L"DragDropHandlers");
    if (pos == std::wstring::npos) return false;
    disRoot.insert(pos, L"-");

    std::wstring base = L"Software\\Classes\\";
    std::wstring srcPath = base + (enable ? disRoot : enRoot) + L"\\" + name;
    std::wstring dstPath = base + (enable ? enRoot : disRoot) + L"\\" + name;

    HKEY hProbe = nullptr;
    if (RegOpenKeyExW(hive, srcPath.c_str(), 0, KEY_READ, &hProbe) != ERROR_SUCCESS)
        return false;
    RegCloseKey(hProbe);

    if (!copy_key_tree(hive, srcPath, dstPath)) return false;
    RegDeleteTreeW(hive, srcPath.c_str());
    return true;
}

// 判某 handler 是否已在目标位置（幂等判定，防重复点假失败）。
bool ddh_at_target(HKEY hive, const std::wstring& cls, const std::wstring& name, bool enable) {
    std::wstring enRoot = class_root_for(cls);
    if (enRoot.empty()) return false;
    std::wstring disRoot = enRoot;
    size_t pos = disRoot.rfind(L"DragDropHandlers");
    if (pos == std::wstring::npos) return false;
    disRoot.insert(pos, L"-");
    std::wstring path = L"Software\\Classes\\" + (enable ? enRoot : disRoot) + L"\\" + name;
    HKEY h = nullptr;
    if (RegOpenKeyExW(hive, path.c_str(), 0, KEY_READ, &h) == ERROR_SUCCESS) {
        RegCloseKey(h);
        return true;
    }
    return false;
}

// 读一个 handler 子键为 DragDropItem。
DragDropItem read_ddh(const std::wstring& cls, const std::wstring& rootPath,
                      const std::wstring& name, bool enabled) {
    DragDropItem item;
    item.objectClass = wide_to_utf8(cls);
    item.id = wide_to_utf8(cls + L"\\" + name);
    item.enabled = enabled;

    std::wstring clsid;
    std::wstring handlerPath = rootPath + L"\\" + name;
    HKEY hHand = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, handlerPath.c_str(), 0,
                      KEY_QUERY_VALUE, &hHand) == ERROR_SUCCESS) {
        clsid = read_reg_string(hHand, nullptr);
        RegCloseKey(hHand);
    }
    std::wstring candidate = (!name.empty() && name[0] == L'{') ? name
                           : (!clsid.empty() && clsid[0] == L'{') ? clsid : L"";
    item.clsid = wide_to_utf8(candidate);
    item.label = wide_to_utf8(resolve_label(name, clsid));
    item.system = is_system_handler(candidate);
    std::wstring icon = resolve_icon(candidate);
    if (!icon.empty()) item.iconSource = wide_to_utf8(icon);
    return item;
}

void scan_class(const std::wstring& cls, const std::wstring& enRoot,
                std::vector<DragDropItem>* out) {
    std::wstring disRoot = enRoot;
    size_t pos = disRoot.rfind(L"DragDropHandlers");
    if (pos != std::wstring::npos) disRoot.insert(pos, L"-");

    auto scan_root = [&](const std::wstring& rootPath, bool enabled) {
        HKEY h = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, rootPath.c_str(), 0,
                          KEY_ENUMERATE_SUB_KEYS, &h) != ERROR_SUCCESS) return;
        std::vector<std::wstring> names = enum_subkeys(h);
        RegCloseKey(h);
        for (const std::wstring& name : names)
            out->push_back(read_ddh(cls, rootPath, name, enabled));
    };
    scan_root(enRoot, true);
    if (pos != std::wstring::npos) scan_root(disRoot, false);
}

}  // namespace

std::vector<DragDropItem> scan_dragdrop() {
    std::vector<DragDropItem> items;
    for (const ClassRoot& r : kRoots)
        scan_class(r.cls, r.root, &items);
    return items;
}

std::string scan_dragdrop_json() {
    std::vector<DragDropItem> items = scan_dragdrop();
    std::string out = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        const DragDropItem& it = items[i];
        if (i) out += ",";
        out += "{\"id\":\"";         out += json_escape(it.id);          out += "\"";
        out += ",\"label\":\"";      out += json_escape(it.label);       out += "\"";
        out += ",\"objectClass\":\"";out += json_escape(it.objectClass); out += "\"";
        out += ",\"clsid\":\"";      out += json_escape(it.clsid);       out += "\"";
        out += ",\"iconSource\":\""; out += json_escape(it.iconSource);  out += "\"";
        out += ",\"enabled\":";      out += it.enabled ? "true" : "false";
        out += ",\"system\":";       out += it.system ? "true" : "false";
        out += ",\"kind\":\"handler\"";
        out += "}";
    }
    out += "]";
    return out;
}

std::string set_dragdrop_enabled(const std::string& id, bool enabled) {
    // id = "<类>\\<name>"。拆出类与 handler 名。
    std::wstring wid = utf8_to_wide(id);
    if (wid.find(L"..") != std::wstring::npos) return result_json(false, "bad_id");
    size_t bs = wid.find(L'\\');
    if (bs == std::wstring::npos) return result_json(false, "bad_id");
    std::wstring cls = wid.substr(0, bs);
    std::wstring name = wid.substr(bs + 1);
    if (cls.empty() || name.empty()) return result_json(false, "bad_id");
    if (class_root_for(cls).empty()) return result_json(false, "bad_class");

    // 两 hive 都尝试搬（同 ContextMenuHandlers：合并视图可能双 hive 都有）。
    bool movedHklm = move_ddh(HKEY_LOCAL_MACHINE, cls, name, enabled);
    bool movedHkcu = move_ddh(HKEY_CURRENT_USER, cls, name, enabled);
    if (!movedHklm && !movedHkcu) {
        // 已在目标态视为幂等成功，防重复点假失败。
        if (ddh_at_target(HKEY_LOCAL_MACHINE, cls, name, enabled) ||
            ddh_at_target(HKEY_CURRENT_USER, cls, name, enabled))
            return result_json(true, "ok");
        return result_json(false, "registry_failed");
    }
    return result_json(true, "ok");
}

}  // namespace cmm
