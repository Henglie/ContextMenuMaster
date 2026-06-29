// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— IContextMenu COM 扩展动态菜单探测（实现）
//
//  见头文件说明。本文件把第三方 shell 扩展「跑」起来读它的真实菜单，**有崩溃风险**，
//  设计上只在 CmmCli 子进程里调用（崩溃隔离）。
//
//  关键流程：
//    1) 按场景造一个真实右键对象（临时文件 / 临时目录 / C:\ / 目录背景），
//       拿到它的 IShellFolder 父 + 子 pidl，GetUIObjectOf 取 IDataObject。
//    2) CoCreateInstance 实例化 handler 的 CLSID。
//    3) 拿 IShellExtInit::Initialize(pidlFolder, pdtobj, hkeyProgID) 喂上下文。
//    4) QueryContextMenu 把动态项填进一个内存 HMENU（CMF_EXTENDEDVERBS 拿全集）。
//    5) 递归遍历 HMENU；子菜单先发 WM_INITMENUPOPUP（经 IContextMenu2/3）触发
//       handler 的懒填充，再读子项。
//    6) dump 成 JSON 菜单树（只读）。
// ============================================================
#include "platform/windows/com_menu_probe.h"

#include <windows.h>
#include <shlobj.h>     // IShellExtInit / IContextMenu / SHBindToParent / ILCreateFromPath
#include <shlwapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace cmm {

namespace {

// QueryContextMenu 的命令 id 区间（任意够大的窗口即可，不真正执行命令）。
constexpr UINT kCmdFirst = 1;
constexpr UINT kCmdLast  = 0x7FFF;
constexpr int  kMaxDepth = 6;   // 子菜单递归深度护栏

// —— UTF-8 -> UTF-16 ——
std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

// —— UTF-16 -> UTF-8 ——
std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                                nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(),
                        s.data(), n, nullptr, nullptr);
    return s;
}

// JSON 字符串值转义（本模块自带一份，菜单文字含任意字符都要过）。
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

std::string result_err(const char* key) {
    std::string s = "{\"ok\":false,\"key\":\"";
    s += key;
    s += "\"}";
    return s;
}

// —— 临时右键对象管理 ——
// 造一个真实存在的文件 / 目录给 handler 当右键目标。探测完删除。
// drive 用 C:\（不创建/删除），目录背景场景用临时目录但 pdtobj 走 null。
struct ScratchTarget {
    std::wstring path;       // 目标路径（文件或目录）
    bool isDir = false;      // 是否目录
    bool created = false;    // 是否由我们创建（决定要不要清理）
    bool background = false; // 目录背景场景：pidlFolder=path，pdtobj=null

    ~ScratchTarget() {
        if (!created || path.empty()) return;
        if (isDir) RemoveDirectoryW(path.c_str());
        else       DeleteFileW(path.c_str());
    }
};

// 取临时目录（末尾带反斜杠）。
std::wstring temp_dir() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetTempPathW(MAX_PATH, buf);
    if (n == 0 || n >= MAX_PATH) return L"";
    return std::wstring(buf, n);
}

// 按场景准备右键目标。失败返回 false。
// overridePath 非空时直接用它当右键对象（诊断用：喂真实文件给挑剔的 handler），
// 不创建、不删除；isDir 按它实际是不是目录判定。
bool make_scratch(MenuScene scene, ScratchTarget* t, const std::wstring& overridePath) {
    if (!overridePath.empty()) {
        DWORD attr = GetFileAttributesW(overridePath.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) return false;
        t->path = overridePath;
        t->isDir = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
        t->created = false;
        return true;
    }
    switch (scene) {
        case MenuScene::AllFiles: {
            // 任意文件：造一个临时 .txt（多数 handler 对所有文件都挂，扩展名不挑）。
            std::wstring dir = temp_dir();
            if (dir.empty()) return false;
            wchar_t name[MAX_PATH];
            if (GetTempFileNameW(dir.c_str(), L"cmm", 0, name) == 0) return false;
            // GetTempFileName 已创建该文件。
            t->path = name;
            t->isDir = false;
            t->created = true;
            return true;
        }
        case MenuScene::Directory: {
            std::wstring dir = temp_dir();
            if (dir.empty()) return false;
            std::wstring sub = dir + L"cmm_probe_dir";
            // 唯一化：附加计数避免撞已存在目录。
            for (int i = 0; i < 1000; ++i) {
                std::wstring cand = sub + std::to_wstring(i);
                if (CreateDirectoryW(cand.c_str(), nullptr)) {
                    t->path = cand; t->isDir = true; t->created = true;
                    return true;
                }
                if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
            }
            return false;
        }
        case MenuScene::Drive: {
            // 系统盘根（不创建、不删除）。
            wchar_t sys[MAX_PATH] = {0};
            UINT got = GetWindowsDirectoryW(sys, MAX_PATH);
            if (got >= 3) {
                t->path.assign(sys, 3);   // 形如 "C:\"
            } else {
                t->path = L"C:\\";
            }
            t->isDir = true; t->created = false;
            return true;
        }
        case MenuScene::DesktopBackground:
        case MenuScene::DirectoryBackground: {
            // 目录背景：handler 走 pidlFolder（文件夹本身）、pdtobj=null。
            // 用临时目录当背景目标。
            std::wstring dir = temp_dir();
            if (dir.empty()) return false;
            std::wstring sub = dir + L"cmm_probe_bg";
            for (int i = 0; i < 1000; ++i) {
                std::wstring cand = sub + std::to_wstring(i);
                if (CreateDirectoryW(cand.c_str(), nullptr)) {
                    t->path = cand; t->isDir = true; t->created = true;
                    t->background = true;
                    return true;
                }
                if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
            }
            return false;
        }
        case MenuScene::RecycleBin:
        default:
            // 回收站没有 shellex handler（scene_handler_root 返回空），不该走到这。
            return false;
    }
}

