// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 打开方式（OpenWith）应用列表（接口契约）
//
//  右键「打开方式」列出的候选应用，来源之一是注册在 Applications 下的程序：
//      HKLM\SOFTWARE\Classes\Applications\<app.exe>\shell\open\command
//      HKCU\Software\Classes\Applications\<app.exe>\shell\open\command
//  每个 <app.exe> 即一个可选「打开方式」应用，读其 FriendlyAppName（或 exe 名）
//  与 open 命令展示。
//
//  与经典菜单/handler 不同：「打开方式」是 Windows 的动态文件关联，不是简单的
//  开关键。安全起见本模块【只做只读枚举】（canToggle=false）——启停/移除某应用
//  涉及改系统文件关联，风险偏高，暂不提供写操作（见 PROGRESS 待办）。
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
    bool enabled = true;    // 只读枚举，恒 true
    bool canToggle = false; // 本模块不支持启停（动态文件关联，风险高）
};

// 扫描 Applications 下注册的全部应用（HKLM + HKCU 合并，去重）。
std::vector<OpenWithItem> scan_openwith();

// 序列化为 JSON 数组（前端拿到即数组，勿再 parse）。
std::string scan_openwith_json();

}  // namespace cmm
