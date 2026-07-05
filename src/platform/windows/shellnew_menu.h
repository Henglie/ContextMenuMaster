// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 新建菜单（ShellNew）管理（接口契约）
//
//  右键空白处「新建」子菜单的项由注册表决定：某扩展名 .ext 下若有 ShellNew 子键，
//  「新建」里就出现「新建 <该类型>」一项。注册位置：
//      HKCR\.<ext>\ShellNew            （直接挂在扩展名下，最常见）
//      HKCR\.<ext>\<ProgID>\ShellNew   （挂在扩展名的关联 ProgID 下，次常见）
//  ShellNew 子键里的值（NullFile / FileName / Data / Command）决定新建时如何造文件。
//
//  显示名：扩展名 + 其关联类型友好名（.txt -> "文本文档"，读 ProgID 的
//  FriendlyTypeName / 默认值解析）。
//  启停：把 ShellNew 子键整树搬到同级 "-ShellNew"（"-" 前缀 shell 不认），可逆。
//  新增：在用户 hive（HKCU）的 .ext\ShellNew 写 NullFile，让「新建」多出该类型
//  （新建空文件）。删除：仅移除用户 hive 下的 ShellNew（系统项拒绝，避免破坏内建）。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 一个「新建」菜单项（对应一个带 ShellNew 的扩展名）。
struct ShellNewItem {
    std::string id;           // 稳定标识：扩展名（含点，如 ".txt"）；ProgID 下的记 ".ext|ProgID"
    std::string ext;          // 扩展名（含点，".txt"）
    std::string friendlyName; // 关联类型友好名（如「文本文档」），解析不到为空
    std::string label;        // 兼容旧前端的拼装名（friendlyName + ext）；新前端用 friendlyName+ext 自拼
    std::string iconSource;   // 图标源（关联 ProgID 的 DefaultIcon，可空）
    bool enabled = true;      // ShellNew 存在即启用，-ShellNew 为停用
};

// 扫描全部带 ShellNew 的扩展名。遍历 HKCR 下 "." 开头的键。
std::vector<ShellNewItem> scan_shellnew();

// 序列化为 JSON 数组（前端拿到即数组，勿再 parse）。
std::string scan_shellnew_json();

// 启用 / 停用某项。id 为 scan 返回的标识。
// 停用 = ShellNew 子键搬到 -ShellNew；启用 = 搬回。写前自动 reg 备份。
// 返回 {"ok":bool,"key":"..."}。
std::string set_shellnew_enabled(const std::string& id, bool enabled);

// 新增「新建」项：在 HKCU\Software\Classes\<ext>\ShellNew 写 NullFile，
// 让右键「新建」多出该扩展名（新建空文件）。ext 需含点（如 ".log"）。
// 已存在 ShellNew（任一 hive）则拒。返回 {"ok":bool,"key":"..."}。
std::string add_shellnew_item(const std::string& ext);

// 删除「新建」项：仅移除用户 hive（HKCU）下 <ext>\ShellNew（及停用镜像 -ShellNew）。
// 系统项（仅 HKLM 存在）拒绝删除（避免破坏内建新建类型）。返回 {"ok":bool,"key":"..."}。
std::string del_shellnew_item(const std::string& id);

}  // namespace cmm
