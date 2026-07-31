// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 「此电脑」盘符图标管理（实现）
//
//  管理 MyComputer\NameSpace 下的第三方 COM 命名空间扩展图标。
//  扫描 HKLM + HKCU 两处（含启用/停用镜像），支持一键删除与启停搬家。
//  写前自动备份；删键前 reg export 对应 CLSID 子树到本地备份目录。
// ============================================================
#include "platform/windows/drive_icons.h"

#include <windows.h>
#include <shlwapi.h>   // SHLoadIndirectString
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")

namespace cmm {

namespace {

// —— UTF-8 <-> UTF-16 ——
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

// JSON 字符串转义。
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

// 读注册表某键下的某值为宽字符串。失败返回 false。
bool reg_read_string(HKEY hKey, const wchar_t* valueName, std::wstring* out) {
    DWORD type = 0, cb = 0;
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

// 展开环境变量。
std::wstring expand_env(const std::wstring& in) {
    if (in.empty()) return in;
    DWORD need = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
    if (need == 0) return in;
    std::wstring out(need, L'\0');
    DWORD got = ExpandEnvironmentStringsW(in.c_str(), out.data(), need);
    if (got == 0) return in;
    out.resize(got > 0 ? got - 1 : 0);
    return out;
}

// 解析间接字符串资源 "@dll,-id"。
std::wstring resolve_indirect_string(const std::wstring& spec) {
    if (spec.empty()) return L"";
    if (spec[0] != L'@') return spec;
    std::wstring expanded = spec;
    if (expanded.find(L'%') != std::wstring::npos) {
        std::wstring rest = expanded.substr(1);
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

// 清洗显示名：去零宽字符、trim、去尾部 " Class"。
std::wstring clean_label(std::wstring s) {
    std::wstring t;
    t.reserve(s.size());
    for (wchar_t c : s) {
        if (c == 0x200B || c == 0xFEFF) continue;
        t += c;
    }
    s.swap(t);
    const wchar_t* ws = L" \t\r\n　";
    size_t b = s.find_first_not_of(ws);
    if (b == std::wstring::npos) return L"";
    size_t e = s.find_last_not_of(ws);
    s = s.substr(b, e - b + 1);
    static const std::wstring kClassSuffix = L" Class";
    if (s.size() > kClassSuffix.size() &&
        s.compare(s.size() - kClassSuffix.size(), kClassSuffix.size(), kClassSuffix) == 0) {
        std::wstring trimmed = s.substr(0, s.size() - kClassSuffix.size());
        size_t te = trimmed.find_last_not_of(ws);
        if (te != std::wstring::npos) s = trimmed.substr(0, te + 1);
    }
    return s;
}

// 取 SystemRoot 小写。
std::wstring sysroot_lower() {
    wchar_t buf[MAX_PATH] = {0};
    DWORD got = GetEnvironmentVariableW(L"SystemRoot", buf, MAX_PATH);
    if (got == 0 || got >= MAX_PATH) return L"";
    std::wstring s(buf);
    for (wchar_t& c : s) c = (wchar_t)towlower(c);
    return s;
}

// 判定 DLL 是否在系统目录下。
bool is_system_dll(const std::wstring& dllPath) {
    if (dllPath.empty()) return false;
    std::wstring lower = dllPath;
    for (wchar_t& c : lower) c = (wchar_t)towlower(c);
    std::wstring sr = sysroot_lower();
    if (sr.empty()) return false;
    if (sr.back() != L'\\') sr += L'\\';
    return lower.size() > sr.size() && lower.compare(0, sr.size(), sr) == 0;
}

// 取备份目录 %LOCALAPPDATA%\ContextMenuMaster\backups\。
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

// 生成时间戳 yyyyMMdd_HHmmss。
std::wstring timestamp_now() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[32];
    wsprintfW(buf, L"%04u%02u%02u_%02u%02u%02u",
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}

// reg export 备份 CLSID 子树。
bool do_backup(const std::wstring& clsid) {
    std::wstring dir = ensure_backup_dir();
    if (dir.empty()) return false;

    std::wstring file = dir + L"\\driveicon_" + clsid + L"_" + timestamp_now() + L".reg";
    std::wstring regRoot = L"HKCR\\CLSID\\" + clsid;

    wchar_t sysDir[MAX_PATH] = {0};
    if (GetSystemDirectoryW(sysDir, MAX_PATH) == 0) return false;
    std::wstring regExe = std::wstring(sysDir) + L"\\reg.exe";
    std::wstring cmd = L"\"" + regExe + L"\" export \"" + regRoot +
                       L"\" \"" + file + L"\" /y";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};
    std::wstring mutableCmd = cmd;

    BOOL ok = CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (!ok) return false;

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
    return exitCode == 0;
}

// 枚举某注册表键下的全部子键名。
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

// 从 CLSID 解析一个驱动图标的显示名与图标源。
// 解析顺序：LocalizedString → 默认值 → CLSID 字符串兜底。
void resolve_clsid_info(const std::wstring& clsid,
                        std::wstring* outLabel,
                        std::wstring* outIcon,
                        std::wstring* outDll) {
    std::wstring clsidKey = L"CLSID\\" + clsid;
    HKEY hClsid = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, clsidKey.c_str(), 0,
                      KEY_QUERY_VALUE, &hClsid) != ERROR_SUCCESS) {
        *outLabel = clsid;
        return;
    }

    // 显示名：LocalizedString 优先。
    std::wstring label;
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
    if (label.empty()) label = clsid;
    *outLabel = clean_label(label);

    // 图标源：DefaultIcon。
    std::wstring diKey = clsidKey + L"\\DefaultIcon";
    HKEY hDi = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, diKey.c_str(), 0,
                      KEY_QUERY_VALUE, &hDi) == ERROR_SUCCESS) {
        std::wstring di;
        if (reg_read_string(hDi, nullptr, &di) && !di.empty() && di != L"%1")
            *outIcon = expand_env(di);
        RegCloseKey(hDi);
    }
    // 兜底：InprocServer32 DLL 路径,0。
    if (outIcon->empty()) {
        std::wstring inpKey = clsidKey + L"\\InprocServer32";
        HKEY hInp = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, inpKey.c_str(), 0,
                          KEY_QUERY_VALUE, &hInp) == ERROR_SUCCESS) {
            std::wstring dll;
            if (reg_read_string(hInp, nullptr, &dll) && !dll.empty()) {
                if (outDll) *outDll = expand_env(dll);
                *outIcon = expand_env(dll) + L",0";
            }
            RegCloseKey(hInp);
        }
    }

    // DLL 路径（单独读一次以防 Icon 分支没取到）。
    if (outDll && outDll->empty()) {
        std::wstring inpKey = clsidKey + L"\\InprocServer32";
        HKEY hInp = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, inpKey.c_str(), 0,
                          KEY_QUERY_VALUE, &hInp) == ERROR_SUCCESS) {
            std::wstring dll;
            if (reg_read_string(hInp, nullptr, &dll) && !dll.empty())
                *outDll = expand_env(dll);
            RegCloseKey(hInp);
        }
    }

    RegCloseKey(hClsid);
}

