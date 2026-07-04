// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— Win+X 菜单管理（接口契约）
//
//  Win+X 菜单（按 Win+X 或右键「开始」按钮弹出）不走注册表，是文件系统机制：
//    根目录 %LOCALAPPDATA%\Microsoft\Windows\WinX
//    下辖 Group1 / Group2 / Group3 三个分组文件夹，各含若干 .lnk 快捷方式。
//    菜单渲染：分组倒序（Group3 在最上），组内按 lnk 内 System.Winx.Hash 校验后排序。
//
//  与经典右键菜单（menu_scanner）本质不同，故独立成模块：
//    - 项 = .lnk 文件，不是注册表键。
//    - 启停 = 改扩展名 .lnk <-> .lnk.disabled（explorer 只认 .lnk，可逆）。
//    - 新增自定义项需给 lnk 写合法 System.Winx.Hash，否则被 explorer 忽略（见 winx hash，T7）。
//
//  本模块（T6）只做「枚举 + 启停」；hash 计算与新增/删除自定义项见 T7。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 一个 Win+X 快捷方式项（对应一个 .lnk / .lnk.disabled 文件）。
struct WinXItem {
    std::string id;         // 稳定标识：相对 WinX 根的路径（如 "Group2\\01 - 程序和功能.lnk"）
    std::string label;      // 显示名（去扩展名的文件名；lnk 常带 "NN - 名称" 前缀）
    std::string target;     // 快捷方式目标路径（读 lnk 得，供展示，可空）
    std::string iconSource; // 图标源（目标 exe 路径，交 icon_extract 提取，可空）
    bool enabled = true;    // 扩展名为 .lnk 即启用，.lnk.disabled 为停用
};

// 一个 Win+X 分组（Group1/2/3），含若干项。
struct WinXGroup {
    std::string id;                  // 分组文件夹名（"Group1"/"Group2"/"Group3"）
    std::vector<WinXItem> items;     // 组内项（启用的 .lnk + 停用的 .lnk.disabled 都列）
};

// 扫描 Win+X 全部分组。分组倒序返回（Group3 在前，与系统菜单渲染一致）。
// WinX 根不存在（罕见）返回空 vector。
std::vector<WinXGroup> scan_winx();

// 序列化为 JSON：[{"id":"Group3","items":[{...}]}, ...]（前端拿到即数组，勿再 parse）。
std::string scan_winx_json();

// 启用 / 停用某项。id 为 scan 返回的相对路径。
// 启用：.lnk.disabled -> .lnk；停用：.lnk -> .lnk.disabled。改名即改可见性，可逆。
// 返回 {"ok":bool,"key":"..."}。
std::string set_winx_item_enabled(const std::string& id, bool enabled);

// 新增自定义 Win+X 项：在指定分组建一个指向 targetPath 的 .lnk，算合法
// System.Winx.Hash 写入（否则 explorer 忽略该项不显示）。
//   group      分组名（"Group1"/"Group2"/"Group3"，不存在则创建）
//   targetPath 快捷方式目标（exe / 文档等绝对路径）
//   args       命令行参数（可空）
//   name       显示名（生成 "<name>.lnk"，前端可自带 "NN - " 序号前缀控制排序）
// 返回 {"ok":bool,"key":"..."}（成功 key="ok"，含 hash 校验结果）。
std::string add_winx_item(const std::string& group, const std::string& targetPath,
                          const std::string& args, const std::string& name);

// 删除某 Win+X 项。id 为 scan 返回的相对路径。送回收站（可恢复，见开发规范安全红线）。
// 返回 {"ok":bool,"key":"..."}。
std::string del_winx_item(const std::string& id);

}  // namespace cmm
