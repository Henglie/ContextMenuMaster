// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 「此电脑」仿真管理（实现）
//
//  把「此电脑」界面 1:1 仿真：真实磁盘（图标 + 容量）与 MyComputer\NameSpace
//  下的第三方 COM 命名空间扩展图标混合渲染。磁盘只读；ns 项可启停搬家
//  （NameSpace ↔ -NameSpace）、可一键删除（删前 reg export 备份）。
// ============================================================
#include "platform/windows/drive_icons.h"
#include "platform/windows/icon_extract.h"   // extract_hicon_data_uri（真实磁盘图标）

#include <windows.h>
#include <shlwapi.h>      // SHLoadIndirectString
#include <shlobj.h>       // SHChangeNotify / SHGetKnownFolderPath
#include <knownfolders.h> // KNOWNFOLDERID
#include <shellapi.h>     // SHGetFileInfoW
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
std::wstring ensure_backup_dir() {    wchar_t base[MAX_PATH] = {0};
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

// 本地变体 CLSID → 对应 KnownFolder（FOLDERID）。
// 「此电脑」NameSpace 里的系统文件夹分两类：真 FOLDERID（{374DE290} 下载、
// {B4BFCC3A} 桌面…）与 CLSID_ThisPCLocal*RegFolder 本地变体（{088e3905} 本地下载、
// {24ad3ad4} 本地文档…）。本地变体不在 FolderDescriptions、也不是 KnownFolder，
// 显示名 = 对应 FOLDERID 的本地化名，故硬编码映射后统一走 KnownFolder API。
static const struct { const wchar_t* clsid; const KNOWNFOLDERID* fid; } kLocalMap[] = {
    { L"{088e3905-0323-4b02-9826-5d99428e115f}", &FOLDERID_Downloads },
    { L"{374DE290-123F-4565-9164-39C4925E467B}", &FOLDERID_Downloads },
    { L"{24ad3ad4-a569-4530-98e1-ab02f9417aa8}", &FOLDERID_Documents },
    { L"{a8cdff1c-4878-43be-b5fd-f8091c1c60d0}", &FOLDERID_Documents },
    { L"{3dfdfced-d30a-4a11-9d63-8321e6d5c7f1}", &FOLDERID_Pictures },
    { L"{3add1653-eb32-4cb0-bbd7-dfa0abb5acca}", &FOLDERID_Pictures },
    { L"{d3162b92-9365-467a-824b-07307d810a95}", &FOLDERID_Videos },
    { L"{a0953c92-50dc-43bf-be83-3742fed03c9c}", &FOLDERID_Videos },
    { L"{1cf1260c-4dd0-4ebb-811f-33c572699fde}", &FOLDERID_Music },
    { L"{b4bfcc3a-db2c-424c-b029-7fe99a87c641}", &FOLDERID_Desktop },
};

// 取 KnownFolder 的本地化显示名（「下载」「文档」…随系统语言）。
// SHGetKnownFolderPath 拿真实路径 → SHGetFileInfoW(DISPLAYNAME) 取 shell 名。
std::wstring known_folder_display_name(const KNOWNFOLDERID* fid) {
    if (!fid) return L"";
    wchar_t* path = nullptr;
    if (FAILED(SHGetKnownFolderPath(*fid, 0, nullptr, &path))) return L"";
    std::wstring name;
    SHFILEINFOW fi = {0};
    if (SHGetFileInfoW(path, FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi),
                       SHGFI_DISPLAYNAME) && fi.szDisplayName[0] != L'\0') {
        name = fi.szDisplayName;
    }
    CoTaskMemFree(path);
    return name;
}

