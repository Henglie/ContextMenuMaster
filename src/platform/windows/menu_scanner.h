// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 真实右键菜单扫描 / 写回（接口契约）
//
//  本模块把 Windows 注册表里「各种右键场景」的真实菜单项读出来，交给前端 1:1
//  还原；并支持改名 / 换图标 / 启用停用 / 批量后缀的写回（写回前自动 .reg 备份）。
//
//  Windows 右键菜单按「在什么东西上点」分多个场景（scene），各自挂在不同注册表根：
//    桌面空白    HKCR\DesktopBackground\Shell  +  ...\ShellEx\ContextMenuHandlers
//    文件(所有)  HKCR\*\shell                  +  HKCR\*\shellex\ContextMenuHandlers
//    文件夹      HKCR\Directory\shell          +  HKCR\Directory\shellex\ContextMenuHandlers
//    文件夹空白  HKCR\Directory\Background\shell + ...\shellex\ContextMenuHandlers
//    驱动器      HKCR\Drive\shell              +  HKCR\Drive\shellex\ContextMenuHandlers
//    回收站      HKCR\CLSID\{645FF040-...}\shell
//    特定类型    HKCR\<ProgID 或 .ext>\shell（如 .txt -> txtfile -> shell）
//
//  两类来源：
//    verb 项（shell\<verb>）：传统菜单项，名字/图标/命令都是明文注册表值，可读可改。
//    handler 项（shellex\ContextMenuHandlers\<name> -> CLSID）：COM 动态菜单扩展，
//      名字由 DLL 运行时决定，注册表只有 CLSID。可读可启停，但改名/图标多不适用。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 右键场景。值与前端约定的字符串一一对应（见 scene_id）。
enum class MenuScene {
    DesktopBackground,   // 桌面空白处
    AllFiles,            // 任意文件 (HKCR\*)
    Directory,           // 文件夹
    DirectoryBackground, // 文件夹/资源管理器空白处
    Drive,               // 驱动器（此电脑里的盘符）
    RecycleBin,          // 回收站
};

// 菜单项来源类型。
enum class MenuItemKind {
    Verb,     // shell\<verb>：明文项，可改名/图标/启停/删
    Handler,  // shellex COM 扩展：可启停，改名/图标多不适用
};

// 一个右键菜单项。字段尽量贴近注册表真实信息，供前端 1:1 还原与编辑。
struct MenuItem {
    std::string id;          // 稳定标识：场景内唯一（取注册表子键名/handler 名）
    MenuItemKind kind;       // Verb / Handler
    std::string label;       // 显示名（verb 的默认值 / MUIVerb 解析后；handler 取键名）
    std::string iconSource;  // 图标源 "dll,-id" / "x.ico" / 空（交给 icon_extract 提取）
    std::string command;     // verb 的执行命令（handler 为空）
    std::string clsid;       // handler 的 CLSID（"{...}"，verb 留空）—— 前端拿去 probe 真实动态菜单
    std::string regPath;     // 该项所在注册表完整路径（写回 / 备份用）
    bool enabled = true;     // 是否启用（停用 = 加 LegacyDisable / 移走，可逆）
    bool system = false;     // 是否系统内置项（系统项改名/删除给前端更强的二次确认）
    bool canRename = false;  // 是否支持改名（Verb 且非保护项才 true）
    bool canChangeIcon = false; // 是否支持换图标
    bool canToggle = true;   // 是否支持启用/停用

    // 级联子菜单（折叠菜单）。某些软件（如夸克AI）把一组子项折叠进一个父项：
    //   静态法：父 verb 带 SubCommands 值（分号/空格分隔，token 指向 CommandStore），
    //   或带 ExtendedSubCommandsKey 值（相对 HKCR 的键路径，其 shell\ 子键放子 verb）。
    // hasSubmenu=true 时 children 为展开后的子项（只读，纯还原，不参与写回）。
    // Cascading submenu: SubCommands (CommandStore tokens) or ExtendedSubCommandsKey.
    bool hasSubmenu = false;
    std::vector<MenuItem> children;
};

// scene <-> 字符串 id（与前端 i18n key、IPC 传参一致）。
const char* scene_id(MenuScene scene);
bool scene_from_id(const std::string& id, MenuScene* out);

// 扫描某个场景下的全部右键菜单项。失败返回空 vector。
std::vector<MenuItem> scan_scene(MenuScene scene);

// 把一个场景的扫描结果序列化为 JSON 数组字符串（前端拿到即数组，勿再 JSON.parse）。
std::string scan_scene_json(MenuScene scene);

// —— 写回操作（每个写操作内部先自动 .reg 备份，再改注册表）——
// 结果用英文 key（前端 i18n 翻译）。返回 JSON：{"ok":bool,"key":"..."}。

// 改名：把某 verb 项的显示名改为 newLabel。
std::string rename_item(MenuScene scene, const std::string& id, const std::string& newLabel);

// 启用 / 停用：停用走 LegacyDisable（可逆），不删项。
std::string set_item_enabled(MenuScene scene, const std::string& id, bool enabled);

// 换图标：把某 verb 项的 Icon 值设为 iconSource（"dll,-id" / 路径）。
std::string set_item_icon(MenuScene scene, const std::string& id, const std::string& iconSource);

// 批量后缀魔法：给指定若干项的显示名统一追加 suffix。ids 为逗号分隔的 id 列表。
std::string batch_append_suffix(MenuScene scene, const std::string& idsCsv, const std::string& suffix);

// —— 备份 / 还原 ——
// 把本工具改过的相关注册表键导出为 .reg 到本地备份目录，返回备份文件路径（JSON）。
std::string backup_scene(MenuScene scene);

}  // namespace cmm
