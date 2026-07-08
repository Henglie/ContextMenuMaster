// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 图片转图标（实现）
//  见 image_icon.h 顶部机制说明。
// ============================================================
#include "platform/windows/image_icon.h"
#include "platform/windows/icon_extract.h"   // ensure_gdiplus()

#include <windows.h>
#include <shlobj.h>       // IFileOpenDialog
#include <shobjidl.h>
#include <gdiplus.h>

#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

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
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::string json_escape(const std::string& s) {
    std::string o; o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) { char b[8]; wsprintfA(b, "\\u%04x", (unsigned)(unsigned char)c); o += b; }
                else o += c;
        }
    }
    return o;
}

std::string result_json_path(bool ok, const char* key, const std::string& path) {
    std::string o = "{\"ok\":";
    o += ok ? "true" : "false";
    o += ",\"key\":\"";  o += json_escape(key ? key : "");
    o += "\",\"path\":\""; o += json_escape(path);
    o += "\"}";
    return o;
}

bool ends_with_ci(const std::wstring& s, const std::wstring& suf) {
    if (s.size() < suf.size()) return false;
    return CompareStringOrdinal(s.c_str() + (s.size() - suf.size()), (int)suf.size(),
                                suf.c_str(), (int)suf.size(), TRUE) == CSTR_EQUAL;
}

// %LOCALAPPDATA%\ContextMenuMaster\icons\，逐级创建。失败返回空。
std::wstring ensure_icons_dir() {
    wchar_t base[MAX_PATH] = {0};
    DWORD got = GetEnvironmentVariableW(L"LOCALAPPDATA", base, MAX_PATH);
    if (got == 0 || got >= MAX_PATH) return L"";
    std::wstring dir = base;
    dir += L"\\ContextMenuMaster";
    CreateDirectoryW(dir.c_str(), nullptr);
    dir += L"\\icons";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

// PNG 编码器 CLSID（GDI+ 内建）。
bool png_encoder_clsid(CLSID* clsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return false;
    std::vector<BYTE> buf(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
    Gdiplus::GetImageEncoders(num, size, codecs);
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(codecs[i].MimeType, L"image/png") == 0) { *clsid = codecs[i].Clsid; return true; }
    }
    return false;
}

// 把源位图缩放到 px×px 的 32bpp ARGB 位图后编码为 PNG 字节。等比缩放居中，
// 透明背景填充（保持圆角/异形图 alpha）。失败返回空。
std::vector<BYTE> scale_to_png(Gdiplus::Bitmap* src, int px, const CLSID& pngClsid) {
    std::vector<BYTE> out;
    Gdiplus::Bitmap canvas(px, px, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(&canvas);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        g.Clear(Gdiplus::Color(0, 0, 0, 0));   // 透明底

        UINT sw = src->GetWidth(), sh = src->GetHeight();
        if (sw == 0 || sh == 0) return out;
        // 等比缩放，长边填满 px，居中。
        double scale = (double)px / (sw > sh ? sw : sh);
        int dw = (int)(sw * scale + 0.5), dh = (int)(sh * scale + 0.5);
        if (dw < 1) dw = 1; if (dh < 1) dh = 1;
        int dx = (px - dw) / 2, dy = (px - dh) / 2;
        g.DrawImage(src, Gdiplus::Rect(dx, dy, dw, dh), 0, 0, sw, sh, Gdiplus::UnitPixel);
    }
    // 编码 PNG 到内存流。
    IStream* stream = nullptr;
    if (CreateStreamOnHGlobal(nullptr, TRUE, &stream) != S_OK) return out;
    if (canvas.Save(stream, &pngClsid, nullptr) != Gdiplus::Ok) { stream->Release(); return out; }
    HGLOBAL hg = nullptr;
    GetHGlobalFromStream(stream, &hg);
    SIZE_T size = GlobalSize(hg);
    void* p = GlobalLock(hg);
    if (p && size) { out.resize(size); memcpy(out.data(), p, size); }
    GlobalUnlock(hg);
    stream->Release();
    return out;
}

#pragma pack(push, 1)
struct IcoDir     { WORD reserved; WORD type; WORD count; };
struct IcoDirEntry{ BYTE w; BYTE h; BYTE colors; BYTE reserved;
                    WORD planes; WORD bitCount; DWORD bytesInRes; DWORD imageOffset; };
#pragma pack(pop)

// 把若干「尺寸 + PNG 字节」组装成 PNG-in-ICO 容器字节。
//  ICONDIR + N×ICONDIRENTRY + 各 PNG 数据。ICO 里嵌 PNG 是 Vista+ 官方支持，
//  256px 大图标必须走 PNG（BMP 编码 256px 会超 ICONDIRENTRY 的字段上限）。
std::vector<BYTE> build_ico(const std::vector<std::pair<int, std::vector<BYTE>>>& imgs) {
    std::vector<BYTE> ico;
    WORD n = (WORD)imgs.size();
    if (n == 0) return ico;

    IcoDir dir{ 0, 1, n };   // type=1 图标
    DWORD offset = sizeof(IcoDir) + (DWORD)n * sizeof(IcoDirEntry);

    std::vector<IcoDirEntry> entries;
    entries.reserve(n);
    for (auto& im : imgs) {
        IcoDirEntry e{};
        // 256px 在 w/h 字段写 0（0 表示 256，ICO 格式约定）。
        e.w = (BYTE)(im.first >= 256 ? 0 : im.first);
        e.h = (BYTE)(im.first >= 256 ? 0 : im.first);
        e.colors = 0; e.reserved = 0;
        e.planes = 1; e.bitCount = 32;
        e.bytesInRes = (DWORD)im.second.size();
        e.imageOffset = offset;
        offset += (DWORD)im.second.size();
        entries.push_back(e);
    }

    const BYTE* dp = reinterpret_cast<const BYTE*>(&dir);
    ico.insert(ico.end(), dp, dp + sizeof(IcoDir));
    for (auto& e : entries) {
        const BYTE* ep = reinterpret_cast<const BYTE*>(&e);
        ico.insert(ico.end(), ep, ep + sizeof(IcoDirEntry));
    }
    for (auto& im : imgs) ico.insert(ico.end(), im.second.begin(), im.second.end());
    return ico;
}

