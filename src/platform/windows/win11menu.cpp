// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— Win11 新式右键菜单管理（实现）
//  见 win11menu.h 顶部机制说明。
// ============================================================
#include "platform/windows/win11menu.h"

#include <windows.h>
#include <shlwapi.h>   // SHLoadIndirectString / SHCreateStreamOnFileEx
#include <xmllite.h>   // IXmlReader（流式解析 AppxManifest.xml）
#include <objbase.h>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "xmllite.lib")

namespace cmm {
namespace {

constexpr wchar_t kPackagedComPath[]   = L"PackagedCom\\Package";
constexpr wchar_t kPackageRepoPath[]   = L"Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\PackageRepository\\Packages";
constexpr wchar_t kCommandStorePath[]  = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CommandStore\\shell";
constexpr wchar_t kBlockedPath[]       = L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Blocked";
// HKLM 下路径不含前导 Software\ 之外的差异，与上一致（HKLM\SOFTWARE\... / HKCU\Software\...）。

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
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

// 规范化 GUID 为「{小写}」形式，与参考项目 Guid.ToString("B") 一致。
// 非 36 长度的裸值原样小写返回（不强加大括号，供宽松匹配）。
std::wstring normalize_guid(std::wstring g) {
    // 去首尾空白与大括号
    while (!g.empty() && (g.front() == L' ' || g.front() == L'{')) g.erase(g.begin());
    while (!g.empty() && (g.back() == L' ' || g.back() == L'}')) g.pop_back();
    std::transform(g.begin(), g.end(), g.begin(), ::towlower);
    if (g.size() == 36) return L"{" + g + L"}";
    return g;
}

// 解析间接字符串资源 "@dll,-id" → 真实本地化文案。非 @ 开头原样返回。
std::wstring resolve_indirect(const std::wstring& spec) {
    if (spec.empty() || spec[0] != L'@') return spec;
    wchar_t buf[512];
    if (SUCCEEDED(SHLoadIndirectString(spec.c_str(), buf, ARRAYSIZE(buf), nullptr)) && buf[0])
        return buf;
    return spec;
}

// 从包全名取友好名。包全名形如 "Microsoft.OneDriveSync_25206.._neutral_.."：
//   取首段（首个 '_' 前）"Microsoft.OneDriveSync"，再取最后一个 '.' 后的词 "OneDriveSync"。
// label 是通用词/为空时兜底，比 "Context Menu Items" 或整个包全名可读。
std::wstring friendly_pkg_name(const std::wstring& pkgFullName) {
    std::wstring head = pkgFullName;
    size_t us = head.find(L'_');
    if (us != std::wstring::npos) head = head.substr(0, us);   // 去版本/架构后缀
    size_t dot = head.rfind(L'.');
    if (dot != std::wstring::npos && dot + 1 < head.size()) head = head.substr(dot + 1);
    return head.empty() ? pkgFullName : head;
}

// 判断 label 是否「通用/无信息」——这类回退到友好包名。OneDrive 的多个 handler
// DisplayName 全填成 "Context Menu Items"，直接显示会刷屏，故归入通用词。
bool is_generic_label(const std::wstring& label) {
    if (label.empty()) return true;
    std::wstring low = label;
    std::transform(low.begin(), low.end(), low.begin(), ::towlower);
    return low == L"context menu items"
        || low == L"context menu verb handler"
        || low == L"context menu handler";
}

// 一个包的 COM 服务信息与上下文声明，解析交集后得 handler 项。
struct ComServerInfo {
    std::wstring path;         // 拼好的绝对路径（安装目录 + Class Path / server Executable）
    std::wstring displayName;  // server DisplayName（可空）
};

struct ManifestResult {
    // clsid(规范化) -> 适用类型列表（原始 Type 字符串）
    std::map<std::wstring, std::vector<std::wstring>> contextMenus;
    // clsid(规范化) -> COM 服务信息
    std::map<std::wstring, ComServerInfo> comServers;
};

// 读属性 helper：把当前元素的某属性（按 local name）读出。
bool read_attr(IXmlReader* reader, const wchar_t* attrLocal, std::wstring* out) {
    HRESULT hr = reader->MoveToFirstAttribute();
    while (hr == S_OK) {
        const wchar_t* an = nullptr; UINT anl = 0;
        if (SUCCEEDED(reader->GetLocalName(&an, &anl)) && an) {
            if (wcscmp(an, attrLocal) == 0) {
                const wchar_t* av = nullptr; UINT avl = 0;
                if (SUCCEEDED(reader->GetValue(&av, &avl)) && av) {
                    out->assign(av, avl);
                    reader->MoveToElement();
                    return true;
                }
            }
        }
        hr = reader->MoveToNextAttribute();
    }
    reader->MoveToElement();
    return false;
}

// 流式解析一个 AppxManifest.xml。installPath 用于拼 ComServer 相对路径。
bool parse_manifest(const std::wstring& manifestPath, const std::wstring& installPath,
                    ManifestResult* out) {
    IStream* stream = nullptr;
    if (FAILED(SHCreateStreamOnFileEx(manifestPath.c_str(),
                                      STGM_READ | STGM_SHARE_DENY_WRITE,
                                      0, FALSE, nullptr, &stream)) || !stream)
        return false;

    IXmlReader* reader = nullptr;
    if (FAILED(CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<void**>(&reader), nullptr))) {
        stream->Release();
        return false;
    }
    reader->SetInput(stream);

