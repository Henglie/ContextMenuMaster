// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— Win11 经典/新版右键菜单接管（实现）
// ============================================================
#include "platform/windows/context_menu.h"

#include <windows.h>
#include <string>

namespace cmm {

namespace {

// Win11 「新版右键菜单」的稳定 CLSID。在它的 InprocServer32 下写一个空默认值，
// 就能让新版菜单的 Shell 扩展加载失败，Explorer 回落到经典菜单。
// The stable CLSID that drives the Win11 new context menu. An empty default value
// under its InprocServer32 makes the shell extension fail to load → classic menu.
constexpr wchar_t kClassicKeyPath[] =
    L"Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InprocServer32";

// 父键路径（删除时连同它一起清掉，不留空壳）。
// Parent key path (deleted too on restore, so we leave no empty shell behind).
constexpr wchar_t kClassicParentPath[] =
    L"Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}";

// 递归删除一个 HKCU 子键（RegDeleteTreeW，Vista+）。返回是否成功或本就不存在。
// Recursively delete an HKCU subkey. Treats "not found" as success.
bool delete_key_tree(const wchar_t* subkey) {
    LSTATUS st = RegDeleteTreeW(HKEY_CURRENT_USER, subkey);
    return st == ERROR_SUCCESS || st == ERROR_FILE_NOT_FOUND;
}

}  // namespace

bool is_classic_menu_enabled() {
    // 接管的判据：InprocServer32 键存在，且其默认值为空字符串（""）。
    // 我们正是靠「空默认值」让扩展加载失败的；非空说明不是我们写的，不算接管。
    // Enabled iff the InprocServer32 key exists AND its default value is empty —
    // the empty default is exactly what disables the extension.
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kClassicKeyPath, 0, KEY_QUERY_VALUE, &hKey)
            != ERROR_SUCCESS) {
        return false;
    }

    wchar_t buf[8] = {0};
    DWORD cb = sizeof(buf);
    DWORD type = 0;
    // 读默认值（lpValueName = nullptr）。我们写的是空串，长度应为 0 或仅含终止符。
    LSTATUS st = RegQueryValueExW(hKey, nullptr, nullptr, &type,
                                  reinterpret_cast<LPBYTE>(buf), &cb);
    RegCloseKey(hKey);

    if (st != ERROR_SUCCESS) {
        // 键在但默认值读不出来：视为存在即接管（保守）。
        return true;
    }
    // 空串：cb<=sizeof(wchar_t) 且首字符为 0。即为我们的接管标记。
    return (type == REG_SZ) && (cb <= sizeof(wchar_t) || buf[0] == L'\0');
}

MenuResult enable_classic_menu(bool refresh) {
    if (is_classic_menu_enabled()) {
        return MenuResult::AlreadyDone;
    }

    HKEY hKey = nullptr;
    DWORD disposition = 0;
    // 创建（或打开）InprocServer32 键。HKCU 无需管理员权限。
    LSTATUS st = RegCreateKeyExW(HKEY_CURRENT_USER, kClassicKeyPath, 0, nullptr,
                                 REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr,
                                 &hKey, &disposition);
    if (st != ERROR_SUCCESS) {
        return MenuResult::RegistryFailed;
    }

    // 写默认值为空字符串。空 InprocServer32 → 扩展加载失败 → 回落经典菜单。
    // 写一个仅含终止符的空串（cbData 含终止符 = sizeof(wchar_t)）。
    const wchar_t empty[] = L"";
    st = RegSetValueExW(hKey, nullptr, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(empty), sizeof(wchar_t));
    RegCloseKey(hKey);

    if (st != ERROR_SUCCESS) {
        return MenuResult::RegistryFailed;
    }

    if (refresh && !restart_explorer()) {
        return MenuResult::RefreshFailed;
    }
    return MenuResult::Ok;
}

MenuResult restore_new_menu(bool refresh) {
    if (!is_classic_menu_enabled()) {
        return MenuResult::AlreadyDone;
    }

    // 删除我们写入的整棵子键（含 InprocServer32 与其父 CLSID 壳），回到系统默认。
    // 先删子键再删父壳；任一失败即视为注册表失败。
    if (!delete_key_tree(kClassicKeyPath)) {
        return MenuResult::RegistryFailed;
    }
    // 父键可能还残留空壳，一并清掉（失败不致命，已达成回落目的）。
    delete_key_tree(kClassicParentPath);

    if (refresh && !restart_explorer()) {
        return MenuResult::RefreshFailed;
    }
    return MenuResult::Ok;
}

MenuResult toggle_classic_menu(bool refresh) {
    return is_classic_menu_enabled() ? restore_new_menu(refresh)
                                     : enable_classic_menu(refresh);
}

// 重启 explorer.exe，让右键菜单改动「瞬间生效」。
//
// 设计红线（修复「资源管理器卡死 → 任务栏/桌面消失再也回不来」的事故）：
//   1) 绝不在调用线程里同步「杀 explorer + 轮询等待」。本函数被 GUI 的 bind 回调
//      （WebView2 UI 线程）调用，任何 Sleep 轮询都会冻住界面，甚至被系统判为无响应；
//      上一版正是在 UI 线程里 SMTO_BLOCK + 长轮询，杀掉 shell 后自己卡死、再没机会
//      把 shell 拉回来 —— 于是任务栏、桌面全没了。
//   2) 把「杀 + 重启」交给一个完全分离（DETACHED）的子进程去做，本函数投递完即返回。
//      子进程独立于 GUI 存活，哪怕 GUI 此刻卡顿或被关闭，重启序列也照样跑完，
//      保证最终一定有一个活着的 shell。这是「不留无 shell 状态」的根本保证。
//
// 子进程命令：taskkill /F /IM explorer.exe（确定性终止，避免友好关闭被忽略导致
//   半死不活）→ 等 1 秒 → start explorer.exe（重新拉起外壳）。用 cmd /c 串起来。
//   另：Win10/11 默认 AutoRestartShell=1，explorer 被杀后系统本身也会自动重生 shell，
//   我们显式重启 + 系统自动重生 = 双保险，桌面不可能回不来。
bool restart_explorer() {
    wchar_t sysDir[MAX_PATH] = {0};
    if (GetSystemDirectoryW(sysDir, MAX_PATH) == 0) return false;
    std::wstring comspec = std::wstring(sysDir) + L"\\cmd.exe";

    // /c 执行完即退出。timeout 用 /nobreak 等 1 秒给系统清理时间；
    // start "" explorer.exe 不带路径让系统从 %windir% 找，重新拉起桌面+任务栏。
    // 整串交给分离子进程，本进程不等待、不阻塞。
    std::wstring cmdline =
        L"\"" + comspec + L"\" /c taskkill /F /IM explorer.exe >nul 2>&1 & "
        L"timeout /t 1 /nobreak >nul & start \"\" explorer.exe";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;          // 不闪黑框
    PROCESS_INFORMATION pi = {0};

    std::wstring mutableCmd = cmdline;  // CreateProcessW 需要可写命令行缓冲
    // DETACHED_PROCESS | CREATE_NO_WINDOW：子进程脱离本进程，独立活到序列跑完。
    BOOL ok = CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
                             DETACHED_PROCESS | CREATE_NO_WINDOW,
                             nullptr, nullptr, &si, &pi);
    if (!ok) return false;
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);   // 不等待：子进程分离运行，本函数立即返回，UI 不冻。
    return true;
}

}  // namespace cmm
