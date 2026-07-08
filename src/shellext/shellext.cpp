// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 富媒体 Shell 扩展 DLL 实现（阶段 2 完整版）
//
//  见 shellext.h。本文件实现一个进程内 COM shell 扩展，含阶段 2 全部三步：
//    · CCmmContextMenu   —— 同时实现 IShellExtInit + IContextMenu(2/3)
//    · CClassFactory     —— IClassFactory，负责造上面的对象
//    · DLL 四导出        —— DllGetClassObject / DllCanUnloadNow /
//                           DllRegisterServer / DllUnregisterServer
//    · DllMain           —— 存模块句柄
//
//  step 1 地基：右键指定类型文件 → 出项 → 点击回显路径（证明链路通）。
//  step 2 owner-draw：菜单项 MFT_OWNERDRAW，HandleMenuMsg2 处理 WM_MEASUREITEM /
//         WM_DRAWITEM，自绘「缩略图 + 文字」，选中高亮。图片类型抽真实缩略图
//         （IShellItemImageFactory），非图片回退大图标（SHGetFileInfo）。
//  step 3 动态子菜单：右键压缩包 / 文件夹时，现场读内容（压缩包走 shell 名字空间
//         枚举，文件夹走 FindFirstFile）动态塞子菜单项，最多 N 条 + 省略。
//
//  ⚠ 与 com_menu_probe.{h,cpp} 相反：那是「读别人的扩展」，本模块是「造自己的」。
// ============================================================
#include "shellext/shellext.h"

#include <shlwapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <strsafe.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "gdi32.lib")

// ---- CLSID 实体（头里只 extern 声明，实体在此分配）----
//  {A7E4C9F1-3D82-4B6E-9C15-2F8A6D5E1B93}
EXTERN_C const CLSID CLSID_CmmShellExt = {
    0xA7E4C9F1, 0x3D82, 0x4B6E, {0x9C, 0x15, 0x2F, 0x8A, 0x6D, 0x5E, 0x1B, 0x93}};

// ---- 进程内全局状态（声明见 shellext.h）----
long      g_cmmDllRefCount = 0;
HINSTANCE g_cmmDllInstance = nullptr;

