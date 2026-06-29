// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 系统图标提取
//
//  把 Windows 系统 DLL（imageres.dll / shell32.dll）里的图标资源、或注册表
//  verb 自带的 Icon 字符串、或独立 .ico 文件，提取成 PNG 的 base64 data URI，
//  直接交给 WebView2 前端用 <img> 渲染——这样右键菜单是「真图标」，不是 emoji。
//
//  为什么用 PrivateExtractIconsW 而不是 ExtractIconEx：
//    前者既支持「负数 = 资源 ID」（匹配注册表 "file.dll,-123" 写法），又能
//    指定提取的精确像素尺寸（高分屏下直接取 32/48px，不靠缩放糊掉）。
//
//  为什么不用 Gdiplus::Bitmap::FromHICON：
//    它对部分 32bpp 图标会丢 alpha、出黑底。我们走 GetIconInfo + GetDIBits
//    手动拉 BGRA 像素，alpha 保真；老式无 alpha 图标再用掩码位重建透明区。
//
//  Extract icons from Windows system DLLs / registry verb Icon strings / .ico
//  files into a base64 PNG data URI for the WebView2 frontend to render as
//  <img>, so the context menu shows REAL icons instead of emoji.
// ============================================================
#pragma once
#include <string>

namespace cmm {

// 进程内确保 GDI+ 已初始化（首调启动，进程退出时自然回收，不需手动 shutdown）。
// GUI / CLI 入口各调一次更稳妥，但本函数幂等，重复调用无害。
// Ensure GDI+ is started (idempotent; token lives for the process lifetime).
void ensure_gdiplus();

// 从「图标源」提取为 PNG 的 base64 data URI（"data:image/png;base64,..."）。
// 失败（找不到资源 / 提取失败）返回空串，前端据此回落到黑白几何字符。
//
// source 支持三种写法（UTF-8）：
//   "imageres.dll,-5350"  系统 DLL + 资源 ID（负）或索引（正、零基）
//   "C:\\path\\foo.ico"    独立 .ico / .exe / .dll（默认取第 0 个图标）
//   "stock:32"            Windows 内置 stock 图标（数字为 SHSTOCKICONID）
//
// px：期望像素边长（高分屏建议 32 或 48）。
// Extract an icon source to a base64 PNG data URI; empty string on failure.
std::string extract_icon_data_uri(const std::string& source, int px = 32);

// 把图标源提取并保存为磁盘上的 PNG 文件（out_path 为 UTF-8 路径）。
// 仅供 CLI 调试命令 dumpicons 用：批量导出某 DLL 的图标，在真机上肉眼
// 确认「剪切/复制/删除…」各自对应的真实资源索引，再钉进前端映射表。
// 成功返回 true。
// Save an extracted icon as a PNG file on disk (CLI dumpicons helper).
bool extract_icon_to_file(const std::string& source, int px,
                          const std::string& out_path);

}  // namespace cmm
