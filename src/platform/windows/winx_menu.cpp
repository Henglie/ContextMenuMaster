// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— Win+X 菜单管理（实现）
//
//  枚举 %LOCALAPPDATA%\Microsoft\Windows\WinX\Group{1,2,3}\*.lnk，读每个快捷方式
//  的目标路径，支持启停（.lnk <-> .lnk.disabled 改扩展名）。全程宽字符 Win32；
//  返回前端的字符串统一转 UTF-8。hash 计算与新增自定义项见 winx_hash（T7）。
// ============================================================
#include "platform/windows/winx_menu.h"

#include <windows.h>
#include <shlobj.h>      // SHGetKnownFolderPath, IShellLink, SHFileOperationW
#include <shlwapi.h>     // HashData
#include <objbase.h>     // CoCreateInstance
#include <propsys.h>     // IPropertyStore, SHGetPropertyStoreFromParsingName
#include <propvarutil.h> // InitPropVariantFromUInt32
#include <propkey.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "propsys.lib")

namespace cmm {

namespace {

// —— UTF-8 <-> UTF-16（与 menu_scanner 一致，本模块自持一份避免跨 TU 依赖）——
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

// JSON 字符串转义（无 JSON 库，手拼）。
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

// WinX 根目录 %LOCALAPPDATA%\Microsoft\Windows\WinX。取不到返回空。
std::wstring winx_root() {
    wchar_t base[MAX_PATH] = {0};
    DWORD got = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    if (got == 0 || got >= MAX_PATH) return L"";
    return std::wstring(base) + L"\\Microsoft\\Windows\\WinX";
}

// 大小写不敏感判断 s 是否以 suffix 结尾。
bool ends_with_ci(const std::wstring& s, const std::wstring& suffix) {
    if (s.size() < suffix.size()) return false;
    return CompareStringOrdinal(
        s.c_str() + (s.size() - suffix.size()), (int)suffix.size(),
        suffix.c_str(), (int)suffix.size(), TRUE) == CSTR_EQUAL;
}

// 读一个 .lnk 的目标路径。失败返回空串（不阻断枚举，目标仅用于展示）。
// 注意：本函数假定调用线程已 CoInitialize。
std::wstring read_lnk_target(const std::wstring& lnkPath) {
    IShellLinkW* link = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_IShellLinkW, reinterpret_cast<void**>(&link)))) {
        return L"";
    }
    std::wstring target;
    IPersistFile* pf = nullptr;
    if (SUCCEEDED(link->QueryInterface(IID_IPersistFile,
                                       reinterpret_cast<void**>(&pf)))) {
        if (SUCCEEDED(pf->Load(lnkPath.c_str(), STGM_READ))) {
            wchar_t buf[MAX_PATH] = {0};
            // SLGP_RAWPATH 保留 %var%，不强制解析（展示更接近 lnk 原始目标）。
            if (SUCCEEDED(link->GetPath(buf, MAX_PATH, nullptr, SLGP_RAWPATH))
                && buf[0] != L'\0') {
                target = buf;
            }
        }
        pf->Release();
    }
    link->Release();
    return target;
}

// 从文件名推显示名：去 .lnk / .lnk.disabled 扩展名。lnk 常见 "NN - 名称" 命名，
// 前缀 "NN - " 是排序用序号，展示时剥掉更干净（保留原始名做 id，不动文件）。
std::wstring lnk_display_name(const std::wstring& fileName) {
    std::wstring name = fileName;
    if (ends_with_ci(name, L".lnk.disabled"))
        name = name.substr(0, name.size() - 13);  // ".lnk.disabled" 长 13
    else if (ends_with_ci(name, L".lnk"))
        name = name.substr(0, name.size() - 4);
    // 剥前导 "NN - " 序号（形如 "01 - 程序和功能"）。
    size_t dash = name.find(L" - ");
    if (dash != std::wstring::npos && dash <= 3) {
        bool allDigit = dash > 0;
        for (size_t i = 0; i < dash; ++i)
            if (name[i] < L'0' || name[i] > L'9') { allDigit = false; break; }
        if (allDigit) name = name.substr(dash + 3);
    }
    return name;
}