namespace {

// step 1 出现在右键菜单里的顶层项文字（后续做多语言再抽出）。
constexpr wchar_t kMenuItemText[] = L"ContextMenuMaster";
// InvokeCommand 里用于匹配「就是我们这一项」的动词字符串（ANSI 调用路径用）。
constexpr char    kVerbA[]        = "cmm_richmedia";

// owner-draw 布局常量。
constexpr int kThumbSize   = 32;   // 缩略图边长（像素）
constexpr int kPadX        = 6;    // 缩略图左右留白
constexpr int kPadY        = 4;    // 项上下留白
constexpr int kTextGap     = 8;    // 缩略图与文字间距
constexpr int kTextMinW    = 160;  // 文字区最小宽（保证够长文件名）

// step 3 动态子菜单：最多列出多少条内容，超出显示省略项。
constexpr int kMaxSubItems = 20;

// -------- CLSID -> 注册表字符串 "{...}" --------
std::wstring clsid_to_wstring(REFCLSID clsid) {
    LPOLESTR s = nullptr;
    if (FAILED(StringFromCLSID(clsid, &s)) || !s) return L"";
    std::wstring out(s);
    CoTaskMemFree(s);
    return out;
}

// -------- 取本 DLL 自身磁盘路径（注册 InprocServer32 用）--------
std::wstring self_module_path() {
    wchar_t buf[MAX_PATH] = {0};
    DWORD n = GetModuleFileNameW(g_cmmDllInstance, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return L"";
    return std::wstring(buf, n);
}

// -------- 写一个注册表字符串值（REG_SZ），失败返回 false --------
bool reg_set_sz(HKEY root, const std::wstring& subkey,
                const wchar_t* valueName, const std::wstring& data) {
    HKEY h = nullptr;
    LONG rc = RegCreateKeyExW(root, subkey.c_str(), 0, nullptr,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &h, nullptr);
    if (rc != ERROR_SUCCESS) return false;
    rc = RegSetValueExW(h, valueName, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(data.c_str()),
                        static_cast<DWORD>((data.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(h);
    return rc == ERROR_SUCCESS;
}

// -------- 取扩展名（小写，含点），无则空 --------
std::wstring lower_ext(const std::wstring& path) {
    size_t dot = path.find_last_of(L'.');
    size_t sep = path.find_last_of(L"\\/");
    if (dot == std::wstring::npos || (sep != std::wstring::npos && dot < sep)) return L"";
    std::wstring e = path.substr(dot);
    std::transform(e.begin(), e.end(), e.begin(), ::towlower);
    return e;
}

// -------- 是否图片类型（step 2 抽真实缩略图用）--------
bool is_image_ext(const std::wstring& ext) {
    static const wchar_t* kImg[] = {L".png", L".jpg", L".jpeg", L".bmp", L".gif",
                                    L".webp", L".tif", L".tiff", L".ico"};
    for (auto* e : kImg) if (ext == e) return true;
    return false;
}

// -------- 是否压缩包类型（step 3 动态子菜单用）--------
bool is_archive_ext(const std::wstring& ext) {
    static const wchar_t* kArc[] = {L".zip", L".7z", L".rar", L".tar", L".gz",
                                    L".cab", L".iso"};
    for (auto* e : kArc) if (ext == e) return true;
    return false;
}

// -------- 是否目录 --------
bool is_directory(const std::wstring& path) {
    DWORD a = GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

// ============================================================
//  缩略图缓存条目：一个 HBITMAP + 尺寸。析构统一 DeleteObject。
// ============================================================
struct Thumb {
    HBITMAP hbmp = nullptr;
    int     w    = 0;
    int     h    = 0;
    ~Thumb() { if (hbmp) DeleteObject(hbmp); }
};

// -------- 为文件抽一张缩略图（图片抽真实缩略图，其余抽大图标）--------
//  返回 32bpp 预乘或普通 DIB 的 HBITMAP；失败返回 nullptr。调用方负责 DeleteObject。
HBITMAP make_thumbnail(const std::wstring& path, int size) {
    // 图片 / 有缩略图的类型：IShellItemImageFactory 拿系统缩略图（含照片实图）。
    IShellItemImageFactory* factory = nullptr;
    HBITMAP hbmp = nullptr;
    if (SUCCEEDED(SHCreateItemFromParsingName(path.c_str(), nullptr,
                                              IID_PPV_ARGS(&factory))) && factory) {
        SIZE sz = {size, size};
        // SIIGBF_RESIZETOFIT 等比缩放贴合；BIGGERSIZEOK 允许略大后我们再缩。
        HRESULT hr = factory->GetImage(sz, SIIGBF_RESIZETOFIT | SIIGBF_BIGGERSIZEOK, &hbmp);
        factory->Release();
        if (SUCCEEDED(hr) && hbmp) return hbmp;
        hbmp = nullptr;
    }
    // 回退：SHGetFileInfo 取大图标，画进一张 DIB。
    SHFILEINFOW sfi = {};
    if (SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi),
                       SHGFI_ICON | SHGFI_LARGEICON) && sfi.hIcon) {
        HDC screen = GetDC(nullptr);
        HDC mem    = CreateCompatibleDC(screen);
        HBITMAP bmp = CreateCompatibleBitmap(screen, size, size);
        HGDIOBJ old = SelectObject(mem, bmp);
        RECT rc = {0, 0, size, size};
        FillRect(mem, &rc, (HBRUSH)(COLOR_MENU + 1));
        DrawIconEx(mem, 0, 0, sfi.hIcon, size, size, 0, nullptr, DI_NORMAL);
        SelectObject(mem, old);
        DeleteDC(mem);
        ReleaseDC(nullptr, screen);
        DestroyIcon(sfi.hIcon);
        return bmp;
    }
    return nullptr;
}

// -------- step 3：读压缩包内条目名（走 shell 名字空间，兼容系统能打开的类型）--------
//  失败或非可枚举返回空。只取顶层若干条。
std::vector<std::wstring> list_archive_entries(const std::wstring& path, int maxN) {
    std::vector<std::wstring> out;
    IShellItem* item = nullptr;
    if (FAILED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&item))) || !item)
        return out;

    IEnumShellItems* en = nullptr;
    // BindToHandler(BHID_StorageEnum) 把压缩包当作可枚举容器（系统 zip 支持）。
    if (SUCCEEDED(item->BindToHandler(nullptr, BHID_StorageEnum, IID_PPV_ARGS(&en))) && en) {
        IShellItem* child = nullptr;
        while (out.size() < static_cast<size_t>(maxN) &&
               en->Next(1, &child, nullptr) == S_OK && child) {
            LPWSTR name = nullptr;
            if (SUCCEEDED(child->GetDisplayName(SIGDN_NORMALDISPLAY, &name)) && name) {
                out.emplace_back(name);
                CoTaskMemFree(name);
            }
            child->Release();
            child = nullptr;
        }
        en->Release();
    }
    item->Release();
    return out;
}

// -------- step 3：读文件夹顶层条目名 --------
std::vector<std::wstring> list_folder_entries(const std::wstring& dir, int maxN) {
    std::vector<std::wstring> out;
    std::wstring pat = dir;
    if (!pat.empty() && pat.back() != L'\\') pat += L'\\';
    pat += L"*";

    WIN32_FIND_DATAW fd = {};
    HANDLE h = FindFirstFileW(pat.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        out.emplace_back(fd.cFileName);
        if (out.size() >= static_cast<size_t>(maxN)) break;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return out;
}

}  // namespace

// ============================================================
//  CCmmContextMenu —— IShellExtInit + IContextMenu3
// ============================================================
class CCmmContextMenu : public IShellExtInit, public IContextMenu3 {
public:
    CCmmContextMenu() : ref_(1) { InterlockedIncrement(&g_cmmDllRefCount); }
    ~CCmmContextMenu() {
        if (thumb_) DeleteObject(thumb_);
        InterlockedDecrement(&g_cmmDllRefCount);
    }

