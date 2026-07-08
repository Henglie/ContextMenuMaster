// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 图片转图标（富媒体阶段 1）
//
//  「美化工具」特色的第一步（纯注册表，无需 COM，无需重启 explorer）：
//  自建菜单项的图标从「限 .ico / DLL 资源路径」扩展到「任意 PNG/JPG/BMP/GIF」。
//
//  机制：图片 → GDI+ 解码 → 缩放多尺寸（16/32/48/256）→ 每尺寸编码 PNG →
//  组装成 PNG-in-ICO 容器（Vista+ 原生支持 ICO 内嵌 PNG，256px 大图标必需）→
//  存到 %LOCALAPPDATA%\ContextMenuMaster\icons\<hash>.ico，注册表 Icon 指向它。
//  经典右键菜单原生就能显示这个 .ico，不需要任何 hook 或自绘。
//
//  为什么转 ICO 而不直接指 PNG：Windows 经典菜单 verb 的 Icon 值只认
//  .ico / .exe / .dll 里的图标资源，不认 .png/.jpg。故必须转成 .ico 落地。
// ============================================================
#pragma once
#include <string>

namespace cmm {

// 弹系统文件选择框（IFileOpenDialog），让用户挑一张图片。
// 返回选中文件的 UTF-8 绝对路径；用户取消 / 出错返回空串。
// 过滤器限图片类型（PNG/JPG/JPEG/BMP/GIF/ICO）。
// 注意：必须在有消息循环的线程调用；GUI bind 里走异步线程时需自带 COM 初始化。
std::string pick_image_file();

// 把一张图片（PNG/JPG/JPEG/BMP/GIF，UTF-8 路径）转成多尺寸 .ico，
// 存到 %LOCALAPPDATA%\ContextMenuMaster\icons\ 下（文件名按源路径+大小去重）。
// 返回 JSON：{"ok":bool,"key":"...","path":"<ico 绝对路径>"}。
// 若源本身已是 .ico，直接复制落地（不重复编码）。
// path 即可直接写进自建项的 iconSource（注册表 Icon）。
std::string convert_image_to_icon(const std::string& srcImagePathUtf8);

}  // namespace cmm