    std::wstring curItemType;       // 当前 ItemType 的 Type（Verb 归属）
    std::wstring curServerDisplay;  // 当前 ComServer 子节点 DisplayName
    std::wstring curServerExe;      // 当前 ComServer 子节点 Executable

    XmlNodeType nodeType;
    while (reader->Read(&nodeType) == S_OK) {
        if (nodeType != XmlNodeType_Element) continue;
        const wchar_t* ln = nullptr; UINT lnl = 0;
        if (FAILED(reader->GetLocalName(&ln, &lnl)) || !ln) continue;
        std::wstring name(ln, lnl);

        if (name == L"ItemType") {
            curItemType.clear();
            read_attr(reader, L"Type", &curItemType);
        } else if (name == L"Verb") {
            std::wstring clsid;
            if (read_attr(reader, L"Clsid", &clsid) && !clsid.empty()) {
                std::wstring key = normalize_guid(clsid);
                // Type 显示：Directory 保留原样，其它前缀 "File: "（对齐参考项目）
                std::wstring typeText;
                if (!curItemType.empty()) {
                    if (_wcsicmp(curItemType.c_str(), L"Directory") == 0)
                        typeText = curItemType;
                    else
                        typeText = L"File: " + curItemType;
                }
                if (!typeText.empty())
                    out->contextMenus[key].push_back(typeText);
                else
                    out->contextMenus[key];  // 确保键存在
            }
        } else if (name == L"SurrogateServer" || name == L"ExeServer") {
            curServerDisplay.clear();
            curServerExe.clear();
            read_attr(reader, L"DisplayName", &curServerDisplay);
            read_attr(reader, L"Executable", &curServerExe);
        } else if (name == L"Class") {
            std::wstring id, relPath;
            if (read_attr(reader, L"Id", &id) && !id.empty()) {
                std::wstring key = normalize_guid(id);
                read_attr(reader, L"Path", &relPath);
                std::wstring rel = !relPath.empty() ? relPath : curServerExe;
                ComServerInfo info;
                if (!rel.empty()) {
                    info.path = installPath;
                    if (!info.path.empty() && info.path.back() != L'\\') info.path += L'\\';
                    info.path += rel;
                }
                info.displayName = curServerDisplay;
                if (out->comServers.find(key) == out->comServers.end())
                    out->comServers[key] = info;
            }
        }
    }

    reader->Release();
    stream->Release();
    return true;
}

// 取安装路径：HKCR\Local Settings\...\PackageRepository\Packages\<全名> 的 Path 值。
std::wstring get_install_path(const std::wstring& packageFullName) {
    std::wstring sub = std::wstring(kPackageRepoPath) + L"\\" + packageFullName;
    HKEY h = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, sub.c_str(), 0, KEY_READ, &h) != ERROR_SUCCESS)
        return L"";
    std::wstring path;
    reg_read_string(h, L"Path", &path);
    RegCloseKey(h);
    return path;
}

// 枚举 PackagedCom\Package 下全部包全名。
std::vector<std::wstring> enum_packages() {
    std::vector<std::wstring> names;
    HKEY h = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, kPackagedComPath, 0, KEY_READ, &h) != ERROR_SUCCESS)
        return names;
    wchar_t nameBuf[512];
    DWORD idx = 0, cch;
    while (true) {
        cch = ARRAYSIZE(nameBuf);
        LSTATUS st = RegEnumKeyExW(h, idx++, nameBuf, &cch, nullptr, nullptr, nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st == ERROR_SUCCESS) names.emplace_back(nameBuf);
        else break;
    }
    RegCloseKey(h);
    return names;
}

