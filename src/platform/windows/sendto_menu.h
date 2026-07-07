// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 「发送到」(SendTo) 菜单管理（接口契约）
//
//  右键「发送到」子菜单不走注册表，是文件系统机制（与 Win+X 同类，故仿 winx_menu）：
//    根目录 %APPDATA%\Microsoft\Windows\SendTo
//    下辖若干项：.lnk 快捷方式 / 直接的 .exe / .url / 甚至普通文件与文件夹。
//    右键任意文件 → 发送到，即列出此目录下的项。
//
//  与经典右键菜单（menu_scanner）本质不同，故独立成模块：
//    - 项 = SendTo 目录下的文件，不是注册表键。
//    - 启停 = 改扩展名加 ".disabled" 后缀 <-> 去掉（explorer 只认原扩展名，可逆）。
//    - 新增 = 往 SendTo 目录建一个指向目标的 .lnk（SendTo 无 hash 校验，纯建 lnk 即可，
//      比 Win+X 简单）。删除 = 送回收站。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 一个「发送到」项（对应 SendTo 目录下一个文件）。
struct SendToItem {
    std::string id;         // 稳定标识：SendTo 根下的文件名（如 "蓝牙设备.lnk"）
    std::string label;      // 显示名（去扩展名的文件名；.lnk 的 .disabled 后缀也剥）
    std::string target;     // 目标路径（.lnk 读 IShellLink 目标；非 lnk 为自身，可空）
    std::string iconSource; // 图标源（目标 exe 路径 / 自身，交 icon_extract 提取，可空）
    bool enabled = true;    // 未带 ".disabled" 后缀即启用
};

// 扫描 SendTo 目录全部项。根不存在返回空 vector。
std::vector<SendToItem> scan_sendto();

// 序列化为 JSON 数组：[{"id","label","target","iconSource","enabled"}, ...]
// （前端拿到即数组，勿再 parse）。
std::string scan_sendto_json();

// 启用 / 停用某项。id 为 scan 返回的文件名。
// 停用：<name> -> <name>.disabled；启用：去掉 ".disabled" 后缀。改名即改可见性，可逆。
// 返回 {"ok":bool,"key":"..."}。
std::string set_sendto_enabled(const std::string& id, bool enabled);

// 新增自定义「发送到」项：在 SendTo 目录建一个指向 targetPath 的 .lnk。
//   targetPath 目标（exe / 文件夹 / 文档等绝对路径）
//   name       显示名（生成 "<name>.lnk"）
// SendTo 无 hash 校验，建好即生效。返回 {"ok":bool,"key":"..."}。
std::string add_sendto_item(const std::string& targetPath, const std::string& name);

// 删除某项。id 为 scan 返回的文件名。送回收站（可恢复，见开发规范安全红线）。
// 返回 {"ok":bool,"key":"..."}。
std::string del_sendto_item(const std::string& id);

// 重命名某项：改文件名主干，保留原扩展名与 ".disabled" 后缀（可逆的启停状态不变）。
//   id      scan 返回的文件名
//   newName 新显示名（不含扩展名；含 \ / .. 视为非法）
// 返回 {"ok":bool,"key":"..."}。key 可能为 bad_id/bad_name/not_found/already_exists/rename_failed。
std::string rename_sendto_item(const std::string& id, const std::string& newName);

// 换图标：仅 .lnk 项可换（写 IShellLink 的 IconLocation）。
//   id         scan 返回的文件名（须为 .lnk / .lnk.disabled）
//   iconSource 图标源（"路径" 或 "路径,索引"）
// 返回 {"ok":bool,"key":"..."}。key 可能为 bad_id/not_lnk/not_found/icon_failed。
std::string set_sendto_icon(const std::string& id, const std::string& iconSource);

}  // namespace cmm