// 枚举某分组文件夹下的项（.lnk 与 .lnk.disabled）。groupRel 如 "Group2"。
void scan_group(const std::wstring& root, const std::wstring& groupDir,
                WinXGroup* out) {
    std::wstring pattern = root + L"\\" + groupDir + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring fname = fd.cFileName;
        bool isLnk = ends_with_ci(fname, L".lnk");
        bool isDisabled = ends_with_ci(fname, L".lnk.disabled");
        if (!isLnk && !isDisabled) continue;

        WinXItem item;
        item.id = wide_to_utf8(groupDir + L"\\" + fname);
        item.enabled = isLnk;
        item.label = wide_to_utf8(lnk_display_name(fname));

        std::wstring full = root + L"\\" + groupDir + L"\\" + fname;
        std::wstring target = read_lnk_target(full);
        if (!target.empty()) {
            item.target = wide_to_utf8(target);
            item.iconSource = item.target;  // 目标 exe 即图标源
        }
        out->items.push_back(std::move(item));
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    // 组内按文件名（含 "NN -" 序号）排，贴近系统渲染顺序。
    std::sort(out->items.begin(), out->items.end(),
              [](const WinXItem& a, const WinXItem& b) { return a.id < b.id; });
}

// —— Win+X hash（移植自参考项目 WinXHasher.cs）——
//
// Win+X 只显示带合法 System.Winx.Hash 属性的 lnk（explorer 校验防用户手动乱塞）。
// 算法：对 (泛化目标路径 + 参数 + 固定盐串) 转小写的 UTF-16 字节跑 shlwapi HashData，
// 取输出前 4 字节为 uint32，写进 lnk 的 System.Winx.Hash（VT_UI4）属性。
//
// 固定盐串必须逐字一致（含中间两个空格），错一个字符 hash 就废、项不显示。
// 泛化路径：把常见系统目录前缀换成微软约定的 GUID（与系统算 hash 时一致）。

// shlwapi 的 HashData（无公开头声明，手动 extern）。对任意字节算固定长度哈希。
extern "C" __declspec(dllimport) int __stdcall HashData(
    const BYTE* pbData, DWORD cbData, BYTE* pbHash, DWORD cbHash);

// 泛化路径映射（大小写不敏感前缀替换）。与 WinXHasher.cs:25-30 一致。
std::wstring generalize_path(const std::wstring& filePath) {
    if (filePath.empty()) return filePath;
    struct Pair { const wchar_t* env; const wchar_t* guid; };
    static const Pair kMap[] = {
        { L"%ProgramFiles%",       L"{905e63b6-c1bf-494e-b29c-65b732d3d21a}" },
        { L"%SystemRoot%\\System32",L"{1ac14e77-02e7-4e5d-b744-2eb1ae5198b7}" },
        { L"%SystemRoot%",         L"{f38bf404-1d43-42f2-9305-67de0b28fc23}" },
    };
    for (const Pair& p : kMap) {
        wchar_t dirBuf[MAX_PATH] = {0};
        DWORD n = ExpandEnvironmentStringsW(p.env, dirBuf, MAX_PATH);
        if (n == 0 || n > MAX_PATH) continue;
        std::wstring dir = dirBuf;
        std::wstring dirSlash = dir + L"\\";
        if (filePath.size() > dirSlash.size() &&
            CompareStringOrdinal(filePath.c_str(), (int)dirSlash.size(),
                                 dirSlash.c_str(), (int)dirSlash.size(), TRUE) == CSTR_EQUAL) {
            return std::wstring(p.guid) + filePath.substr(dir.size());
        }
    }
    return filePath;
}

// 转小写（宽字符）。
std::wstring to_lower_w(std::wstring s) {
    for (wchar_t& c : s) c = (wchar_t)towlower(c);
    return s;
}