// 读某 hive 的 Blocked 名单为「规范化 clsid -> 显示名」映射。
std::map<std::wstring, std::wstring> read_blocked(HKEY hive) {
    std::map<std::wstring, std::wstring> m;
    HKEY h = nullptr;
    if (RegOpenKeyExW(hive, kBlockedPath, 0, KEY_READ, &h) != ERROR_SUCCESS)
        return m;
    wchar_t nameBuf[256];
    DWORD idx = 0, cch, type;
    wchar_t dataBuf[512]; DWORD cb;
    while (true) {
        cch = ARRAYSIZE(nameBuf);
        cb = sizeof(dataBuf);
        LSTATUS st = RegEnumValueW(h, idx++, nameBuf, &cch, nullptr, &type,
                                   reinterpret_cast<BYTE*>(dataBuf), &cb);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st != ERROR_SUCCESS) break;
        std::wstring vn(nameBuf, cch);
        std::wstring key = normalize_guid(vn);
        std::wstring disp;
        if (type == REG_SZ || type == REG_EXPAND_SZ) {
            dataBuf[std::min<DWORD>(cb / sizeof(wchar_t), ARRAYSIZE(dataBuf) - 1)] = 0;
            disp = dataBuf;
        }
        m[key] = disp;
    }
    RegCloseKey(h);
    return m;
}

// 系统命令白名单：CommandStore 里 Windows.* 有上百个，绝大多数是文件管理器
// 内部命令（复制/粘贴/视图/排序…）与无关命令，且全部只读（canToggle=false）。
// 全列出来是噪音，用户看一堆动不了的项体验差。对齐参考项目，只保留真正属于
// 「Win11 现代右键菜单」且有意义的两个系统项。
bool is_whitelisted_system_command(const std::wstring& keyName) {
    return _wcsicmp(keyName.c_str(), L"Windows.SendToMyPhone") == 0
        || _wcsicmp(keyName.c_str(), L"Windows.Share") == 0;
}

// 枚举 SystemCommandStore 里白名单内的系统命令（只读展示）。
void scan_system_commands(std::vector<Win11MenuItem>* out) {
    HKEY hRoot = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kCommandStorePath, 0, KEY_READ, &hRoot) != ERROR_SUCCESS)
        return;
    wchar_t nameBuf[256];
    DWORD idx = 0, cch;
    while (true) {
        cch = ARRAYSIZE(nameBuf);
        LSTATUS st = RegEnumKeyExW(hRoot, idx++, nameBuf, &cch, nullptr, nullptr, nullptr, nullptr);
        if (st == ERROR_NO_MORE_ITEMS) break;
        if (st != ERROR_SUCCESS) break;
        std::wstring keyName(nameBuf, cch);
        if (!is_whitelisted_system_command(keyName)) continue;

        HKEY hItem = nullptr;
        if (RegOpenKeyExW(hRoot, keyName.c_str(), 0, KEY_READ, &hItem) != ERROR_SUCCESS)
            continue;

        std::wstring mui, label;
        if (reg_read_string(hItem, L"MUIVerb", &mui) && !mui.empty()) {
            label = resolve_indirect(mui);
        }
        // 白名单项一定展示；MUIVerb 解析失败时用去前缀的键名兜底（如 "SendToMyPhone"）。
        if (label.empty() || label[0] == L'@') label = keyName.substr(8);

        std::wstring icon;
        reg_read_string(hItem, L"Icon", &icon);
        std::wstring handler;
        reg_read_string(hItem, L"ExplorerCommandHandler", &handler);

        Win11MenuItem it;
        it.id = "system|" + wide_to_utf8(keyName);
        it.source = "system";
        it.commandKey = wide_to_utf8(keyName);
        it.clsid = handler.empty() ? "" : wide_to_utf8(normalize_guid(handler));
        it.label = wide_to_utf8(label);
        it.iconSource = wide_to_utf8(icon);
        it.enabled = true;
        it.canToggle = false;  // 系统内置只读（误封锁会致现代菜单回退经典）
        it.system = true;
        out->push_back(std::move(it));
        RegCloseKey(hItem);
    }
    RegCloseKey(hRoot);
}

}  // namespace