// 从 CLSID 解析一个驱动图标的显示名与图标源。
// 解析顺序：硬编码 FOLDERID 映射 → HKCR\CLSID LocalizedString → 默认值 →
// FolderDescriptions LocalizedName → SHCreateItemFromParsingName（shell 自解析
// 任意命名空间对象）→ CLSID 字符串兜底。
void resolve_clsid_info(const std::wstring& clsid,
                        std::wstring* outLabel,
                        std::wstring* outIcon,
                        std::wstring* outDll) {
    std::wstring label;

    // 0) 本地变体映射：直接拿对应 KnownFolder 的本地化名。
    for (const auto& m : kLocalMap) {
        if (CompareStringOrdinal(clsid.c_str(), -1, m.clsid, -1, TRUE) == CSTR_EQUAL) {
            label = known_folder_display_name(m.fid);
            break;
        }
    }

    std::wstring clsidKey = L"CLSID\\" + clsid;
    HKEY hClsid = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, clsidKey.c_str(), 0,
                      KEY_QUERY_VALUE, &hClsid) != ERROR_SUCCESS) {
        if (!label.empty()) { *outLabel = clean_label(label); }
        else { *outLabel = clsid; }
        return;
    }

    // 显示名：LocalizedString 优先。
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
    // 系统内建文件夹（下载/文档/音乐/视频/图片/桌面）的 HKCR\CLSID 常只有
    // DescriptionID/DefaultIcon，显示名在 KnownFolder 注册里 → 查
    // FolderDescriptions\<clsid> 的 LocalizedName（@dll,-id 间接资源）。
    // 注意：FolderDescriptions 的 Name 是英文内部标识（"Downloads"），不是
    // 显示名，故只用 LocalizedName。
    if (label.empty()) {
        std::wstring fdKey =
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
            L"FolderDescriptions\\" + clsid;
        HKEY hFd = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, fdKey.c_str(), 0,
                          KEY_QUERY_VALUE, &hFd) == ERROR_SUCCESS) {
            std::wstring name;
            if (reg_read_string(hFd, L"LocalizedName", &name) && !name.empty()) {
                std::wstring r = resolve_indirect_string(name);
                if (!r.empty() && r[0] != L'@') label = r;
            }
            RegCloseKey(hFd);
        }
    }
    // 兜底：SHCreateItemFromParsingName("::{CLSID}") 让 shell 自己解析
    // 命名空间对象的本地化显示名（对真 FOLDERID 有效）。
    if (label.empty()) {
        std::wstring parse = L"::" + clsid;
        IShellItem* item = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(parse.c_str(), nullptr,
                                                  IID_PPV_ARGS(&item)))) {
            wchar_t* name = nullptr;
            HRESULT hr = item->GetDisplayName(SIGDN_NORMALDISPLAY, &name);
            item->Release();
            if (SUCCEEDED(hr) && name && name[0] != L'\0') {
                // 该 API 有时返回英文名（Music/Documents），本地化失败时
                // 仍比 CLSID 强；若与 CLSID 同形（解析失败回显）则弃用。
                if (CompareStringOrdinal(name, -1, clsid.c_str(), -1, TRUE) != CSTR_EQUAL)
                    label = name;
                CoTaskMemFree(name);
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
            // DelegateFolders 是 NameSpace 下的特殊容器（委托命名空间），不是
            // CLSID 图标项，跳过（否则会解析失败显示成数字）。
            if (CompareStringOrdinal(clsid.c_str(), -1, L"DelegateFolders",
                                     -1, TRUE) == CSTR_EQUAL) continue;
            DriveIconItem item;
            item.kind = "ns";
            item.id = wide_to_utf8(clsid);
            item.fromHKLM = isHKLM;
            item.enabled = true;
            std::wstring label, icon, dll;
            resolve_clsid_info(clsid, &label, &icon, &dll);
            item.label = wide_to_utf8(label);
            item.iconSource = wide_to_utf8(icon);
            item.dllPath = wide_to_utf8(dll);
            item.systemItem = is_system_dll(dll);
            // 解析不出名字的系统项（如 shell32 内部命名空间）标 knownId=system，
            // 前端 i18n 显示「系统组件」，避免数字 CLSID 暴露。
            if (item.systemItem && !label.empty() && label[0] == L'{') {
                item.knownId = "system";
            }
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
            item.kind = "ns";
            item.id = id;
            item.fromHKLM = isHKLM;
            item.enabled = false;
            std::wstring label, icon, dll;
            resolve_clsid_info(clsid, &label, &icon, &dll);
            item.label = wide_to_utf8(label);
            item.iconSource = wide_to_utf8(icon);
            item.dllPath = wide_to_utf8(dll);
            item.systemItem = is_system_dll(dll);
            if (item.systemItem && !label.empty() && label[0] == L'{') {
                item.knownId = "system";
            }
            items.push_back(item);
        }
    }
}

}  // namespace