    // ---- IUnknown ----
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        static const QITAB qit[] = {
            QITABENT(CCmmContextMenu, IShellExtInit),
            QITABENT(CCmmContextMenu, IContextMenu),
            QITABENT(CCmmContextMenu, IContextMenu2),
            QITABENT(CCmmContextMenu, IContextMenu3),
            {nullptr, 0},
        };
        return QISearch(this, qit, riid, ppv);
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
    IFACEMETHODIMP_(ULONG) Release() override {
        long r = InterlockedDecrement(&ref_);
        if (r == 0) delete this;
        return r;
    }

    // ---- IShellExtInit：explorer 喂上下文，我们抽出被右键的文件路径 ----
    IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE /*pidlFolder*/,
                              IDataObject* pdtobj,
                              HKEY /*hkeyProgID*/) override {
        files_.clear();
        if (!pdtobj) return E_INVALIDARG;

        FORMATETC fmt = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM stg = {};
        if (FAILED(pdtobj->GetData(&fmt, &stg))) return E_INVALIDARG;

        HDROP hdrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
        if (hdrop) {
            UINT count = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < count; ++i) {
                wchar_t path[MAX_PATH] = {0};
                if (DragQueryFileW(hdrop, i, path, MAX_PATH)) files_.emplace_back(path);
            }
            GlobalUnlock(stg.hGlobal);
        }
        ReleaseStgMedium(&stg);
        return files_.empty() ? E_INVALIDARG : S_OK;
    }

    // ---- IContextMenu：往 explorer 的 HMENU 里塞我们的项 ----
    IFACEMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu,
                                    UINT idCmdFirst, UINT idCmdLast,
                                    UINT uFlags) override {
        if (uFlags & CMF_DEFAULTONLY) return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);

        idCmdFirst_ = idCmdFirst;
        subLabels_.clear();

        // 决定形态：单文件且（压缩包 / 目录）→ 动态子菜单（step 3）；否则单项。
        bool useSubmenu = false;
        if (files_.size() == 1) {
            const std::wstring& f = files_[0];
            std::wstring ext = lower_ext(f);
            if (is_directory(f)) {
                subLabels_ = list_folder_entries(f, kMaxSubItems);
                useSubmenu = !subLabels_.empty();
            } else if (is_archive_ext(ext)) {
                subLabels_ = list_archive_entries(f, kMaxSubItems);
                useSubmenu = !subLabels_.empty();
            }
        }

        // step 2：为单文件预备缩略图（owner-draw 顶层项要画）。
        if (thumb_) { DeleteObject(thumb_); thumb_ = nullptr; }
        if (files_.size() == 1) thumb_ = make_thumbnail(files_[0], kThumbSize);

        UINT idNext = idCmdFirst;

        if (useSubmenu) {
            // 顶层项带一个子菜单。顶层 owner-draw（画缩略图 + 名称）。
            HMENU sub = CreatePopupMenu();
            UINT subIdBase = idCmdFirst + 1;  // 子项 id 从顶层项 id 之后开始
            for (size_t i = 0; i < subLabels_.size(); ++i) {
                MENUITEMINFOW si = {sizeof(si)};
                si.fMask      = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
                si.fType      = MFT_STRING;
                si.wID        = static_cast<UINT>(subIdBase + i);
                si.dwTypeData = const_cast<wchar_t*>(subLabels_[i].c_str());
                InsertMenuItemW(sub, static_cast<UINT>(i), TRUE, &si);
            }
            // 内容多于上限时补一条省略提示（不可点，disabled）。
            std::wstring top = kMenuItemText;

            MENUITEMINFOW mii = {sizeof(mii)};
            mii.fMask      = MIIM_ID | MIIM_FTYPE | MIIM_SUBMENU | MIIM_DATA | MIIM_STRING;
            mii.fType      = MFT_OWNERDRAW;
            mii.wID        = idCmdFirst;            // 顶层项自身 id
            mii.hSubMenu   = sub;
            mii.dwItemData = reinterpret_cast<ULONG_PTR>(this);  // owner-draw 回本对象
            mii.dwTypeData = const_cast<wchar_t*>(top.c_str());  // 保留文字备用
            if (!InsertMenuItemW(hmenu, indexMenu, TRUE, &mii)) {
                DestroyMenu(sub);
                return HRESULT_FROM_WIN32(GetLastError());
            }
            // 用掉的 id：顶层 1 个 + 子项 subLabels_ 个。
            idNext = subIdBase + static_cast<UINT>(subLabels_.size());
        } else {
            // 单项，owner-draw（画缩略图 + 名称）。
            MENUITEMINFOW mii = {sizeof(mii)};
            mii.fMask      = MIIM_ID | MIIM_FTYPE | MIIM_DATA | MIIM_STRING;
            mii.fType      = MFT_OWNERDRAW;
            mii.wID        = idCmdFirst;
            mii.dwItemData = reinterpret_cast<ULONG_PTR>(this);
            mii.dwTypeData = const_cast<wchar_t*>(kMenuItemText);
            if (!InsertMenuItemW(hmenu, indexMenu, TRUE, &mii))
                return HRESULT_FROM_WIN32(GetLastError());
            idNext = idCmdFirst + 1;
        }

        UINT used = idNext - idCmdFirst;
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, used);
    }

    // ---- 用户点了我们的项 ----
    IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici) override {
        if (!pici) return E_INVALIDARG;

        UINT offset = 0;
        bool byVerb = false;
        if (HIWORD(pici->lpVerb)) {
            if (lstrcmpA(pici->lpVerb, kVerbA) != 0) return E_FAIL;
            byVerb = true;
        } else {
            offset = LOWORD(pici->lpVerb);
        }

        // offset 0 = 顶层项；>=1 = 子项（内容项）。
        std::wstring msg;
        if (byVerb || offset == 0) {
            msg = L"ContextMenuMaster 选中：\n";
            for (const auto& f : files_) { msg += f; msg += L"\n"; }
        } else {
            size_t idx = static_cast<size_t>(offset - 1);
            if (idx < subLabels_.size())
                msg = L"打开内容项：\n" + subLabels_[idx];
            else
                return E_FAIL;
        }
        MessageBoxW(pici->hwnd, msg.c_str(), L"ContextMenuMaster", MB_OK | MB_ICONINFORMATION);
        return S_OK;
    }

    // ---- 状态栏提示文字 / verb 反查 ----
    IFACEMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT* /*pReserved*/,
                                    LPSTR pszName, UINT cchMax) override {
        if (idCmd != 0) return E_INVALIDARG;  // 仅顶层项支持 verb 反查
        switch (uType) {
            case GCS_VERBA:
                return StringCchCopyA(pszName, cchMax, kVerbA);
            case GCS_VERBW:
                return StringCchCopyW(reinterpret_cast<LPWSTR>(pszName), cchMax, L"cmm_richmedia");
            case GCS_HELPTEXTA:
                return StringCchCopyA(pszName, cchMax, "ContextMenuMaster rich-media action");
            case GCS_HELPTEXTW:
                return StringCchCopyW(reinterpret_cast<LPWSTR>(pszName), cchMax,
                                      L"ContextMenuMaster 富媒体动作");
            default:
                return E_INVALIDARG;
        }
    }

    // ---- IContextMenu2/3：owner-draw 消息中转 ----
    IFACEMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override {
        LRESULT res = 0;
        return HandleMenuMsg2(uMsg, wParam, lParam, &res);
    }
    IFACEMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM /*wParam*/, LPARAM lParam,
                                  LRESULT* plResult) override {
        switch (uMsg) {
            case WM_MEASUREITEM: {
                auto* mis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
                if (!mis || mis->CtlType != ODT_MENU) break;
                if (mis->itemData != reinterpret_cast<ULONG_PTR>(this)) break;
                onMeasureItem(mis);
                if (plResult) *plResult = TRUE;
                return S_OK;
            }
            case WM_DRAWITEM: {
                auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                if (!dis || dis->CtlType != ODT_MENU) break;
                if (dis->itemData != reinterpret_cast<ULONG_PTR>(this)) break;
                onDrawItem(dis);
                if (plResult) *plResult = TRUE;
                return S_OK;
            }
            default:
                break;
        }
        if (plResult) *plResult = 0;
        return S_OK;
    }