// MyComputer\NameSpace 的注册表路径（相对 SOFTWARE）。
const wchar_t* kNameSpacePath =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\NameSpace";

// 扫一个 hive（HKLM 或 HKCU）下 NameSpace（及 -NameSpace）的所有 CLSID。
void scan_hive(HKEY hive, bool isHKLM, std::vector<DriveIconItem>& items) {
    // 启用位置。
    HKEY hNs = nullptr;
    if (RegOpenKeyExW(hive, kNameSpacePath, 0, KEY_ENUMERATE_SUB_KEYS,
                      &hNs) == ERROR_SUCCESS) {
        std::vector<std::wstring> clsidList = enum_subkeys(hNs);
        RegCloseKey(hNs);
        for (const std::wstring& clsid : clsidList) {
            DriveIconItem item;
            item.id = wide_to_utf8(clsid);
            item.fromHKLM = isHKLM;
            item.enabled = true;
            std::wstring label, icon, dll;
            resolve_clsid_info(clsid, &label, &icon, &dll);
            item.label = wide_to_utf8(label);
            item.iconSource = wide_to_utf8(icon);
            item.dllPath = wide_to_utf8(dll);
            item.systemItem = is_system_dll(dll);
            items.push_back(item);
        }
    }

    // 停用镜像（-NameSpace）。
    std::wstring disPath = kNameSpacePath;
    size_t pos = disPath.rfind(L"NameSpace");
    if (pos != std::wstring::npos) disPath.insert(pos, L"-");
    HKEY hDis = nullptr;
    if (RegOpenKeyExW(hive, disPath.c_str(), 0, KEY_ENUMERATE_SUB_KEYS,
                      &hDis) == ERROR_SUCCESS) {
        std::vector<std::wstring> clsidList = enum_subkeys(hDis);
        RegCloseKey(hDis);
        for (const std::wstring& clsid : clsidList) {
            // 已在启用列表里的跳过（两处都有的罕见情况，启用态优先）。
            std::string id = wide_to_utf8(clsid);
            bool dup = false;
            for (const DriveIconItem& it : items) {
                if (it.id == id && it.fromHKLM == isHKLM) { dup = true; break; }
            }
            if (dup) continue;
            DriveIconItem item;
            item.id = id;
            item.fromHKLM = isHKLM;
            item.enabled = false;
            std::wstring label, icon, dll;
            resolve_clsid_info(clsid, &label, &icon, &dll);
            item.label = wide_to_utf8(label);
            item.iconSource = wide_to_utf8(icon);
            item.dllPath = wide_to_utf8(dll);
            item.systemItem = is_system_dll(dll);
            items.push_back(item);
        }
    }
}

}  // namespace

