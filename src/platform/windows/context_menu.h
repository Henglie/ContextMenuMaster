// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— Win11 经典/新版右键菜单接管
//
//  Win11 默认用「新版」精简右键菜单，把传统项目折叠进「显示更多选项」。
//  系统留了一个 per-user 开关：在 HKCU 下伪造一个空的 InprocServer32，
//  让那个负责「新版菜单」的 Shell 扩展 CLSID 加载失败，资源管理器就回落到
//  Win10 经典菜单。这套做法只写 HKCU、无需管理员、随时可逆（删键即恢复）。
//
//  Windows 11 ships a trimmed "new" context menu and hides classic entries
//  behind "Show more options". There is a per-user switch: register an *empty*
//  InprocServer32 under HKCU for the CLSID that drives the new menu, so the
//  shell extension fails to load and Explorer falls back to the Win10 classic
//  menu. HKCU-only, no admin rights, fully reversible (delete the key to undo).
// ============================================================
#pragma once

namespace cmm {

// 经典菜单接管的结果码。前端据此给中英双语提示。
// Result codes for the classic-menu takeover. Frontend maps to i18n messages.
enum class MenuResult {
    Ok,              // 操作成功 / succeeded
    AlreadyDone,     // 目标状态已是当前状态，无需改动 / already in target state
    RegistryFailed,  // 注册表读写失败 / registry read/write failed
    RefreshFailed    // 注册表已改，但刷新 Explorer 失败（重登后仍会生效）
                     // registry changed but Explorer refresh failed (takes effect after re-login)
};

// 当前是否已启用「经典菜单」（即我们的接管键是否存在且有效）。
// Whether the classic menu is currently enabled (our takeover key present & valid).
bool is_classic_menu_enabled();

// 启用经典菜单：写入 HKCU 空 InprocServer32。refresh=true 时顺带优雅重启 Explorer。
// Enable the classic menu by writing the empty HKCU InprocServer32 key.
MenuResult enable_classic_menu(bool refresh = true);

// 恢复 Win11 新版菜单：删除我们写入的 HKCU 键（回到系统默认）。
// Restore the Win11 new menu by deleting our HKCU key (back to system default).
MenuResult restore_new_menu(bool refresh = true);

// 一键互换：经典<->新版（依据当前状态自动切换）。
// One-click toggle between classic and new menu based on current state.
MenuResult toggle_classic_menu(bool refresh = true);

// 优雅重启 explorer.exe，让菜单改动「瞬间生效」而无需注销重登。
//   先用任务栏未公开的「优雅退出」消息让 Explorer 干净下班（不触发崩溃恢复、
//   不残留桌面），等它退出后我们再把它请回来；失败才回落到结束进程。
// Gracefully restart explorer.exe so changes apply instantly without re-login:
// post the taskbar's undocumented graceful-exit message, wait, then relaunch.
bool restart_explorer();

}  // namespace cmm