private:
    // -------- owner-draw：报告顶层项尺寸 --------
    void onMeasureItem(MEASUREITEMSTRUCT* mis) {
        HDC dc = GetDC(nullptr);
        // 文字宽度按项名量。
        SIZE ext = {0, 0};
        HFONT menuFont = menu_font();
        HGDIOBJ old = menuFont ? SelectObject(dc, menuFont) : nullptr;
        GetTextExtentPoint32W(dc, kMenuItemText, lstrlenW(kMenuItemText), &ext);
        if (old) SelectObject(dc, old);
        ReleaseDC(nullptr, dc);
        if (menuFont) DeleteObject(menuFont);

        int textW = (std::max)(static_cast<int>(ext.cx), kTextMinW);
        mis->itemWidth  = kPadX + kThumbSize + kTextGap + textW + kPadX;
        mis->itemHeight = (std::max)(kThumbSize + kPadY * 2, static_cast<int>(ext.cy) + kPadY * 2);
    }

    // -------- owner-draw：自绘顶层项（缩略图 + 文字 + 选中高亮）--------
    void onDrawItem(DRAWITEMSTRUCT* dis) {
        HDC dc = dis->hDC;
        RECT rc = dis->rcItem;
        bool selected = (dis->itemState & ODS_SELECTED) != 0;

        // 背景：选中用高亮色，否则菜单底色。
        HBRUSH bg = GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_MENU);
        FillRect(dc, &rc, bg);

        // 缩略图（左侧居中）。
        int thumbY = rc.top + ((rc.bottom - rc.top) - kThumbSize) / 2;
        int thumbX = rc.left + kPadX;
        if (thumb_) {
            HDC mem = CreateCompatibleDC(dc);
            HGDIOBJ oldb = SelectObject(mem, thumb_);
            // 32bpp 缩略图带 alpha，用 AlphaBlend 保透明；失败退 BitBlt。
            BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
            if (!AlphaBlend(dc, thumbX, thumbY, kThumbSize, kThumbSize,
                            mem, 0, 0, kThumbSize, kThumbSize, bf)) {
                BitBlt(dc, thumbX, thumbY, kThumbSize, kThumbSize, mem, 0, 0, SRCCOPY);
            }
            SelectObject(mem, oldb);
            DeleteDC(mem);
        }

        // 文字（缩略图右侧，垂直居中）。
        int textX = thumbX + kThumbSize + kTextGap;
        RECT tr = {textX, rc.top, rc.right - kPadX, rc.bottom};
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
        HFONT menuFont = menu_font();
        HGDIOBJ oldf = menuFont ? SelectObject(dc, menuFont) : nullptr;
        DrawTextW(dc, kMenuItemText, -1, &tr,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        if (oldf) SelectObject(dc, oldf);
        if (menuFont) DeleteObject(menuFont);
    }

    // -------- 取当前菜单字体（NONCLIENTMETRICS）；调用方负责 DeleteObject --------
    static HFONT menu_font() {
        NONCLIENTMETRICSW ncm = {sizeof(ncm)};
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
            return CreateFontIndirectW(&ncm.lfMenuFont);
        return nullptr;
    }

    long ref_;
    std::vector<std::wstring> files_;
    std::vector<std::wstring> subLabels_;   // step 3 动态子项文字
    HBITMAP thumb_ = nullptr;               // step 2 顶层项缩略图
    UINT idCmdFirst_ = 0;                   // QueryContextMenu 起始 id
};