std::vector<DriveIconItem> scan_drive_icons() {
    std::vector<DriveIconItem> items;
    scan_hive(HKEY_LOCAL_MACHINE, true, items);
    scan_hive(HKEY_CURRENT_USER, false, items);
    // 排序：系统项在前，按 label 排。
    std::sort(items.begin(), items.end(),
              [](const DriveIconItem& a, const DriveIconItem& b) {
                  if (a.systemItem != b.systemItem) return a.systemItem;
                  return a.label < b.label;
              });
    return items;
}

std::string scan_drive_icons_json() {
    std::vector<DriveIconItem> items = scan_drive_icons();
    std::string out = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        const DriveIconItem& it = items[i];
        if (i) out += ",";
        out += "{\"id\":\"";         out += json_escape(it.id);          out += "\"";
        out += ",\"label\":\"";      out += json_escape(it.label);       out += "\"";
        out += ",\"iconSource\":\""; out += json_escape(it.iconSource);  out += "\"";
        out += ",\"dllPath\":\"";    out += json_escape(it.dllPath);     out += "\"";
        out += ",\"system\":";       out += it.systemItem ? "true" : "false";
        out += ",\"fromHKLM\":";     out += it.fromHKLM ? "true" : "false";
        out += ",\"enabled\":";      out += it.enabled ? "true" : "false";
        out += "}";
    }
    out += "]";
    return out;
}

