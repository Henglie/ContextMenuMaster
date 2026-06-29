// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 系统图标提取（实现）
// ============================================================
#include "platform/windows/icon_extract.h"

#include <windows.h>
#include <shlobj.h>       // SHGetStockIconInfo / SHSTOCKICONINFO
#include <wincrypt.h>     // CryptBinaryToStringA（base64）
#include <gdiplus.h>
#include <vector>
#include <string>
#include <mutex>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "crypt32.lib")

namespace cmm {

namespace {

// —— UTF-8 <-> UTF-16 小工具（系统 API 全走宽字符，路径含中文也安全）——
std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

// 取 GDI+ 某编码器（这里只需 PNG）的 CLSID。失败返回 false。
bool png_encoder_clsid(CLSID* clsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return false;
    std::vector<BYTE> buf(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
    Gdiplus::GetImageEncoders(num, size, codecs);
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(codecs[i].MimeType, L"image/png") == 0) {
            *clsid = codecs[i].Clsid;
            return true;
        }
    }
    return false;
}

// 把一段二进制编码成 base64（CryptBinaryToStringA，无换行）。
std::string base64_encode(const BYTE* data, DWORD len) {
    DWORD chars = 0;
    if (!CryptBinaryToStringA(data, len,
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &chars)) {
        return "";
    }
    std::string out(chars, '\0');
    if (!CryptBinaryToStringA(data, len,
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, out.data(), &chars)) {
        return "";
    }
    // CryptBinaryToStringA 把结尾计入了终止符，去掉尾部多余的 \0。
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

// 把 HICON 渲染为 32bpp BGRA 的 GDI+ 位图，alpha 保真。
// 走 GetIconInfo + GetDIBits 手动取色位，避开 FromHICON 丢 alpha 出黑底的坑。
// 老式无 alpha 图标（全 0 alpha）用 AND 掩码位重建透明，避免整张变透明。
Gdiplus::Bitmap* hicon_to_bitmap(HICON hIcon, int px) {
    ICONINFO ii = {0};
    if (!GetIconInfo(hIcon, &ii)) return nullptr;

    // 目标 px x px 的 32bpp DIB section。
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = px;
    bmi.bmiHeader.biHeight = -px;   // 负高 = 自上而下，行序与 GDI+ 一致
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(nullptr);
    void* bits = nullptr;
    HBITMAP dib = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib) {
        ReleaseDC(nullptr, screen);
        if (ii.hbmColor) DeleteObject(ii.hbmColor);
        if (ii.hbmMask)  DeleteObject(ii.hbmMask);
        return nullptr;
    }

    HDC memdc = CreateCompatibleDC(screen);
    HGDIOBJ old = SelectObject(memdc, dib);
    // DrawIconEx 缩放到目标尺寸绘制（DI_NORMAL 会合成 alpha/掩码）。
    DrawIconEx(memdc, 0, 0, hIcon, px, px, 0, nullptr, DI_NORMAL);
    SelectObject(memdc, old);

    // 检查 DrawIconEx 后是否有任何非零 alpha；全 0 说明是无 alpha 老图标，
    // 需要靠 AND 掩码把「不透明区」的 alpha 补成 255，否则后面 PNG 全透明。
    auto* px32 = reinterpret_cast<DWORD*>(bits);
    bool hasAlpha = false;
    for (int i = 0; i < px * px; ++i) {
        if ((px32[i] & 0xFF000000u) != 0) { hasAlpha = true; break; }
    }
    if (!hasAlpha && ii.hbmMask) {
        // 取掩码位：掩码=0 处为不透明，置 alpha=255。
        std::vector<BYTE> maskBits(px * px * 4);
        BITMAPINFO mbi = bmi;   // 同样 px、32bpp、自上而下
        GetDIBits(memdc, ii.hbmMask, 0, px, maskBits.data(), &mbi, DIB_RGB_COLORS);
        auto* m = reinterpret_cast<DWORD*>(maskBits.data());
        for (int i = 0; i < px * px; ++i) {
            bool transparent = (m[i] & 0x00FFFFFFu) != 0;  // 掩码非 0 = 透明
            if (!transparent) px32[i] |= 0xFF000000u;
        }
    }

    // 构造一个「拥有自己像素」的 GDI+ 位图，再用 LockBits 把 DIB 的 BGRA 像素
    // 逐行拷进去。
    //
    // 为什么不能用 `Bitmap(w,h,stride,fmt,scan0).Clone()`：那种构造法让 Bitmap
    // 直接引用外部 scan0（即这里的 DIB bits）；Clone 在部分 GDI+ 版本下仍是浅引用，
    // 不深拷像素。一旦本函数末尾 DeleteObject(dib) 释放了 bits，之后 Save 访问已
    // 释放内存 → segfault（本项目第一次跑就是栽在这）。LockBits 写入自有缓冲后，
    // 位图与 DIB 彻底解耦，DIB 释放后依然安全。
    auto* result = new Gdiplus::Bitmap(px, px, PixelFormat32bppARGB);
    Gdiplus::BitmapData bd;
    Gdiplus::Rect rect(0, 0, px, px);
    if (result->LockBits(&rect, Gdiplus::ImageLockModeWrite,
                         PixelFormat32bppARGB, &bd) == Gdiplus::Ok) {
        const BYTE* srcRow = reinterpret_cast<const BYTE*>(bits);
        BYTE* dstRow = reinterpret_cast<BYTE*>(bd.Scan0);
        for (int y = 0; y < px; ++y) {
            memcpy(dstRow, srcRow, px * 4);   // 两边都是 32bpp 自上而下，行宽一致
            srcRow += px * 4;
            dstRow += bd.Stride;
        }
        result->UnlockBits(&bd);
    } else {
        delete result;
        result = nullptr;
    }

    DeleteDC(memdc);
    DeleteObject(dib);
    ReleaseDC(nullptr, screen);
    if (ii.hbmColor) DeleteObject(ii.hbmColor);
    if (ii.hbmMask)  DeleteObject(ii.hbmMask);
    return result;
}

// 把 GDI+ 位图编码成内存 PNG，返回字节。失败返回空。
std::vector<BYTE> bitmap_to_png(Gdiplus::Bitmap* bmp) {
    std::vector<BYTE> out;
    CLSID clsid;
    if (!png_encoder_clsid(&clsid)) return out;

    IStream* stream = nullptr;
    if (CreateStreamOnHGlobal(nullptr, TRUE, &stream) != S_OK) return out;

    if (bmp->Save(stream, &clsid, nullptr) != Gdiplus::Ok) {
        stream->Release();
        return out;
    }
    // 从流取回字节。
    HGLOBAL hg = nullptr;
    GetHGlobalFromStream(stream, &hg);
    SIZE_T size = GlobalSize(hg);
    void* p = GlobalLock(hg);
    if (p && size) {
        out.resize(size);
        memcpy(out.data(), p, size);
    }
    GlobalUnlock(hg);
    stream->Release();
    return out;
}

// 解析 "stock:NN" → 用 SHGetStockIconInfo 取跨版本稳定的系统 stock 图标。
// 这类图标（回收站/文件夹/盾牌等）资源 ID 不随版本漂移，是最可靠的图标源。
HICON load_stock_icon(int stockId, int px) {
    SHSTOCKICONINFO sii = { sizeof(SHSTOCKICONINFO) };
    UINT flags = SHGSI_ICON | (px >= 48 ? SHGSI_LARGEICON : SHGSI_SMALLICON);
    if (SUCCEEDED(SHGetStockIconInfo((SHSTOCKICONID)stockId, flags, &sii))) {
        return sii.hIcon;   // 调用方负责 DestroyIcon
    }
    return nullptr;
}

// 解析 "file.dll,-id" / "file.dll,index" / 纯路径，用 PrivateExtractIconsW 提取。
// 负数=资源 ID（匹配注册表写法），正数=零基索引，省略=取第 0 个。
HICON load_module_icon(const std::wstring& spec, int px) {
    // 拆出路径与索引（最后一个逗号分隔；路径本身可能含逗号则不拆——系统路径罕见）。
    std::wstring path = spec;
    int index = 0;
    size_t comma = spec.find_last_of(L',');
    if (comma != std::wstring::npos) {
        // 逗号后必须是合法整数才认作索引，否则当成路径一部分。
        wchar_t* end = nullptr;
        long v = wcstol(spec.c_str() + comma + 1, &end, 10);
        if (end && *end == L'\0') {
            path = spec.substr(0, comma);
            index = (int)v;
        }
    }

    HICON icons[1] = { nullptr };
    // PrivateExtractIconsW：第 4 参 nIconIndex 支持负数资源 ID，第 5 参 = 1 取一个。
    UINT got = PrivateExtractIconsW(path.c_str(), index, px, px,
                                    icons, nullptr, 1, 0);
    if (got == 0 || !icons[0]) return nullptr;
    return icons[0];   // 调用方负责 DestroyIcon
}

}  // namespace