// 取某路径的 IDataObject（被右键选中对象的数据对象）+ 其所属文件夹的 pidl。
// 成功时 *ppdo 与 *ppidlFolder 都需调用方释放（pidlFolder 用 ILFree，
// pdtobj 用 Release）。
bool get_data_object(const std::wstring& path, IDataObject** ppdo,
                     PIDLIST_ABSOLUTE* ppidlFolder) {
    *ppdo = nullptr;
    *ppidlFolder = nullptr;

    PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(path.c_str());
    if (!pidl) return false;

    IShellFolder* psfParent = nullptr;
    PCUITEMID_CHILD pidlChild = nullptr;
    HRESULT hr = SHBindToParent(pidl, IID_IShellFolder,
                                reinterpret_cast<void**>(&psfParent), &pidlChild);
    if (FAILED(hr) || !psfParent) { ILFree(pidl); return false; }

    IDataObject* pdo = nullptr;
    hr = psfParent->GetUIObjectOf(nullptr, 1, &pidlChild, IID_IDataObject,
                                  nullptr, reinterpret_cast<void**>(&pdo));
    psfParent->Release();
    if (FAILED(hr) || !pdo) { ILFree(pidl); return false; }

    // IShellExtInit::Initialize 的 pidlFolder 规范上是「被右键对象所在的父文件夹」
    // 的 pidl，不是对象自身的绝对 pidl。严格的 handler（夸克AI）会校验它，传错
    // 直接 E_INVALIDARG。这里克隆绝对 pidl 后去掉最后一节，得到父文件夹 pidl。
    // pidlFolder must be the PARENT folder's pidl, not the object's own absolute
    // pidl. Strict handlers reject the latter with E_INVALIDARG.
    PIDLIST_ABSOLUTE pidlParent = ILCloneFull(pidl);
    ILFree(pidl);
    if (!pidlParent) { pdo->Release(); *ppdo = nullptr; return false; }
    ILRemoveLastID(pidlParent);   // 原地去掉末节 -> 父文件夹

    *ppdo = pdo;
    *ppidlFolder = pidlParent;
    return true;
}

// 取目录背景的 pidlFolder（文件夹自身的绝对 pidl，pdtobj=null）。
PIDLIST_ABSOLUTE get_folder_pidl(const std::wstring& path) {
    return ILCreateFromPathW(path.c_str());
}

// 打开场景对应的「类键」（HKCR 下），作为 IShellExtInit::Initialize 的 hkeyProgID。
// shell 真实调用时会把被右键对象的类键句柄传进来；严格的 handler（夸克AI）会校验
// 它非空，传 NULL 就 E_INVALIDARG。失败返回 NULL（调用方据此退回 NULL）。
// Open the scene's class key under HKCR for Initialize's hkeyProgID. Strict
// handlers (Quark) require a non-NULL key.
HKEY open_scene_class_key(MenuScene scene) {
    const wchar_t* sub = L"";
    switch (scene) {
        case MenuScene::AllFiles:            sub = L"*";                    break;
        case MenuScene::Directory:           sub = L"Directory";            break;
        case MenuScene::DirectoryBackground: sub = L"Directory\\Background"; break;
        case MenuScene::Drive:               sub = L"Drive";                break;
        case MenuScene::DesktopBackground:   sub = L"DesktopBackground";    break;
        default:                             return nullptr;
    }
    HKEY hk = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, sub, 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        return hk;
    }
    return nullptr;
}

