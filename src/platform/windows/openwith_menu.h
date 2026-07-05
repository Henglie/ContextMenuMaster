// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 打开方式（OpenWith）应用列表（接口契约）
//
//  右键「打开方式」列出的候选应用，来源之一是注册在 Applications 下的程序：
//      HKLM\SOFTWARE\Classes\Applications\<app.exe>\shell\open\command
//      HKCU\Software\Classes\Applications\<app.exe>\shell\open\command
//  每个 <app.exe> 即一个可选「打开方式」应用，读其 FriendlyAppName（或 exe 名）
//  与 open 命令展示。
//
//  启停 = 写/删应用键下的 NoOpenWith 值（低风险可逆，不动文件关联本身）；
//  新增 = 在 HKCU 用户覆盖层建 <name>.exe\shell\open\command + FriendlyAppName；
//  删除 = 仅限用户新增项，软删除为 <app>.deleted 子树（可恢复），系统项拒删。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 一个「打开方式」候选应用（对应 Applications 下一个 <app.exe>）。
struct OpenWithItem {
    std::string id;         // 稳定标识：应用可执行名（Applications 下子键名，如 "notepad.exe"）
    std::string label;      // 显示名（FriendlyAppName 解析后 / 回退可执行名）
    std::string command;    // shell\open\command 命令行（展示用，可空）
    std::string iconSource; // 图标源（DefaultIcon / 命令里的 exe，可空）
    bool enabled = true;    // NoOpenWith 值存在即停用（不出现在「打开方式」列表）
    bool canToggle = true;  // 支持启停（写/删 NoOpenWith 值，低风险可逆）
    bool system = false;    // 机器级（HKLM）项，改动给二次确认
    bool deleted = false;   // 软删除态（<app>.deleted，仅用户新增项可删，可恢复）
};

// 扫描 Applications 下注册的全部应用（HKLM + HKCU 合并，去重）。
std::vector<OpenWithItem> scan_openwith();

// 序列化为 JSON 数组（前端拿到即数组，勿再 parse）。
std::string scan_openwith_json();

// 启用 / 停用某应用：停用 = 写 NoOpenWith 空值（该 app 不出现在「打开方式」），
// 启用 = 删 NoOpenWith 值。写项真实所在 hive（HKLM 有就写 HKLM，需管理员）。
// 返回 {"ok":bool,"key":"..."}。
std::string set_openwith_enabled(const std::string& id, bool enabled);

// 新增自定义「打开方式」应用：在 HKCU\Software\Classes\Applications\<name>.exe
// 写 shell\open\command = "<targetPath>" "%1" + FriendlyAppName。
//   targetPath 目标 exe 绝对路径
//   name       显示名（写 FriendlyAppName；app 子键名取 targetPath 的文件名）
// 返回 {"ok":bool,"key":"..."}。
std::string add_openwith_item(const std::string& targetPath, const std::string& name);

// 删除某应用项。仅允许删用户 hive（HKCU）下的项（HKLM 系统项拒绝）。
// 软删除：<app> 整树搬到 <app>.deleted（可恢复），不真删。返回 {"ok":bool,"key":"..."}。
std::string del_openwith_item(const std::string& id);

}  // namespace cmm