// 计算 Win+X hash 并写进 lnk 的 System.Winx.Hash 属性。假定调用线程已 CoInitialize。
// 成功返回 true。targetPath/args 为 lnk 的目标与参数（用于算 hash 的输入）。
bool write_winx_hash(const std::wstring& lnkPath, const std::wstring& targetPath,
                     const std::wstring& args) {
    if (targetPath.empty()) return false;

    // blob = (泛化目标 + 参数 + 固定盐).toLowerInvariant()，UTF-16 字节。
    std::wstring blob = generalize_path(targetPath) + args +
        L"do not prehash links.  this should only be done by the user.";
    blob = to_lower_w(blob);

    const BYTE* input = reinterpret_cast<const BYTE*>(blob.c_str());
    DWORD cb = (DWORD)(blob.size() * sizeof(wchar_t));
    std::vector<BYTE> output(cb ? cb : 1);
    if (HashData(input, cb, output.data(), (DWORD)output.size()) != 0) return false;
    uint32_t hash = 0;
    memcpy(&hash, output.data(), sizeof(hash));

    // 写 System.Winx.Hash（GUID {FB8D2D7B-90D1-4E34-BF60-6EAC09922BBF}, PID 2）。
    // 硬编码 PropertyKey（PSGetPropertyKeyFromName 在部分 Win10 上不可靠，参考项目注释）。
    PROPERTYKEY pkey;
    pkey.fmtid = { 0xFB8D2D7B, 0x90D1, 0x4E34,
                   { 0xBF, 0x60, 0x6E, 0xAC, 0x09, 0x92, 0x2B, 0xBF } };
    pkey.pid = 2;

    IPropertyStore* store = nullptr;
    HRESULT hr = SHGetPropertyStoreFromParsingName(
        lnkPath.c_str(), nullptr, GPS_READWRITE,
        IID_IPropertyStore, reinterpret_cast<void**>(&store));
    if (FAILED(hr) || !store) return false;

    PROPVARIANT pv;
    InitPropVariantFromUInt32(hash, &pv);
    hr = store->SetValue(pkey, pv);
    if (SUCCEEDED(hr)) hr = store->Commit();
    PropVariantClear(&pv);
    store->Release();
    return SUCCEEDED(hr);
}

}  // namespace

std::vector<WinXGroup> scan_winx() {
    std::vector<WinXGroup> groups;
    std::wstring root = winx_root();
    if (root.empty()) return groups;

    // 本模块用 COM 读 lnk，整个扫描期开一次 COM。
    bool comInit = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

    // 枚举分组文件夹（Group1/2/3…），倒序（Group3 在前，与菜单一致）。
    std::vector<std::wstring> groupDirs;
    std::wstring pattern = root + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            std::wstring name = fd.cFileName;
            if (name == L"." || name == L"..") continue;
            groupDirs.push_back(name);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    std::sort(groupDirs.begin(), groupDirs.end());
    std::reverse(groupDirs.begin(), groupDirs.end());

    for (const std::wstring& gd : groupDirs) {
        WinXGroup group;
        group.id = wide_to_utf8(gd);
        scan_group(root, gd, &group);
        groups.push_back(std::move(group));
    }

    if (comInit) CoUninitialize();
    return groups;
}

std::string scan_winx_json() {
    std::vector<WinXGroup> groups = scan_winx();
    std::string out = "[";
    for (size_t g = 0; g < groups.size(); ++g) {
        if (g) out += ",";
        out += "{\"id\":\"";     out += json_escape(groups[g].id); out += "\"";
        out += ",\"items\":[";
        for (size_t i = 0; i < groups[g].items.size(); ++i) {
            const WinXItem& it = groups[g].items[i];
            if (i) out += ",";
            out += "{\"id\":\"";        out += json_escape(it.id);         out += "\"";
            out += ",\"label\":\"";     out += json_escape(it.label);      out += "\"";
            out += ",\"target\":\"";    out += json_escape(it.target);     out += "\"";
            out += ",\"iconSource\":\"";out += json_escape(it.iconSource); out += "\"";
            out += ",\"enabled\":";     out += it.enabled ? "true" : "false";
            out += "}";
        }
        out += "]}";
    }
    out += "]";
    return out;
}

