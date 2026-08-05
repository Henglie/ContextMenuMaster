// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 「此电脑」仿真管理（接口契约）
//
//  把「此电脑」（我的电脑）界面 1:1 仿真出来：真实磁盘（图标 + 容量）与
//  第三方软件（云盘、网盘等）塞进来的命名空间扩展图标混合渲染，可一键删除。
//
//  两类格子：
//    drive —— 真实磁盘。GetLogicalDrives 枚举 + GetDriveType 判型 +
//             GetDiskFreeSpaceEx 拿容量；图标走 SHSTOCKICONID（stock:145~149）
//             交给前端 extract_icon 提取，只读不可删。
//    ns    —— MyComputer\NameSpace 下的 COM 命名空间扩展（第三方快捷图标）。
//             注册位置：
//               HKLM\SOFTWARE\...\Explorer\MyComputer\NameSpace
//               HKCU\SOFTWARE\...\Explorer\MyComputer\NameSpace
//             子键名为 CLSID。可启停（搬到 -NameSpace 镜像）、可一键删除
//             （删前自动 .reg 备份）。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 一个「此电脑」格子项：真实磁盘 或 命名空间扩展图标。
struct DriveIconItem {
    std::string kind;        // "drive" 真实磁盘 | "ns" 命名空间扩展
    std::string id;          // drive: "drive:C:"；ns: CLSID 字符串（含花括号）
    std::string letter;      // drive only：盘符（如 "C:"）
    std::string type;        // drive only：fixed/removable/network/cdrom/ram
    std::string label;       // 显示名（ns: 从 CLSID 注册信息解析；drive: 盘符）
    std::string iconSource;  // 图标源字符串（ns: DefaultIcon 或 DLL,0；drive 可为空）
    std::string iconData;    // 已提取的 PNG data URI（drive: 真实系统图标；ns 空）
    std::string knownId;     // ns only：解析失败的系统项标 "system"（前端 i18n 显示）
    unsigned long long totalBytes = 0;   // drive only：总容量
    unsigned long long freeBytes  = 0;   // drive only：剩余容量
    std::string dllPath;     // ns only：实现 DLL 路径（InprocServer32）
    bool systemItem = false; // ns: DLL 在 SystemRoot 下即系统项
    bool fromHKLM = false;   // ns: 注册在 HKLM（否则 HKCU）
    bool enabled = true;     // ns: NameSpace 下为启用，-NameSpace 下为停用
};

// 扫描「此电脑」全部格子：真实磁盘（含容量）+ NameSpace 项（HKLM+HKCU，含停用镜像）。
// 磁盘排前（字母序），ns 项排后。
std::vector<DriveIconItem> scan_drive_icons();

// 序列化为 JSON 数组。字段见 struct。容量为数字（字节）。
std::string scan_drive_icons_json();

// 启用/停用 ns 项。停用 = 子键从 NameSpace 搬到 -NameSpace（启用反之），可逆。
// id = CLSID 字符串。返回 {"ok":bool,"key":"..."}。
std::string set_drive_icon_enabled(const std::string& id, bool enabled);

// 删除 ns 项：从注册表移除 MyComputer\NameSpace（及 -NameSpace）下的 CLSID 子键。
// 写前自动 .reg 备份（备份失败拒绝删除）。返回 {"ok":bool,"key":"..."}。
std::string del_drive_icon(const std::string& id);

}  // namespace cmm
