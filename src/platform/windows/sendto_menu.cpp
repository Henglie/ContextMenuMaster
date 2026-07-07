// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 发送到（SendTo）菜单管理（实现）
//
//  SendTo 与 Win+X 同为文件系统机制（非注册表），故仿 winx_menu 实现，但更简单：
//    - 无分组（SendTo 是扁平单目录）。
//    - 无 hash（System.Winx.Hash 是 Win+X 独有；SendTo 项 explorer 直接认，改名即启停）。
//  根目录 %APPDATA%\Microsoft\Windows\SendTo，下辖 .lnk / .exe / .url / 其它文件。
//  启停 = 加 / 去 ".disabled" 后缀（explorer 只枚举无该后缀的项，可逆）。
// ============================================================
#include "platform/windows/sendto_menu.h"

#include <windows.h>
#include <shlobj.h>      // IShellLink, SHFileOperationW
#include <objbase.h>     // CoCreateInstance
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ole32.lib")

namespace cmm {

namespace {

// —— UTF-8 <-> UTF-16（本模块自持一份，避免跨 TU 依赖，与 winx_menu 一致）——
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

// 大小写不敏感判断 s 是否以 suffix 结尾。
bool ends_with_ci(const std::wstring& s, const std::wstring& suffix) {
    if (s.size() < suffix.size()) return false;
    return CompareStringOrdinal(
        s.c_str() + (s.size() - suffix.size()), (int)suffix.size(),
        suffix.c_str(), (int)suffix.size(), TRUE) == CSTR_EQUAL;
}

// SendTo 根目录 %APPDATA%\Microsoft\Windows\SendTo。取不到返回空。
std::wstring sendto_root() {
    wchar_t base[MAX_PATH] = {0};
    DWORD got = GetEnvironmentVariableW(L"APPDATA", base, MAX_PATH);
    if (got == 0 || got >= MAX_PATH) return L"";
    return std::wstring(base) + L"\\Microsoft\\Windows\\SendTo";
}

// 读一个 .lnk 的目标路径。失败返回空串。假定调用线程已 CoInitialize。
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

// 从文件名推显示名：去掉 ".disabled" 后缀 + 去掉最后一段扩展名（.lnk/.exe/.url）。
std::wstring display_name(const std::wstring& fileName) {
    std::wstring name = fileName;
    if (ends_with_ci(name, L".disabled"))
        name = name.substr(0, name.size() - 9);  // ".disabled" 长 9
    // 去最后一段扩展名。
    size_t dot = name.rfind(L'.');
    if (dot != std::wstring::npos && dot > 0)
        name = name.substr(0, dot);
    return name;
}

}  // namespace

std::vector<SendToItem> scan_sendto() {
    std::vector<SendToItem> items;
    std::wstring root = sendto_root();
    if (root.empty()) return items;

    bool comInit = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));

    std::wstring pattern = root + L"\\*";
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            std::wstring fname = fd.cFileName;
            // desktop.ini 等隐藏配置不列。
            if (CompareStringOrdinal(fname.c_str(), -1, L"desktop.ini", -1, TRUE)
                == CSTR_EQUAL) continue;

            SendToItem item;
            item.id = wide_to_utf8(fname);
            item.enabled = !ends_with_ci(fname, L".disabled");
            item.label = wide_to_utf8(display_name(fname));

            std::wstring full = root + L"\\" + fname;
            // .lnk（含 .lnk.disabled）读快捷方式目标；非 lnk（如 exe 直放）目标即自身。
            std::wstring stem = item.enabled ? fname : fname.substr(0, fname.size() - 9);
            if (ends_with_ci(stem, L".lnk")) {
                std::wstring target = read_lnk_target(full);
                if (!target.empty()) {
                    item.target = wide_to_utf8(target);
                    item.iconSource = item.target;
                }
            } else {
                item.target = wide_to_utf8(full);
                item.iconSource = item.target;
            }
            items.push_back(std::move(item));
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }

    std::sort(items.begin(), items.end(),
              [](const SendToItem& a, const SendToItem& b) { return a.id < b.id; });

    if (comInit) CoUninitialize();
    return items;
}