std::string set_winx_item_enabled(const std::string& id, bool enabled) {
    std::wstring root = winx_root();
    if (root.empty()) return result_json(false, "winx_root_missing");

    // id 是相对 WinX 根的路径。禁止路径穿越（含 ".." 直接拒）。
    std::wstring rel = utf8_to_wide(id);
    if (rel.find(L"..") != std::wstring::npos)
        return result_json(false, "bad_id");

    std::wstring src = root + L"\\" + rel;

    // 计算目标名：启用 -> 去 .disabled 后缀；停用 -> 加 .disabled。
    std::wstring dst;
    bool srcIsDisabled = ends_with_ci(rel, L".lnk.disabled");
    bool srcIsLnk = ends_with_ci(rel, L".lnk");
    if (enabled) {
        if (srcIsLnk) return result_json(true, "ok");  // 已启用，幂等
        if (!srcIsDisabled) return result_json(false, "not_winx_lnk");
        dst = src.substr(0, src.size() - 9);  // 去 ".disabled"（长 9）
    } else {
        if (srcIsDisabled) return result_json(true, "ok");  // 已停用，幂等
        if (!srcIsLnk) return result_json(false, "not_winx_lnk");
        dst = src + L".disabled";
    }

    // 源必须存在。
    if (GetFileAttributesW(src.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json(false, "not_found");

    if (!MoveFileExW(src.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING))
        return result_json(false, "rename_failed");

    return result_json(true, "ok");
}

std::string add_winx_item(const std::string& group, const std::string& targetPath,
                          const std::string& args, const std::string& name) {
    std::wstring root = winx_root();
    if (root.empty()) return result_json(false, "winx_root_missing");

    std::wstring wgroup = utf8_to_wide(group);
    std::wstring wtarget = utf8_to_wide(targetPath);
    std::wstring wargs = utf8_to_wide(args);
    std::wstring wname = utf8_to_wide(name);
    // 基本校验：组名 / 名称禁止路径穿越与分隔符，目标必须存在。
    if (wgroup.empty() || wname.empty()) return result_json(false, "bad_arg");
    if (wgroup.find(L"..") != std::wstring::npos ||
        wgroup.find(L'\\') != std::wstring::npos ||
        wgroup.find(L'/') != std::wstring::npos)
        return result_json(false, "bad_group");
    if (wname.find(L"..") != std::wstring::npos ||
        wname.find(L'\\') != std::wstring::npos ||
        wname.find(L'/') != std::wstring::npos)
        return result_json(false, "bad_name");
    if (GetFileAttributesW(wtarget.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json(false, "target_not_found");

    // 组文件夹按需创建（Group1~3 常已存在；自定义组也允许）。
    std::wstring groupDir = root + L"\\" + wgroup;
    CreateDirectoryW(groupDir.c_str(), nullptr);

    std::wstring lnkPath = groupDir + L"\\" + wname + L".lnk";
    if (GetFileAttributesW(lnkPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        return result_json(false, "already_exists");

    bool comInit = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    bool created = false;
    IShellLinkW* link = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW, reinterpret_cast<void**>(&link)))) {
        link->SetPath(wtarget.c_str());
        if (!wargs.empty()) link->SetArguments(wargs.c_str());
        IPersistFile* pf = nullptr;
        if (SUCCEEDED(link->QueryInterface(IID_IPersistFile,
                                           reinterpret_cast<void**>(&pf)))) {
            if (SUCCEEDED(pf->Save(lnkPath.c_str(), TRUE))) created = true;
            pf->Release();
        }
        link->Release();
    }

    // lnk 建好后写合法 hash，否则 explorer 忽略该项不显示（Win+X 机制）。
    bool hashed = false;
    if (created) hashed = write_winx_hash(lnkPath, wtarget, wargs);
    if (comInit) CoUninitialize();

    if (!created) return result_json(false, "create_failed");
    if (!hashed) {
        // hash 写失败：项虽在但 Win+X 不会显示。删掉半成品避免留垃圾（此处直接删，
        // 是我们刚建的临时文件、尚未生效，不涉及用户数据，不需回收站）。
        DeleteFileW(lnkPath.c_str());
        return result_json(false, "hash_failed");
    }
    return result_json(true, "ok");
}

std::string del_winx_item(const std::string& id) {
    std::wstring root = winx_root();
    if (root.empty()) return result_json(false, "winx_root_missing");

    std::wstring rel = utf8_to_wide(id);
    if (rel.find(L"..") != std::wstring::npos)
        return result_json(false, "bad_id");
    // 只允许删 .lnk / .lnk.disabled，防误删其他文件。
    if (!ends_with_ci(rel, L".lnk") && !ends_with_ci(rel, L".lnk.disabled"))
        return result_json(false, "not_winx_lnk");

    std::wstring full = root + L"\\" + rel;
    if (GetFileAttributesW(full.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json(false, "not_found");

    // 送回收站（开发规范安全红线：删除永不硬删）。SHFileOperationW + FOF_ALLOWUNDO，
    // pFrom 需双 NUL 结尾。
    // 本函数经异步 bind 在全新 std::thread 里跑（该线程从未初始化 COM），而
    // SHFileOperationW 送回收站底层走 shell 的 IFileOperation，依赖调用线程 COM。
    // 故自持 CoInitializeEx 配对（与 add_winx_item 对称），避免新线程无 COM 时删除失败。
    bool comInit = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    std::wstring dbl = full;
    dbl.push_back(L'\0');
    dbl.push_back(L'\0');
    SHFILEOPSTRUCTW op = {0};
    op.wFunc = FO_DELETE;
    op.pFrom = dbl.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
    int rc = SHFileOperationW(&op);
    if (comInit) CoUninitialize();
    if (rc != 0 || op.fAnyOperationsAborted)
        return result_json(false, "delete_failed");

    return result_json(true, "ok");
}

}  // namespace cmm