// —— 真实磁盘枚举 ——
// 磁盘类型 -> SHSTOCKICONID（前端 extract_icon 的 "stock:NN" 写法）。
//   SIID_DRIVEFIXED=145, SIID_DRIVEREMOVABLE=146, SIID_DRIVENET=147,
//   SIID_DRIVECD=148, SIID_DRIVERAM=149
namespace {

// 盘符类型转内部 type 字符串 + stock 图标 id。
void drive_type_to_info(UINT driveType, std::string* type, int* stockIcon) {
    switch (driveType) {
        case DRIVE_REMOVABLE: *type = "removable"; *stockIcon = 146; break;
        case DRIVE_REMOTE:    *type = "network";   *stockIcon = 147; break;
        case DRIVE_CDROM:     *type = "cdrom";     *stockIcon = 148; break;
        case DRIVE_RAMDISK:   *type = "ram";       *stockIcon = 149; break;
        default:              *type = "fixed";     *stockIcon = 145; break;
    }
}

// 取真实磁盘图标：SHGetFileInfoW 按盘符拿系统图标（系统盘带 Windows 徽标、
// 可移动盘是 U 盘造型等），转 PNG data URI 内嵌给前端。失败返回空串。
// SHGFI_USEFILEATTRIBUTES：未就绪的光驱等也能按属性返回通用图标。
std::string drive_icon_data_uri(const std::wstring& root) {
    SHFILEINFOW fi = {0};
    if (!SHGetFileInfoW(root.c_str(), FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi),
                        SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES) ||
        !fi.hIcon) {
        return "";
    }
    std::string uri = cmm::extract_hicon_data_uri(fi.hIcon, 32);
    DestroyIcon(fi.hIcon);
    return uri;
}

// 枚举全部真实磁盘（字母序）。失败盘符跳过（如未就绪的光驱）。
void scan_drives(std::vector<DriveIconItem>& items) {
    DWORD mask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (!(mask & (1u << i))) continue;
        wchar_t root[8];
        wsprintfW(root, L"%c:\\", L'A' + i);
        UINT t = GetDriveTypeW(root);
        // 未知类型（驱动器不存在/未就绪）跳过。
        if (t == DRIVE_UNKNOWN || t == DRIVE_NO_ROOT_DIR) continue;

        std::string type;
        int stockIcon = 145;
        drive_type_to_info(t, &type, &stockIcon);

        DriveIconItem item;
        item.kind = "drive";
        item.letter = wide_to_utf8(std::wstring(1, L'A' + i) + L":");
        item.id = "drive:" + item.letter;
        item.label = item.letter;   // 前端拼「本地磁盘 (C:)」
        item.type = type;
        char buf[32];
        wsprintfA(buf, "stock:%d", stockIcon);
        item.iconSource = buf;
        item.iconData = drive_icon_data_uri(root);

        // 容量。GetDiskFreeSpaceExW 拿 可用字节 + 总字节。
        ULARGE_INTEGER freeAvail = {0}, total = {0}, totalFree = {0};
        if (GetDiskFreeSpaceExW(root, &freeAvail, &total, &totalFree)) {
            item.freeBytes  = freeAvail.QuadPart;
            item.totalBytes = total.QuadPart;
        }
        items.push_back(std::move(item));
    }
}

}  // namespace

std::vector<DriveIconItem> scan_drive_icons() {
    std::vector<DriveIconItem> items;
    scan_drives(items);
    scan_hive(HKEY_LOCAL_MACHINE, true, items);
    scan_hive(HKEY_CURRENT_USER, false, items);
    return items;
}