// 读取一个 HMENU 项的文字（含 \t 后的快捷键），失败返回空串。
// MFT_OWNERDRAW 项无字符串，返回空（前端显示占位）。
std::wstring read_menu_text(HMENU hmenu, int pos) {
    wchar_t buf[512] = {0};
    MENUITEMINFOW mii = { sizeof(MENUITEMINFOW) };
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = buf;
    mii.cch = ARRAYSIZE(buf);
    if (!GetMenuItemInfoW(hmenu, (UINT)pos, TRUE, &mii)) return L"";
    return std::wstring(buf, mii.cch);
}

// 把一个 HMENU 递归序列化成 JSON 项数组（不含外层 []）。
// pcm2 用于子菜单懒填充（WM_INITMENUPOPUP）。out 持续追加。
void walk_menu(HMENU hmenu, IContextMenu2* pcm2, int depth, std::string& out) {
    if (!hmenu || depth > kMaxDepth) return;
    int count = GetMenuItemCount(hmenu);
    if (count <= 0) return;

    bool first = true;
    for (int i = 0; i < count; ++i) {
        MENUITEMINFOW mii = { sizeof(MENUITEMINFOW) };
        mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_SUBMENU | MIIM_ID;
        if (!GetMenuItemInfoW(hmenu, (UINT)i, TRUE, &mii)) continue;

        if (!first) out += ",";
        first = false;
        out += "{";

        // 分隔线。
        if (mii.fType & MFT_SEPARATOR) {
            out += "\"separator\":true";
            out += "}";
            continue;
        }

        // 文字（含 \t 快捷键），拆成 label + shortcut。
        std::wstring text = read_menu_text(hmenu, i);
        std::wstring label = text, shortcut;
        size_t tab = text.find(L'\t');
        if (tab != std::wstring::npos) {
            label = text.substr(0, tab);
            shortcut = text.substr(tab + 1);
        }

        bool ownerDraw = (mii.fType & MFT_OWNERDRAW) != 0;
        bool disabled = (mii.fState & (MFS_DISABLED | MFS_GRAYED)) != 0;

        out += "\"label\":\"";   out += json_escape(wide_to_utf8(label)); out += "\"";
        out += ",\"shortcut\":\"";out += json_escape(wide_to_utf8(shortcut)); out += "\"";
        out += ",\"separator\":false";
        out += ",\"disabled\":";  out += disabled ? "true" : "false";
        out += ",\"ownerDraw\":"; out += ownerDraw ? "true" : "false";

        // 子菜单：先让 handler 懒填充，再递归。
        if (mii.hSubMenu) {
            if (pcm2) {
                // shell 在弹出子菜单前发这两条消息触发 handler 填充。
                pcm2->HandleMenuMsg(WM_INITMENUPOPUP,
                                    reinterpret_cast<WPARAM>(mii.hSubMenu),
                                    MAKELPARAM(i, FALSE));
            }
            out += ",\"hasSubmenu\":true,\"children\":[";
            walk_menu(mii.hSubMenu, pcm2, depth + 1, out);
            out += "]";
        } else {
            out += ",\"hasSubmenu\":false,\"children\":[]";
        }
        out += "}";
    }
}

}  // namespace

