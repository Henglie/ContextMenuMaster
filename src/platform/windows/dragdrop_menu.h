// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 拖放处理程序（DragDropHandlers）管理（接口契约）
//
//  「拖放处理程序」是一类特殊的 COM shell 扩展：当你用鼠标右键把文件/文件夹「拖」到
//  某个目标（文件夹/驱动器等）再松开时，弹出的那个菜单（复制到此处/移动到此处/在此
//  创建快捷方式…）就由这些 handler 决定。它们注册在各对象类的：
//      HKCR\<类>\shellex\DragDropHandlers\<name> -> (默认值) = CLSID
//  涉及的对象类：Directory（文件夹）/ Folder / Drive（驱动器）/
//      AllFilesystemObjects（所有文件系统对象）。
//
//  与 ContextMenuHandlers 停用机制完全一致：停用 = 把子键整树搬到同级
//  "-DragDropHandlers\<name>"（"-" 前缀 shell 不枚举），启用 = 搬回。可逆。
//  故本模块聚合枚举上述几个类的 DragDropHandlers，启停复用「搬家」思路。
//  这些 handler 的菜单形态由 DLL 运行时决定，静态只读到 CLSID，不支持改名/图标。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 一个拖放处理程序项。
struct DragDropItem {
    std::string id;         // 稳定标识："<类>\\<name>"（如 "Directory\\WinRAR"），启停用
    std::string label;      // 显示名（CLSID 友好名 / 键名，解析后）
    std::string objectClass; // 所属对象类（Directory/Folder/Drive/AllFilesystemObjects）
    std::string clsid;       // handler 的 CLSID（"{...}"，可空）
    std::string iconSource;  // 图标源（从 CLSID 提取，可空）
    bool enabled = true;     // 位于 DragDropHandlers 即启用，位于 -DragDropHandlers 为停用
    bool system = false;     // 是否系统内置（InprocServer32 DLL 在 %SystemRoot% 下）
};

// 扫描全部对象类的 DragDropHandlers（含停用镜像 -DragDropHandlers）。
std::vector<DragDropItem> scan_dragdrop();

// 序列化为 JSON 数组（前端拿到即数组，勿再 parse）。
std::string scan_dragdrop_json();

// 启用 / 停用某项。id 为 scan 返回的 "<类>\\<name>"。
// 停用 = 子键搬到 -DragDropHandlers；启用 = 搬回。两 hive（HKLM/HKCU）都尝试。
// 返回 {"ok":bool,"key":"..."}。
std::string set_dragdrop_enabled(const std::string& id, bool enabled);

}  // namespace cmm