// 源路径 + 文件大小 + mtime 生成稳定 hash（同图不重复转）。
std::wstring hash_name(const std::wstring& srcPath) {
    ULONGLONG size = 0, mtime = 0;
    WIN32_FILE_ATTRIBUTE_DATA fad{};
    if (GetFileAttributesExW(srcPath.c_str(), GetFileExInfoStandard, &fad)) {
        size = ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
        mtime = ((ULONGLONG)fad.ftLastWriteTime.dwHighDateTime << 32) | fad.ftLastWriteTime.dwLowDateTime;
    }
    // FNV-1a over path + size + mtime。
    ULONGLONG h = 1469598103934665603ULL;
    auto mix = [&](const void* p, size_t len) {
        const BYTE* b = (const BYTE*)p;
        for (size_t i = 0; i < len; ++i) { h ^= b[i]; h *= 1099511628211ULL; }
    };
    mix(srcPath.data(), srcPath.size() * sizeof(wchar_t));
    mix(&size, sizeof(size));
    mix(&mtime, sizeof(mtime));
    wchar_t buf[32];
    wsprintfW(buf, L"%08x%08x", (unsigned)(h >> 32), (unsigned)(h & 0xffffffff));
    return buf;
}

bool write_file_bytes(const std::wstring& path, const std::vector<BYTE>& data) {
    HANDLE hf = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hf == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    BOOL ok = WriteFile(hf, data.data(), (DWORD)data.size(), &written, nullptr);
    CloseHandle(hf);
    return ok && written == data.size();
}

bool copy_file_bytes(const std::wstring& src, const std::wstring& dst) {
    return CopyFileW(src.c_str(), dst.c_str(), FALSE) != 0;
}

}  // namespace

std::string pick_image_file() {
    // COM 由调用线程负责初始化（GUI 异步 bind 线程自带 CoInitializeEx）。
    IFileOpenDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&dlg))))
        return "";
    std::string result;
    COMDLG_FILTERSPEC filters[] = {
        { L"图片 Images", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.ico" },
        { L"所有文件 All files", L"*.*" },
    };
    dlg->SetFileTypes(ARRAYSIZE(filters), filters);
    dlg->SetTitle(L"选择图片");
    if (SUCCEEDED(dlg->Show(nullptr))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
                result = wide_to_utf8(path);
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dlg->Release();
    return result;
}

std::string convert_image_to_icon(const std::string& srcImagePathUtf8) {
    if (srcImagePathUtf8.empty()) return result_json_path(false, "bad_arg", "");
    std::wstring src = utf8_to_wide(srcImagePathUtf8);

    // 源文件须存在。
    if (GetFileAttributesW(src.c_str()) == INVALID_FILE_ATTRIBUTES)
        return result_json_path(false, "not_found", "");

    std::wstring dir = ensure_icons_dir();
    if (dir.empty()) return result_json_path(false, "no_dir", "");

    std::wstring outPath = dir + L"\\" + hash_name(src) + L".ico";

    // 源已是 .ico：直接复制落地，不重复编码。
    if (ends_with_ci(src, L".ico")) {
        if (copy_file_bytes(src, outPath))
            return result_json_path(true, "ok", wide_to_utf8(outPath));
        return result_json_path(false, "copy_failed", "");
    }

    ensure_gdiplus();

    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(src.c_str(), FALSE);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
        delete bmp;
        return result_json_path(false, "decode_failed", "");
    }

    CLSID pngClsid;
    if (!png_encoder_clsid(&pngClsid)) { delete bmp; return result_json_path(false, "no_encoder", ""); }

    // 多尺寸：16/32/48/256（菜单常用 16/32，256 备高分屏/属性预览）。
    const int sizes[] = { 16, 32, 48, 256 };
    std::vector<std::pair<int, std::vector<BYTE>>> imgs;
    for (int px : sizes) {
        std::vector<BYTE> png = scale_to_png(bmp, px, pngClsid);
        if (!png.empty()) imgs.emplace_back(px, std::move(png));
    }
    delete bmp;
    if (imgs.empty()) return result_json_path(false, "encode_failed", "");

    std::vector<BYTE> ico = build_ico(imgs);
    if (ico.empty()) return result_json_path(false, "build_failed", "");

    if (!write_file_bytes(outPath, ico))
        return result_json_path(false, "write_failed", "");

    return result_json_path(true, "ok", wide_to_utf8(outPath));
}

}  // namespace cmm