std::string set_drive_icon_enabled(const std::string& id, bool enabled) {
    // id 必须是 CLSID 格式的 GUID（含花括号，不含路径分隔符等危险字符）。
    std::wstring clsid = utf8_to_wide(id);
    if (clsid.empty() || clsid.front() != L'{' || clsid.back() != L'}')
        return result_json(false, "bad_id");
    if (clsid.find(L'\\') != std::wstring::npos ||
        clsid.find(L'/') != std::wstring::npos ||
        clsid.find(L"..") != std::wstring::npos)
        return result_json(false, "bad_id");

    // 先备份 CLSID 注册信息。
    do_backup(clsid);

    // 构造启用根和停用根路径。
    std::wstring enPath = std::wstring(kNameSpacePath) + L"\\" + clsid;
    std::wstring disPath = kNameSpacePath;
    size_t pos = disPath.rfind(L"NameSpace");
    if (pos == std::wstring::npos) return result_json(false, "registry_failed");
    disPath.insert(pos, L"-");
    disPath += L"\\";
    disPath += clsid;

    std::wstring srcPath = enabled ? disPath : enPath;
    std::wstring dstPath = enabled ? enPath : disPath;

    // 尝试在 HKLM 和 HKCU 里都找。
    bool moved = false;
    for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
        // 源必须存在。
        HKEY hProbe = nullptr;
        if (RegOpenKeyExW(hive, srcPath.c_str(), 0, KEY_READ, &hProbe) != ERROR_SUCCESS)
            continue;
        RegCloseKey(hProbe);

        // 复制到目标。
        HKEY hSrc = nullptr;
        HKEY hDst = nullptr;
        DWORD disp = 0;
        if (RegOpenKeyExW(hive, srcPath.c_str(), 0, KEY_READ, &hSrc) != ERROR_SUCCESS)
            continue;
        if (RegCreateKeyExW(hive, dstPath.c_str(), 0, nullptr,
                            REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                            &hDst, &disp) != ERROR_SUCCESS) {
            RegCloseKey(hSrc);
            continue;
        }
        // 复制值。
        bool ok = true;
        for (DWORD i = 0; ok; ++i) {
            DWORD cchName = 0, cbData = 0, type = 0;
            LSTATUS st = RegEnumValueW(hSrc, i, nullptr, &cchName, nullptr,
                                       &type, nullptr, &cbData);
            if (st == ERROR_NO_MORE_ITEMS) break;
            if (st != ERROR_SUCCESS && st != ERROR_MORE_DATA) { ok = false; break; }
            std::vector<wchar_t> name(cchName + 1);
            std::vector<BYTE> data(cbData ? cbData : 1);
            DWORD cchName2 = cchName + 1, cbData2 = cbData;
            st = RegEnumValueW(hSrc, i, name.data(), &cchName2, nullptr, &type,
                               data.data(), &cbData2);
            if (st != ERROR_SUCCESS) { ok = false; break; }
            if (RegSetValueExW(hDst, name.data(), 0, type, data.data(),
                               cbData2) != ERROR_SUCCESS) { ok = false; break; }
        }
        RegCloseKey(hDst);
        RegCloseKey(hSrc);
        if (!ok) continue;
        // 删源。须检查返回值：删源失败不应报成功，否则项留在原位。
        LSTATUS delSt = RegDeleteTreeW(hive, srcPath.c_str());
        if (delSt != ERROR_SUCCESS) continue;
        moved = true;
    }

    if (!moved) {
        // 可能是已在目标态（幂等）。
        for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
            HKEY h = nullptr;
            if (RegOpenKeyExW(hive, dstPath.c_str(), 0, KEY_READ, &h) == ERROR_SUCCESS) {
                RegCloseKey(h);
                return result_json(true, "already_done");
            }
        }
        return result_json(false, "registry_failed");
    }
    return result_json(true, "ok");
}

std::string del_drive_icon(const std::string& id) {
    std::wstring clsid = utf8_to_wide(id);
    if (clsid.empty() || clsid.front() != L'{' || clsid.back() != L'}')
        return result_json(false, "bad_id");
    if (clsid.find(L'\\') != std::wstring::npos ||
        clsid.find(L'/') != std::wstring::npos ||
        clsid.find(L"..") != std::wstring::npos)
        return result_json(false, "bad_id");

    // 写前备份 CLSID 注册信息。备份失败拒绝删除，确保可恢复。
    if (!do_backup(clsid)) return result_json(false, "backup_failed");

    // 尝试在 HKLM 和 HKCU 的 NameSpace 和 -NameSpace 里删除。
    bool deleted = false;
    for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
        for (const wchar_t* suffix : { L"NameSpace\\", L"-NameSpace\\" }) {
            std::wstring keyPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                                   L"Explorer\\MyComputer\\";
            keyPath += suffix;
            keyPath += clsid;
            LSTATUS st = RegDeleteTreeW(hive, keyPath.c_str());
            if (st == ERROR_SUCCESS) deleted = true;
        }
    }

    if (!deleted) return result_json(false, "not_found");
    return result_json(true, "ok");
}

}  // namespace cmm
