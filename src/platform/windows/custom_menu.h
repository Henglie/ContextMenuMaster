// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 自建右键菜单项（接口契约）
//
//  差异化核心：让用户在任意经典场景里「新建」自己的右键菜单项——放自己的程序、
//  文件夹、网址或任意命令，可选图标、位置（顶部/底部）、级联分组。
//
//  机制：标准 shell\<verb>\command。在场景的 shell 根（scene_shell_root）下建一个
//    verb 子键，写 MUIVerb（显示名）、Icon（图标）、Position（Top/Bottom），
//    其 command 子键写执行命令行。这是 Windows 官方、最兼容的自定义菜单机制，
//    经典菜单与「显示更多选项」都认（Win11 现代菜单不认非包标识项，见架构事实）。
//
//  自建项标识：verb 键名统一加前缀 "cmm_"（如 cmm_1720012345）。scan_scene 扫到
//    该前缀即知是本工具自建、可编辑删除，与第三方/系统 verb 区分（前端据此给编辑入口）。
//
//  执行类型（frontend 传 kind，backend 组装 command 模板）：
//    program : "<target>" "%1"        —— 目标程序，%1 传入右键选中的文件/文件夹
//    folder  : explorer.exe "<target>" —— 打开一个文件夹
//    url     : cmd /c start "" "<target>" —— 打开网址（默认浏览器）
//    command : <target>               —— 原样命令行（高级，用户自负）
//    对 desktop/folder_bg 等背景场景，%1 无意义，program 模板退化为 "<target>"。
//
//  写入 hive：默认 HKCU\Software\Classes（用户级，免污染全局）；提权已具备，
//    如需全局可扩展写 HKLM，本版先只 HKCU（自建项本就是个人偏好，用户级足够）。
//
//  删除：直接删 verb 键（自建项是我们自己写的，删干净即可；非软删，因为重建成本低
//    且不涉及系统项）。仅允许删带 "cmm_" 前缀的项，杜绝误删第三方/系统 verb。
// ============================================================
#pragma once
#include <string>

#include "platform/windows/menu_scanner.h"  // MenuScene

namespace cmm {

// 新建一个自建菜单项。
//   scene   目标场景（desktop/file/folder/... 由 scene_from_id 解析）
//   label   显示名（写 MUIVerb，UTF-8）
//   kind    执行类型："program" / "folder" / "url" / "command"
//   target  目标（程序路径 / 文件夹路径 / 网址 / 命令行，UTF-8）
//   iconSource 图标源（"exe,-id" / ".ico" / 路径，可空 —— 空则不写 Icon）
//   position   "top" / "bottom" / 空（空=默认位置，不写 Position）
// 返回 {"ok":bool,"key":"...","id":"cmm_xxx"}（成功含新建项 id，供前端定位）。
std::string add_custom_item(MenuScene scene, const std::string& label,
                            const std::string& kind, const std::string& target,
                            const std::string& iconSource, const std::string& position);

// 编辑一个已存在的自建项（仅限 "cmm_" 前缀项）。
//   id      verb 键名（scan 返回的 id，须以 cmm_ 开头）
//   其余参数含义同 add；传入即覆盖对应值（label/target 必填，icon/position 可空=清除该值）。
// 返回 {"ok":bool,"key":"..."}。非自建项（无 cmm_ 前缀）返回 {"ok":false,"key":"not_custom"}。
std::string edit_custom_item(MenuScene scene, const std::string& id,
                             const std::string& label, const std::string& kind,
                             const std::string& target, const std::string& iconSource,
                             const std::string& position);

// 删除一个自建项（仅限 "cmm_" 前缀项）。直接删 verb 键（含 command 子键）。
// 返回 {"ok":bool,"key":"..."}。非自建项返回 {"ok":false,"key":"not_custom"}。
std::string del_custom_item(MenuScene scene, const std::string& id);

}  // namespace cmm