// ============================================================
//  CClassFactory —— 造 CCmmContextMenu
// ============================================================
class CClassFactory : public IClassFactory {
public:
    CClassFactory() : ref_(1) { InterlockedIncrement(&g_cmmDllRefCount); }
    ~CClassFactory() { InterlockedDecrement(&g_cmmDllRefCount); }

    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
    IFACEMETHODIMP_(ULONG) Release() override {
        long r = InterlockedDecrement(&ref_);
        if (r == 0) delete this;
        return r;
    }

    IFACEMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) override {
        *ppv = nullptr;
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;  // 不支持聚合
        CCmmContextMenu* obj = new (std::nothrow) CCmmContextMenu();
        if (!obj) return E_OUTOFMEMORY;
        HRESULT hr = obj->QueryInterface(riid, ppv);
        obj->Release();
        return hr;
    }
    IFACEMETHODIMP LockServer(BOOL fLock) override {
        if (fLock) InterlockedIncrement(&g_cmmDllRefCount);
        else       InterlockedDecrement(&g_cmmDllRefCount);
        return S_OK;
    }

private:
    long ref_;
};

// ============================================================
//  DLL 导出
// ============================================================
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (rclsid != CLSID_CmmShellExt) return CLASS_E_CLASSNOTAVAILABLE;
    CClassFactory* factory = new (std::nothrow) CClassFactory();
    if (!factory) return E_OUTOFMEMORY;
    HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();
    return hr;
}

