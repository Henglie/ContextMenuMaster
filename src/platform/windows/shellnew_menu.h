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
//  本模块只做「枚举 + 启停」，新增自定义新建类型极复杂（要建 ProgID + 模板），不做。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 一个「新建」菜单项（对应一个带 ShellNew 的扩展名）。
struct ShellNewItem {
    std::string id;         // 稳定标识：扩展名（含点，如 ".txt"）；ProgID 下的记 ".ext|ProgID"
    std::string ext;        // 扩展名（含点，".txt"）
    std::string label;      // 显示名（关联类型友好名 + 扩展名）
    std::string iconSource; // 图标源（关联 ProgID 的 DefaultIcon，可空）
    bool enabled = true;    // ShellNew 存在即启用，-ShellNew 为停用
};

// 扫描全部带 ShellNew 的扩展名。遍历 HKCR 下 "." 开头的键。
std::vector<ShellNewItem> scan_shellnew();

// 序列化为 JSON 数组（前端拿到即数组，勿再 parse）。
std::string scan_shellnew_json();

// 启用 / 停用某项。id 为 scan 返回的标识。
// 停用 = ShellNew 子键搬到 -ShellNew；启用 = 搬回。写前自动 reg 备份。
// 返回 {"ok":bool,"key":"..."}。
std::string set_shellnew_enabled(const std::string& id, bool enabled);

}  // namespace cmm