std::string probe_handler_menu_json(MenuScene scene, const std::string& clsid,
                                    const std::string& overridePathUtf8) {
    // CLSID 解析。
    std::wstring wclsid(clsid.begin(), clsid.end());
    CLSID id;
    if (FAILED(CLSIDFromString(wclsid.c_str(), &id))) {
        return result_err("bad_clsid");
    }

    // shell 扩展多为 STA；本进程（CmmCli）专为探测而起，初始化 COM。
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool didInit = SUCCEEDED(hrInit);

    std::string result;
    {
        ScratchTarget target;
        if (!make_scratch(scene, &target, utf8_to_wide(overridePathUtf8))) {
            if (didInit) CoUninitialize();
            return result_err("scratch_failed");
        }

        // 取 pidlFolder / pdtobj。
        IDataObject* pdo = nullptr;
        PIDLIST_ABSOLUTE pidlFolder = nullptr;
        if (target.background) {
            // 目录背景：pidlFolder = 文件夹自身，pdtobj = null。
            pidlFolder = get_folder_pidl(target.path);
            if (!pidlFolder) {
                if (didInit) CoUninitialize();
                return result_err("data_object_failed");
            }
        } else {
            if (!get_data_object(target.path, &pdo, &pidlFolder)) {
                if (didInit) CoUninitialize();
                return result_err("data_object_failed");
            }
        }

        // 实例化 handler。
        IUnknown* punk = nullptr;
        HRESULT hr = CoCreateInstance(id, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_IUnknown, reinterpret_cast<void**>(&punk));
        if (FAILED(hr) || !punk) {
            if (pdo) pdo->Release();
            if (pidlFolder) ILFree(pidlFolder);
            if (didInit) CoUninitialize();
            return result_err("cocreate_failed");
        }

        // IShellExtInit::Initialize 喂上下文（多数 handler 必需）。
        HRESULT hrSei = 0x99999999, hrInitCall = 0x99999999;
        IShellExtInit* psei = nullptr;
        hrSei = punk->QueryInterface(IID_IShellExtInit,
                                     reinterpret_cast<void**>(&psei));
        if (SUCCEEDED(hrSei) && psei) {
            // IShellExtInit::Initialize 的传参：
            //   选中对象类（file/folder/drive）：pidlFolder = 父文件夹 pidl，
            //                                    pdtobj = 选中项数据对象。
            //   背景类（desktop/folder_bg）：    pidlFolder = 文件夹自身，pdtobj = NULL。
            // 关键：pidlFolder 必须是「父文件夹」，不是对象自身绝对 pidl。get_data_object
            // 已把它处理成父文件夹 pidl；背景类的 pidlFolder 本就是文件夹自身。
            // hkeyProgID 传场景类键（HKCR\* 等）——严格 handler（夸克AI）会校验它非空。
            HKEY hkeyProgID = open_scene_class_key(scene);
            hrInitCall = psei->Initialize(pidlFolder, pdo, hkeyProgID);
            if (hkeyProgID) RegCloseKey(hkeyProgID);
            psei->Release();
        }

        // 拿 IContextMenu。
        IContextMenu* pcm = nullptr;
        hr = punk->QueryInterface(IID_IContextMenu, reinterpret_cast<void**>(&pcm));
        if (FAILED(hr) || !pcm) {
            punk->Release();
            if (pdo) pdo->Release();
            if (pidlFolder) ILFree(pidlFolder);
            if (didInit) CoUninitialize();
            return result_err("no_icontextmenu");
        }

        // IContextMenu2/3 用于子菜单懒填充消息。
        IContextMenu2* pcm2 = nullptr;
        pcm->QueryInterface(IID_IContextMenu2, reinterpret_cast<void**>(&pcm2));

        // 填充菜单。CMF_EXTENDEDVERBS 拿到「Shift+右键」才出现的全集。
        HMENU hmenu = CreatePopupMenu();
        if (hmenu) {
            hr = pcm->QueryContextMenu(hmenu, 0, kCmdFirst, kCmdLast,
                                       CMF_NORMAL | CMF_EXTENDEDVERBS);
            // 部分 handler 在 QueryContextMenu 里只占位，真正填项延迟到 shell 弹出
            // 菜单前发的 WM_INITMENUPOPUP。这里对顶层菜单补发一次，触发懒填充。
            // Some handlers defer item creation to WM_INITMENUPOPUP; fire it for
            // the top-level menu to trigger lazy population.
            if (pcm2) {
                pcm2->HandleMenuMsg(WM_INITMENUPOPUP,
                                    reinterpret_cast<WPARAM>(hmenu),
                                    MAKELPARAM(0, FALSE));
            }
            // 顶层仍空时，部分 handler 不认 CMF_EXTENDEDVERBS，用纯 CMF_NORMAL 重试。
            // Retry with plain CMF_NORMAL for handlers that reject extended verbs.
            if (GetMenuItemCount(hmenu) <= 0) {
                pcm->QueryContextMenu(hmenu, 0, kCmdFirst, kCmdLast, CMF_NORMAL);
                if (pcm2) {
                    pcm2->HandleMenuMsg(WM_INITMENUPOPUP,
                                        reinterpret_cast<WPARAM>(hmenu),
                                        MAKELPARAM(0, FALSE));
                }
            }
            // QueryContextMenu 成功返回 SEVERITY_SUCCESS 的 HRESULT，HRESULT_CODE
            // 是添加的命令数。即便返回 0 项也照样遍历（可能是占位/owner-draw）。
            // hrSei/hrInitCall 暂未对外暴露，留作将来诊断（避免未使用告警）。
            (void)hrSei; (void)hrInitCall;
            std::string items;
            walk_menu(hmenu, pcm2, 0, items);

            result = "{\"ok\":true,\"items\":[";
            result += items;
            result += "]}";

            DestroyMenu(hmenu);
        } else {
            result = result_err("menu_failed");
        }

        if (pcm2) pcm2->Release();
        pcm->Release();
        punk->Release();
        if (pdo) pdo->Release();
        if (pidlFolder) ILFree(pidlFolder);
    }

    if (didInit) CoUninitialize();
    return result;
}

}  // namespace cmm