std::vector<Win11MenuItem> scan_win11menu() {
    std::vector<Win11MenuItem> items;

    // 仅 Win11 22000+ 有现代菜单；低版本直接返回空。
    // 用 CommandStore 是否存在做粗略判断即可，不引 versionhelpers。
    // PackagedCom 项 -------------------------------------------------------
    auto blockedMachine = read_blocked(HKEY_LOCAL_MACHINE);
    auto blockedUser    = read_blocked(HKEY_CURRENT_USER);

    std::set<std::wstring> seen;  // 去重（同 clsid 跨包只取一次）
    std::map<std::wstring, size_t> pkgMerge;  // package|label -> items 下标（同包同名归并）
    for (const auto& pkg : enum_packages()) {
        std::wstring installPath = get_install_path(pkg);
        if (installPath.empty()) continue;

        std::wstring manifest = installPath + L"\\AppxManifest.xml";
        DWORD attr = GetFileAttributesW(manifest.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            std::wstring alt = installPath + L"\\AppxMetadata\\AppxBundleManifest.xml";
            if (GetFileAttributesW(alt.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
            manifest = alt;
        }

        ManifestResult mr;
        if (!parse_manifest(manifest, installPath, &mr)) continue;

        // 友好包名（label 通用/为空时兜底，如 OneDrive 的 21 个 "Context Menu Items"）。
        std::wstring pkgFriendly = friendly_pkg_name(pkg);

        // 交集：既在 FileExplorerContextMenus 声明、又有 ComServer Class 的 clsid。
        // 归并：同一包内 label 相同的多个 handler（如 OneDrive 21 个同名项）合并成一项，
        //   clsid 以 ';' 拼接，启停时整组一起写黑名单。key = 包全名 + '|' + 显示 label。
        for (const auto& cm : mr.contextMenus) {
            auto sit = mr.comServers.find(cm.first);
            if (sit == mr.comServers.end()) continue;
            if (seen.count(cm.first)) continue;
            seen.insert(cm.first);

            const std::wstring& clsid = cm.first;
            // 显示名：ComServer DisplayName 优先；通用词/空则回退友好包名。
            std::wstring label = resolve_indirect(sit->second.displayName);
            if (is_generic_label(label)) label = pkgFriendly;

            // 适用场景文字
            std::wstring types;
            for (const auto& t : cm.second) {
                if (!types.empty()) types += L", ";
                types += t;
            }

            std::wstring mergeKey = pkg + L"|" + label;
            auto mit = pkgMerge.find(mergeKey);
            if (mit != pkgMerge.end()) {
                // 已有同包同名项 → 合并 CLSID（分号拼接），不新增行。
                Win11MenuItem& ex = items[mit->second];
                ex.clsid += ";" + wide_to_utf8(clsid);
                bool bm = blockedMachine.count(clsid) > 0;
                bool bu = blockedUser.count(clsid) > 0;
                ex.blockedMachine = ex.blockedMachine || bm;
                ex.blockedUser    = ex.blockedUser || bu;
                ex.enabled = !(ex.blockedMachine || ex.blockedUser);
                if (ex.contextTypes.empty()) ex.contextTypes = wide_to_utf8(types);
                continue;
            }

            Win11MenuItem it;
            it.id = "packaged|" + wide_to_utf8(clsid);
            it.source = "packaged";
            it.clsid = wide_to_utf8(clsid);
            it.label = wide_to_utf8(label);
            it.iconSource = wide_to_utf8(sit->second.path);
            it.package = wide_to_utf8(pkg);
            it.contextTypes = wide_to_utf8(types);
            it.blockedMachine = blockedMachine.count(clsid) > 0;
            it.blockedUser    = blockedUser.count(clsid) > 0;
            it.enabled = !(it.blockedMachine || it.blockedUser);
            it.canToggle = true;
            it.system = false;
            pkgMerge[mergeKey] = items.size();
            items.push_back(std::move(it));
        }
    }

    // 归并：同一包内 label 相同的多个 handler（如 OneDrive 声明 21 个 CLSID、
    // ComServer DisplayName 全填通用词）合并成一项，避免菜单刷屏。clsid 用分号拼接，
    // 启停时对该组所有 CLSID 一起写黑名单（见 set_win11menu_enabled）。
    {
        std::vector<Win11MenuItem> merged;
        std::map<std::wstring, size_t> keyIndex;  // package|label -> merged 下标
        for (auto& it : items) {
            std::wstring key = utf8_to_wide(it.package) + L"|" + utf8_to_wide(it.label);
            auto f = keyIndex.find(key);
            if (f == keyIndex.end()) {
                keyIndex[key] = merged.size();
                merged.push_back(std::move(it));
            } else {
                Win11MenuItem& tgt = merged[f->second];
                tgt.clsid += ";";
                tgt.clsid += it.clsid;               // 追加同组 CLSID
                if (it.blockedMachine) tgt.blockedMachine = true;
                if (it.blockedUser)    tgt.blockedUser = true;
                // 组内任一被封锁即视为停用（保守：全组统一状态）
                tgt.enabled = !(tgt.blockedMachine || tgt.blockedUser);
            }
        }
        items.swap(merged);
    }

    // SystemCommandStore 项（只读） ---------------------------------------
    scan_system_commands(&items);

    // 排序：packaged 在前，同源按显示名。
    std::sort(items.begin(), items.end(), [](const Win11MenuItem& a, const Win11MenuItem& b) {
        if (a.source != b.source) return a.source < b.source;  // "packaged" < "system"
        return a.label < b.label;
    });

    return items;
}

std::string scan_win11menu_json() {
    std::vector<Win11MenuItem> items = scan_win11menu();
    std::string o = "[";
    bool first = true;
    for (const auto& it : items) {
        if (!first) o += ",";
        first = false;
        o += "{\"id\":\"";           o += json_escape(it.id);           o += "\"";
        o += ",\"source\":\"";       o += json_escape(it.source);       o += "\"";
        o += ",\"clsid\":\"";        o += json_escape(it.clsid);        o += "\"";
        o += ",\"commandKey\":\"";   o += json_escape(it.commandKey);   o += "\"";
        o += ",\"label\":\"";        o += json_escape(it.label);        o += "\"";
        o += ",\"iconSource\":\"";   o += json_escape(it.iconSource);   o += "\"";
        o += ",\"package\":\"";      o += json_escape(it.package);      o += "\"";
        o += ",\"contextTypes\":\""; o += json_escape(it.contextTypes); o += "\"";
        o += ",\"enabled\":";        o += it.enabled ? "true" : "false";
        o += ",\"blockedMachine\":"; o += it.blockedMachine ? "true" : "false";
        o += ",\"blockedUser\":";    o += it.blockedUser ? "true" : "false";
        o += ",\"canToggle\":";      o += it.canToggle ? "true" : "false";
        o += ",\"system\":";         o += it.system ? "true" : "false";
        o += "}";
    }
    o += "]";
    return o;
}

std::string set_win11menu_enabled(const std::string& clsid,
                                  const std::string& displayName, bool enabled) {
    // clsid 可能是分号拼接的多个 CLSID（归并项，如 OneDrive 一组）——全部处理。
    std::vector<std::wstring> norms;
    {
        std::wstring all = utf8_to_wide(clsid);
        size_t pos = 0;
        while (pos <= all.size()) {
            size_t sc = all.find(L';', pos);
            std::wstring seg = all.substr(pos, sc == std::wstring::npos ? std::wstring::npos : sc - pos);
            std::wstring n = normalize_guid(seg);
            if (!n.empty()) norms.push_back(n);
            if (sc == std::wstring::npos) break;
            pos = sc + 1;
        }
    }
    if (norms.empty()) return result_json(false, "bad_clsid");

    std::wstring disp = utf8_to_wide(displayName);
    bool anyOk = false;

    for (HKEY hive : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
        HKEY h = nullptr;
        if (enabled) {
            if (RegOpenKeyExW(hive, kBlockedPath, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &h) != ERROR_SUCCESS)
                continue;
            // 收集所有需删的值名（对每个 norm 做变体匹配），再统一删。
            std::vector<std::wstring> toDelete;
            wchar_t nameBuf[256]; DWORD idx = 0, cch;
            while (true) {
                cch = ARRAYSIZE(nameBuf);
                LSTATUS st = RegEnumValueW(h, idx++, nameBuf, &cch, nullptr, nullptr, nullptr, nullptr);
                if (st == ERROR_NO_MORE_ITEMS) break;
                if (st != ERROR_SUCCESS) break;
                std::wstring vn(nameBuf, cch);
                std::wstring vnNorm = normalize_guid(vn);
                for (const auto& n : norms) {
                    if (vnNorm == n) { toDelete.push_back(vn); break; }
                }
            }
            for (const auto& n : norms) RegDeleteValueW(h, n.c_str());
            for (const auto& vn : toDelete) RegDeleteValueW(h, vn.c_str());
            anyOk = true;
            RegCloseKey(h);
        } else {
            if (RegCreateKeyExW(hive, kBlockedPath, 0, nullptr, 0,
                                KEY_SET_VALUE, nullptr, &h, nullptr) != ERROR_SUCCESS)
                continue;
            for (const auto& n : norms) {
                LSTATUS st = RegSetValueExW(h, n.c_str(), 0, REG_SZ,
                                            reinterpret_cast<const BYTE*>(disp.c_str()),
                                            (DWORD)((disp.size() + 1) * sizeof(wchar_t)));
                if (st == ERROR_SUCCESS) anyOk = true;
            }
            RegCloseKey(h);
        }
    }

    return anyOk ? result_json(true, "ok") : result_json(false, "registry_failed");
}

}  // namespace cmm
