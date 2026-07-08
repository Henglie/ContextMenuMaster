// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 文件类型右键菜单（接口契约）
//
//  与 menu_scanner 的固定 6+3 场景不同：这里管理「某个具体扩展名」的右键菜单
//  （如只给 .txt / .jpg / .zip 加项）。参考项目 ContextMenuMgr 的「文件类型」页，
//  本项目 C 批「场景补全」的最后一块。
//
//  为什么独立成模块（不塞进 menu_scanner）：
//    menu_scanner 所有读写硬绑 scene_shell_root(scene) 这个编译期常量根；文件类型
//    是「用户运行时输入 .ext → 解析 ProgID 链 → 聚合多个动态根」，模型完全不同。
//    强塞会污染已验证的核心扫描器（见 win11menu 曾提交损坏的教训），故独立。
//
//  一个扩展名的右键菜单项分散在 4 类根（真机 regedit 实测，见 PROGRESS C 批）：
//    1. HKCR\.ext\shell + \shellex\ContextMenuHandlers          扩展名直挂（少见）
//    2. HKCR\<ProgID>\shell + \shellex\...                       ProgID 关联（.txt→txtfilelegacy、.zip→7-Zip.zip）
//    3. HKCR\SystemFileAssociations\.ext\shell + \shellex\...    扩展名系统关联（.jpg 的 ShellImagePreview）
//    4. HKCR\SystemFileAssociations\<PerceivedType>\shell + ...  感知类型（image/text/video…，一类共享）
//  scan 聚合这 4 类，每项标 originRoot 表明来源根，写回时据此定位。
//
//  可逆与安全：沿用 menu_scanner 的成熟做法——
//    - verb 启停：加/删 LegacyDisable 值（不删项，可逆）。
//    - handler 启停：ContextMenuHandlers <-> -ContextMenuHandlers 搬家（可逆）。
//    - 新建自建项：写 HKCU\Software\Classes\SystemFileAssociations\.ext\shell\cmm_xxx
//      （用户级，免污染全局；verb 名带 cmm_ 前缀，与 custom_menu 同规矩）。
//    - 写前对目标扩展名整棵关联树 reg export 备份（见 backup_filetype）。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 文件类型菜单项来源根（写回 / 显示分组用）。
enum class FtRoot {
    Ext,          // HKCR\.ext\shell
    ProgId,       // HKCR\<ProgID>\shell
    SfaExt,       // HKCR\SystemFileAssociations\.ext\shell
    SfaPerceived, // HKCR\SystemFileAssociations\<PerceivedType>\shell
};

// 一个文件类型右键菜单项。
struct FtItem {
    std::string id;          // 稳定标识："<rootTag>|<verb 或 handler:name>"，场景内唯一
    std::string kind;        // "verb" / "handler"
    std::string label;       // 显示名（verb: MUIVerb/默认值解析；handler: CLSID 明文解析）
    std::string iconSource;  // 图标源（交 icon_extract；可空）
    std::string command;     // verb 执行命令（handler 空）
    std::string clsid;       // handler 的 CLSID（verb 空）
    std::string rootTag;     // 来源根标签："ext"/"progid"/"sfaext"/"sfaperceived"
    std::string regPath;     // 该项注册表完整路径（写回 / 备份参考）
    bool enabled = true;     // 启用（verb 无 LegacyDisable / handler 在启用根）
    bool system = false;     // 系统内置（更强二次确认）
    bool canToggle = true;
    bool canRename = false;  // verb 且非系统项才可改名
    bool canChangeIcon = false;
    bool custom = false;     // 本工具自建（verb 名 cmm_ 前缀）
};

// 解析某扩展名的关联信息（供前端显示：ProgID、感知类型、各根是否存在）。
struct FtAssoc {
    std::string ext;           // 规范化后的扩展名（带点、小写，如 ".txt"）
    std::string progId;        // 关联 ProgID（可空）
    std::string perceivedType; // PerceivedType（image/text/video/…，可空）
    bool valid = false;        // ext 是否合法（以点开头、无非法字符）
};

// 规范化扩展名：去空白、补前导点、转小写。非法（空 / 含 \ / 空格 / 路径分隔）返回 valid=false。
FtAssoc resolve_filetype(const std::string& extRaw);

// 扫描某扩展名的全部右键菜单项（聚合 4 类根 + 各自 shellex 启用/停用镜像）。
// 扩展名非法或无任何项时返回空 vector。
std::vector<FtItem> scan_filetype(const std::string& ext);

// 序列化为 JSON：{"assoc":{ext,progId,perceivedType,valid}, "items":[FtItem...]}
// （前端拿到即对象，勿再 parse）。扩展名非法则 items 空、assoc.valid=false。
std::string scan_filetype_json(const std::string& ext);

// 启用 / 停用某项。id 为 scan 返回的 id（含 rootTag 前缀）。
//   verb：加/删 LegacyDisable（可逆）。handler：搬 ContextMenuHandlers <-> -...（可逆）。
// 写前自动备份。返回 {"ok":bool,"key":"..."}。
std::string set_filetype_enabled(const std::string& ext, const std::string& id, bool enabled);

// 新建自建项：写 HKCU\Software\Classes\SystemFileAssociations\.ext\shell\cmm_xxx
//   （SFA\.ext 是最稳的用户级挂载点，对该扩展名所有文件生效，不碰 ProgID/全局）。
//   label/kind/target/iconSource/position 语义同 custom_menu 的 add_custom_item。
// 返回 {"ok":bool,"key":"...","id":"..."}。
std::string add_filetype_item(const std::string& ext, const std::string& label,
                              const std::string& kind, const std::string& target,
                              const std::string& iconSource, const std::string& position);

// 编辑自建项（仅限 cmm_ 前缀、HKCU SFA 下的项）：重写 label/kind/target/iconSource/position。
//   与 add_filetype_item 同字段语义；id 为 scan 返回的 id（sfaext|verb:cmm_xxx）。返回 {"ok":bool,"key":"..."}。
std::string edit_filetype_item(const std::string& ext, const std::string& id,
                               const std::string& label, const std::string& kind,
                               const std::string& target, const std::string& iconSource,
                               const std::string& position);

// 删除自建项（仅限 cmm_ 前缀、且在 HKCU SFA 下的项）。返回 {"ok":bool,"key":"..."}。
std::string del_filetype_item(const std::string& ext, const std::string& id);

// 备份某扩展名的整棵关联树（.ext + ProgID + SFA\.ext）到本地文件。
// 返回 {"ok":bool,"key":"...","path":"..."}。
std::string backup_filetype(const std::string& ext);

}  // namespace cmm
