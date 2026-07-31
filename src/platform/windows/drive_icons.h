// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 「此电脑」盘符图标管理（接口契约）
//
//  第三方软件（云盘、网盘、同步工具等）常在「此电脑」（我的电脑）的盘符界面里
//  塞快捷图标。它们不是真正的盘符，而是注册为 MyComputer\NameSpace 下的 COM
//  命名空间扩展（Shell Namespace Extension），注册位置：
//    HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\MyComputer\NameSpace
//    HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\MyComputer\NameSpace
//  每个子键名为 CLSID，指向实现 IShellFolder 等的 COM 对象。
//
//  本模块扫描这些图标项并支持一键删除（删 CLSID 子键前自动 .reg 备份），
//  后续可扩展启停（搬到 -NameSpace 镜像）功能。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 一个「此电脑」盘符图标项（MyComputer\NameSpace 下的一个注册项）。
struct DriveIconItem {
    std::string id;          // 稳定标识 = CLSID 字符串（含花括号，如 "{xxx}"）
    std::string label;       // 显示名（从 CLSID 注册信息解析）
    std::string iconSource;  // 图标源（DefaultIcon 或 DLL 路径,0）
    std::string dllPath;     // 实现 DLL 路径（InprocServer32，用于判断来源）
    bool systemItem = false; // 是否系统内置项（DLL 在 SystemRoot 下即为系统项）
    bool fromHKLM = false;   // 该项是否注册在 HKLM（否则 HKCU）
    bool enabled = true;     // NameSpace 下为启用，-NameSpace 下为停用
};

// 扫描 MyComputer\NameSpace 全部项（含 HKLM + HKCU，及各自的启用/停用镜像）。
std::vector<DriveIconItem> scan_drive_icons();

// 序列化为 JSON 数组：[{"id","label","iconSource","dllPath","system","hkcu","enabled"},...]
std::string scan_drive_icons_json();

// 启用/停用某项。停用 = 子键从 NameSpace 搬到 -NameSpace（启用反之），可逆。
// id = CLSID 字符串。返回 {"ok":bool,"key":"..."}。
std::string set_drive_icon_enabled(const std::string& id, bool enabled);

// 删除某项：从注册表移除 MyComputer\NameSpace（及 -NameSpace）下的 CLSID 子键。
// 写前自动 .reg 备份。返回 {"ok":bool,"key":"..."}。
std::string del_drive_icon(const std::string& id);

}  // namespace cmm