std::string scan_sendto_json() {
    std::vector<SendToItem> items = scan_sendto();
    std::string out = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        const SendToItem& it = items[i];
        if (i) out += ",";
        out += "{\"id\":\"";        out += json_escape(it.id);         out += "\"";
        out += ",\"label\":\"";     out += json_escape(it.label);      out += "\"";
        out += ",\"target\":\"";    out += json_escape(it.target);     out += "\"";
        out += ",\"iconSource\":\"";out += json_escape(it.iconSource); out += "\"";
        out += ",\"enabled\":";     out += it.enabled ? "true" : "false";
        out += "}";
    }
    out += "]";
    return out;
}

std::string set_sendto_enabled(const std::string& id, bool enabled) {
    std::wstring root = sendto_root();
    if (root.empty()) return result_json(false, "sendto_root_missing");

    std::wstring rel = utf8_to_wide(id);
    if (rel.find(L"..") != std::wstring::npos ||
        rel.find(L'\\') != std::wstring::npos ||
        rel.find(L'/') != std::wstring::npos)
        return result_json(false, "bad_id");

    std::wstring src = root + L"\\" + rel;
    bool srcIsDisabled = ends_with_ci(rel, L".disabled");

    std::wstring dst;
    if (enabled) {
        if (!srcIsDisabled) return result_json(true, "ok");  // 已启用，幂等
        dst = src.substr(0, src.size() - 9);  // 去 ".disabled"
    } else {
        if (srcIsDisabled) return result_json(true, "ok");  // 已停用，幂等
        dst = src + L".disabled";
    }

    if (GetFileAttributesW(src.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json(false, "not_found");

    if (!MoveFileExW(src.c_str(), dst.c_str(), MOVEFILE_REPLACE_EXISTING))
        return result_json(false, "rename_failed");

    return result_json(true, "ok");
}

std::string add_sendto_item(const std::string& targetPath, const std::string& name) {
    std::wstring root = sendto_root();
    if (root.empty()) return result_json(false, "sendto_root_missing");

    std::wstring wtarget = utf8_to_wide(targetPath);
    std::wstring wname = utf8_to_wide(name);
    if (wname.empty()) return result_json(false, "bad_arg");
    if (wname.find(L"..") != std::wstring::npos ||
        wname.find(L'\\') != std::wstring::npos ||
        wname.find(L'/') != std::wstring::npos)
        return result_json(false, "bad_name");
    if (GetFileAttributesW(wtarget.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json(false, "target_not_found");

    std::wstring lnkPath = root + L"\\" + wname + L".lnk";
    if (GetFileAttributesW(lnkPath.c_str()) != INVALID_FILE_ATTRIBUTES)
        return result_json(false, "already_exists");

    bool comInit = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    bool created = false;
    IShellLinkW* link = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW, reinterpret_cast<void**>(&link)))) {
        link->SetPath(wtarget.c_str());
        IPersistFile* pf = nullptr;
        if (SUCCEEDED(link->QueryInterface(IID_IPersistFile,
                                           reinterpret_cast<void**>(&pf)))) {
            if (SUCCEEDED(pf->Save(lnkPath.c_str(), TRUE))) created = true;
            pf->Release();
        }
        link->Release();
    }
    if (comInit) CoUninitialize();

    if (!created) return result_json(false, "create_failed");
    return result_json(true, "ok");
}

std::string del_sendto_item(const std::string& id) {
    std::wstring root = sendto_root();
    if (root.empty()) return result_json(false, "sendto_root_missing");

    std::wstring rel = utf8_to_wide(id);
    if (rel.find(L"..") != std::wstring::npos ||
        rel.find(L'\\') != std::wstring::npos ||
        rel.find(L'/') != std::wstring::npos)
        return result_json(false, "bad_id");

    std::wstring full = root + L"\\" + rel;
    if (GetFileAttributesW(full.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json(false, "not_found");

    // 送回收站（开发规范安全红线）。异步 bind 新线程需自持 COM（同 winx del）。
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

std::string rename_sendto_item(const std::string& id, const std::string& newName) {
    std::wstring root = sendto_root();
    if (root.empty()) return result_json(false, "sendto_root_missing");

    std::wstring rel = utf8_to_wide(id);
    std::wstring wnew = utf8_to_wide(newName);
    if (rel.find(L"..") != std::wstring::npos ||
        rel.find(L'\\') != std::wstring::npos ||
        rel.find(L'/') != std::wstring::npos)
        return result_json(false, "bad_id");
    if (wnew.empty() ||
        wnew.find(L"..") != std::wstring::npos ||
        wnew.find(L'\\') != std::wstring::npos ||
        wnew.find(L'/') != std::wstring::npos)
        return result_json(false, "bad_name");

    std::wstring core = rel;
    std::wstring disabledSuffix;
    if (ends_with_ci(core, L".disabled")) {
        disabledSuffix = core.substr(core.size() - 9);
        core = core.substr(0, core.size() - 9);
    }
    std::wstring ext;
    size_t dot = core.rfind(L'.');
    if (dot != std::wstring::npos && dot > 0) ext = core.substr(dot);
    std::wstring newRel = wnew + ext + disabledSuffix;

    std::wstring src = root + L"\\" + rel;
    std::wstring dst = root + L"\\" + newRel;
    if (GetFileAttributesW(src.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json(false, "not_found");
    if (CompareStringOrdinal(src.c_str(), -1, dst.c_str(), -1, TRUE) == CSTR_EQUAL)
        return result_json(true, "ok");
    if (GetFileAttributesW(dst.c_str()) != INVALID_FILE_ATTRIBUTES)
        return result_json(false, "already_exists");
    if (!MoveFileExW(src.c_str(), dst.c_str(), 0))
        return result_json(false, "rename_failed");
    return result_json(true, "ok");
}

std::string set_sendto_icon(const std::string& id, const std::string& iconSource) {
    std::wstring root = sendto_root();
    if (root.empty()) return result_json(false, "sendto_root_missing");

    std::wstring rel = utf8_to_wide(id);
    if (rel.find(L"..") != std::wstring::npos ||
        rel.find(L'\\') != std::wstring::npos ||
        rel.find(L'/') != std::wstring::npos)
        return result_json(false, "bad_id");

    std::wstring stem = rel;
    if (ends_with_ci(stem, L".disabled")) stem = stem.substr(0, stem.size() - 9);
    if (!ends_with_ci(stem, L".lnk")) return result_json(false, "not_lnk");

    std::wstring full = root + L"\\" + rel;
    if (GetFileAttributesW(full.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json(false, "not_found");

    std::wstring wicon = utf8_to_wide(iconSource);
    std::wstring iconPath = wicon;
    int iconIndex = 0;
    size_t comma = wicon.rfind(L',');
    if (comma != std::wstring::npos) {
        std::wstring idxStr = wicon.substr(comma + 1);
        bool allDigit = !idxStr.empty();
        for (wchar_t c : idxStr) if (c < L'0' || c > L'9') { allDigit = false; break; }
        if (allDigit) {
            iconPath = wicon.substr(0, comma);
            iconIndex = _wtoi(idxStr.c_str());
        }
    }

    bool comInit = SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
    bool ok = false;
    IShellLinkW* link = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IShellLinkW, reinterpret_cast<void**>(&link)))) {
        IPersistFile* pf = nullptr;
        if (SUCCEEDED(link->QueryInterface(IID_IPersistFile,
                                           reinterpret_cast<void**>(&pf)))) {
            if (SUCCEEDED(pf->Load(full.c_str(), STGM_READWRITE))) {
                if (SUCCEEDED(link->SetIconLocation(iconPath.c_str(), iconIndex))) {
                    if (SUCCEEDED(pf->Save(full.c_str(), TRUE))) ok = true;
                }
            }
            pf->Release();
        }
        link->Release();
    }
    if (comInit) CoUninitialize();
    return ok ? result_json(true, "ok") : result_json(false, "icon_failed");
}

}  // namespace cmm