std::string scan_drive_icons_json() {
    std::vector<DriveIconItem> items = scan_drive_icons();
    std::string out = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        const DriveIconItem& it = items[i];
        if (i) out += ",";
        out += "{\"kind\":\"";       out += json_escape(it.kind);        out += "\"";
        out += ",\"id\":\"";         out += json_escape(it.id);          out += "\"";
        out += ",\"letter\":\"";     out += json_escape(it.letter);      out += "\"";
        out += ",\"type\":\"";       out += json_escape(it.type);        out += "\"";
        out += ",\"label\":\"";      out += json_escape(it.label);       out += "\"";
        out += ",\"iconSource\":\""; out += json_escape(it.iconSource);  out += "\"";
        out += ",\"iconData\":\"";   out += json_escape(it.iconData);    out += "\"";
        out += ",\"knownId\":\"";    out += json_escape(it.knownId);     out += "\"";
        out += ",\"totalBytes\":";   out += std::to_string(it.totalBytes);
        out += ",\"freeBytes\":";    out += std::to_string(it.freeBytes);
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

    // 停用 = 删除 NameSpace 子键（explorer 不再显示）。-NameSpace 镜像对
    // MyComputer 不可靠（实测 explorer 不认），改用删键 + 备份，可还原。
    // 关键：同一 CLSID 可能 HKLM、HKCU 两处都注册（如百度网盘），必须全清，
    // 只清一份合并视图仍显示（「关不掉」根因）。启用 = 重建 NameSpace 子键
    // （默认值 = CLSID，explorer 从 HKCR\CLSID 解析显示名），并清理镜像残留。
    // 写前自动备份 CLSID 注册信息（备份失败拒绝操作，确保可恢复）。
    if (!do_backup(clsid)) return result_json(false, "backup_failed");

    if (enabled) {
        // 启用：重建 NameSpace\{clsid}（写 HKLM，对所有用户生效），清 -NameSpace 残留。
        bool created = false;
        std::wstring enPath = std::wstring(kNameSpacePath) + L"\\" + clsid;
        for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
            HKEY hKey = nullptr;
            DWORD disp = 0;
            if (RegCreateKeyExW(hive, enPath.c_str(), 0, nullptr,
                                REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr,
                                &hKey, &disp) == ERROR_SUCCESS) {
                LSTATUS st = RegSetValueExW(
                    hKey, nullptr, 0, REG_SZ,
                    reinterpret_cast<const BYTE*>(clsid.c_str()),
                    (DWORD)((clsid.size() + 1) * sizeof(wchar_t)));
                RegCloseKey(hKey);
                if (st == ERROR_SUCCESS) created = true;
            }
        }
        if (!created) return result_json(false, "registry_failed");
        // 清理停用镜像残留。
        std::wstring disRoot = kNameSpacePath;
        size_t pos = disRoot.rfind(L"NameSpace");
        if (pos != std::wstring::npos) {
            disRoot.insert(pos, L"-");
            std::wstring disPath = disRoot + L"\\" + clsid;
            for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER })
                RegDeleteTreeW(hive, disPath.c_str());
        }
        return result_json(true, "ok");
    }

    // 停用：删全部 hive 的 NameSpace\{clsid} 与 -NameSpace\{clsid}。
    std::wstring enRoot = kNameSpacePath;
    std::wstring disRoot = kNameSpacePath;
    size_t pos = disRoot.rfind(L"NameSpace");
    if (pos == std::wstring::npos) return result_json(false, "registry_failed");
    disRoot.insert(pos, L"-");

    bool deleted = false;
    for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
        LSTATUS st1 = RegDeleteTreeW(hive, (enRoot + L"\\" + clsid).c_str());
        LSTATUS st2 = RegDeleteTreeW(hive, (disRoot + L"\\" + clsid).c_str());
        if (st1 == ERROR_SUCCESS || st2 == ERROR_SUCCESS) deleted = true;
    }
    if (!deleted) {
        // 已是停用态（键不存在）——幂等成功。
        bool anywhere = false;
        for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
            HKEY h = nullptr;
            if (RegOpenKeyExW(hive, (enRoot + L"\\" + clsid).c_str(), 0,
                              KEY_READ, &h) == ERROR_SUCCESS) {
                RegCloseKey(h);
                anywhere = true;
            }
            if (RegOpenKeyExW(hive, (disRoot + L"\\" + clsid).c_str(), 0,
                              KEY_READ, &h) == ERROR_SUCCESS) {
                RegCloseKey(h);
                anywhere = true;
            }
        }
        return anywhere ? result_json(false, "registry_failed")
                        : result_json(true, "already_done");
    }
    // 通知 shell 重新枚举（NameSpace 变更 explorer 不一定即时刷新）。
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
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
