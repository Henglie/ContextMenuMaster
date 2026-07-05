// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— Win11 新式右键菜单管理（接口契约）
//
//  Win11（22000+）的「现代」右键菜单不走经典注册表 verb / shellex，而是两套来源：
//    1) PackagedCom：MSIX/APPX 包在 AppxManifest.xml 里用 <desktop4:FileExplorerContextMenus>
//       声明的 IExplorerCommand 扩展（第三方装的，如夸克AI、Epson 打印等）。
//       枚举：HKCR\PackagedCom\Package\<全名> 列包 → PackageRepository 取安装路径
//       → 解析清单里 Verb 的 Clsid 与 ComServer 的 Class Id 取交集，得 handler CLSID。
//    2) SystemCommandStore：HKLM\...\Explorer\CommandStore\shell 下的 Windows.* 命令
//       （系统内置：分享、发送到手机等）。
//
//  启停机制（与经典层完全不同）：不是删键/改名，而是把 handler 的 CLSID 写进
//    「Shell Extensions\Blocked」黑名单（在名单里 = 停用）。两处：
//      机器级 HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Blocked
//      用户级 HKCU\Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Blocked
//    值名 = "{CLSID}"（大括号小写），值数据 = 显示名（仅备注用）。启用 = 从两处删掉。
//
//  安全边界（本版）：
//    - PackagedCom 项：可启停（黑名单双写，可逆）。这是第三方 Win11 菜单项的主体。
//    - SystemCommandStore 项：只读展示（canToggle=false）。系统内置命令若误用黑名单
//      封锁，可能导致整个 Win11 现代菜单回退到经典菜单（参考项目明确告警），不冒此险。
//    - 两个项目都无法「新建」Win11 现代菜单项（需 MSIX 包声明，非注册表可造），故无 add。
// ============================================================
#pragma once
#include <string>
#include <vector>

namespace cmm {

// 一个 Win11 现代菜单项。
struct Win11MenuItem {
    std::string id;           // 稳定标识："packaged|{clsid}" 或 "system|Windows.Xxx"
    std::string source;       // "packaged" / "system"
    std::string clsid;        // handler CLSID（"{...}" 大括号小写；system 项为其 handler guid，可空）
    std::string commandKey;   // system 项的 CommandStore 键名（如 "Windows.Share"；packaged 空）
    std::string label;        // 显示名（ComServer DisplayName / MUIVerb 解析 / 包名兜底）
    std::string iconSource;   // 图标源（handler exe 路径 / Icon 值；交 icon_extract 提取，可空）
    std::string package;      // 包全名（packaged 项；system 空）
    std::string contextTypes; // 适用场景文字（如 "Directory, File: .txt"；packaged，可空）
    bool enabled = true;      // 未在 Blocked 名单即启用
    bool blockedMachine = false; // 机器级黑名单命中
    bool blockedUser = false;    // 用户级黑名单命中
    bool canToggle = true;    // 是否允许启停（system 项 false，只读）
    bool system = false;      // 是否系统内置（前端更强确认 / 只读）
};

// 扫描全部 Win11 现代菜单项（PackagedCom + SystemCommandStore）。
// 非 Win11（<22000）或无来源时返回空 vector。
std::vector<Win11MenuItem> scan_win11menu();

// 序列化为 JSON 数组（前端拿到即数组，勿再 parse——项目踩坑 1）。
std::string scan_win11menu_json();

// 启用 / 停用某 Win11 项（仅 PackagedCom 项，system 项拒绝）。
//   clsid       handler CLSID（"{...}" 或裸 GUID，内部规范化为大括号小写）
//   displayName 停用时写入黑名单值数据（仅备注，可空）
//   enabled     true=从两处黑名单删除；false=写入机器级+用户级两处黑名单
// 返回 {"ok":bool,"key":"..."}。
std::string set_win11menu_enabled(const std::string& clsid,
                                  const std::string& displayName, bool enabled);

}  // namespace cmm