STDAPI DllCanUnloadNow(void) {
    return (g_cmmDllRefCount == 0) ? S_OK : S_FALSE;
}

// -------- 自注册：写 CLSID\InprocServer32；调用方（注册脚本/CLI）再把本扩展
//          挂到具体文件类型的 ContextMenuHandlers 下。这里只登记 COM 服务器本身。
STDAPI DllRegisterServer(void) {
    const std::wstring clsid = clsid_to_wstring(CLSID_CmmShellExt);
    if (clsid.empty()) return E_FAIL;
    const std::wstring dllPath = self_module_path();
    if (dllPath.empty()) return E_FAIL;

    const std::wstring clsidKey = L"CLSID\\" + clsid;
    if (!reg_set_sz(HKEY_CLASSES_ROOT, clsidKey, nullptr, CMM_SHELLEXT_FRIENDLY_NAME))
        return E_ACCESSDENIED;
    if (!reg_set_sz(HKEY_CLASSES_ROOT, clsidKey + L"\\InprocServer32", nullptr, dllPath))
        return E_ACCESSDENIED;
    // 进程内 COM 用 Apartment 线程模型（shell 扩展惯例）。
    if (!reg_set_sz(HKEY_CLASSES_ROOT, clsidKey + L"\\InprocServer32",
                    L"ThreadingModel", L"Apartment"))
        return E_ACCESSDENIED;

    // 挂到 HKCR\*\shellex\ContextMenuHandlers\<name>（默认值 = CLSID）→ 右键任意
    // 文件时 explorer 加载本扩展。验证期挂 *（全类型）；生产可改挂具体类型。
    const std::wstring handlerKey =
        std::wstring(L"*\\shellex\\ContextMenuHandlers\\") + CMM_SHELLEXT_HANDLER_NAME;
    if (!reg_set_sz(HKEY_CLASSES_ROOT, handlerKey, nullptr, clsid))
        return E_ACCESSDENIED;

    // 通知 shell 关联已变（图标/菜单缓存刷新）。
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return S_OK;
}

STDAPI DllUnregisterServer(void) {
    const std::wstring clsid = clsid_to_wstring(CLSID_CmmShellExt);
    if (clsid.empty()) return E_FAIL;
    const std::wstring clsidKey = L"CLSID\\" + clsid;
    // 先摘 * 下的 handler 挂载，再删整棵 CLSID 子树（含 InprocServer32）。
    const std::wstring handlerKey =
        std::wstring(L"*\\shellex\\ContextMenuHandlers\\") + CMM_SHELLEXT_HANDLER_NAME;
    RegDeleteTreeW(HKEY_CLASSES_ROOT, handlerKey.c_str());
    RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidKey.c_str());
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return S_OK;
}

// ============================================================
//  DllMain —— 存模块句柄，关线程通知（性能）
// ============================================================
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID /*reserved*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_cmmDllInstance = hInst;
        DisableThreadLibraryCalls(hInst);
    }
    return TRUE;
}