void ensure_gdiplus() {
    static std::once_flag flag;
    static ULONG_PTR token = 0;
    std::call_once(flag, [] {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup(&token, &input, nullptr);
        // 不注册 shutdown：token 随进程生命周期存在，进程退出时 OS 回收。
    });
}

namespace {

// 共享路径：把「图标源 + 像素」提取为内存 PNG 字节。失败返回空。
// extract_icon_data_uri 与 extract_icon_to_file 都走这里，避免重复逻辑。
std::vector<BYTE> source_to_png(const std::string& source, int px) {
    std::vector<BYTE> empty;
    if (source.empty() || px <= 0 || px > 256) return empty;
    ensure_gdiplus();

    HICON hIcon = nullptr;
    const std::string stockPrefix = "stock:";
    if (source.compare(0, stockPrefix.size(), stockPrefix) == 0) {
        int id = atoi(source.c_str() + stockPrefix.size());
        hIcon = load_stock_icon(id, px);
    } else {
        hIcon = load_module_icon(utf8_to_wide(source), px);
    }
    if (!hIcon) return empty;

    Gdiplus::Bitmap* bmp = hicon_to_bitmap(hIcon, px);
    DestroyIcon(hIcon);
    if (!bmp) return empty;

    std::vector<BYTE> png = bitmap_to_png(bmp);
    delete bmp;
    return png;
}

}  // namespace

std::string extract_icon_data_uri(const std::string& source, int px) {
    std::vector<BYTE> png = source_to_png(source, px);
    if (png.empty()) return "";
    std::string b64 = base64_encode(png.data(), (DWORD)png.size());
    if (b64.empty()) return "";
    return "data:image/png;base64," + b64;
}

bool extract_icon_to_file(const std::string& source, int px,
                          const std::string& out_path) {
    std::vector<BYTE> png = source_to_png(source, px);
    if (png.empty()) return false;

    // 宽字符打开，支持中文输出目录（对标全程宽字符约定）。
    std::wstring wpath = utf8_to_wide(out_path);
    HANDLE h = CreateFileW(wpath.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(h, png.data(), (DWORD)png.size(), &written, nullptr)
              && written == png.size();
    CloseHandle(h);
    return ok;
}

}  // namespace cmm
