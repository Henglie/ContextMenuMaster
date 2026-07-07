// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— CLI 调试入口（CmmCli）
//
//  仅用于无界面验证后端逻辑（i18n 判定 / 注册表菜单接管），不是给最终用户的。
//  对标 FairySaveCli：产品要 GUI，CLI 只是开发期的探针。
//
//  用法：
//    CmmCli lang              打印检测到的系统 UI 语言标签（zh-CN/zh-TW/en）
//    CmmCli status            打印当前是否已接管为经典菜单
//    CmmCli enable            接管为经典菜单（写 HKCU + 刷新 Explorer）
//    CmmCli restore           恢复 Win11 新版菜单（删 HKCU + 刷新 Explorer）
//    CmmCli toggle            一键互换
//    CmmCli enable-norefresh  接管但不刷新（调试用，避免反复重启 Explorer）
//
//  注意（FairySave 踩坑第 9 条）：中文输出在 bash→cmd 下走 GBK 会乱码，
//  本 CLI 输出一律用纯 ASCII，避免在终端里得出错误结论。
// ============================================================
#include "platform/windows/i18n.h"
#include "platform/windows/context_menu.h"
#include "platform/windows/icon_extract.h"
#include "platform/windows/menu_scanner.h"
#include "platform/windows/com_menu_probe.h"
#include "platform/windows/winx_menu.h"
#include "platform/windows/sendto_menu.h"
#include "platform/windows/dragdrop_menu.h"
#include "platform/windows/shellnew_menu.h"
#include "platform/windows/openwith_menu.h"
#include "platform/windows/win11menu.h"
#include "platform/windows/custom_menu.h"
#include "platform/windows/filetype_menu.h"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

// 把 MenuResult 翻成 ASCII 文案（CLI 调试用，正式提示在前端做 i18n）。
const char* result_text(cmm::MenuResult r) {
    switch (r) {
        case cmm::MenuResult::Ok:             return "OK";
        case cmm::MenuResult::AlreadyDone:    return "ALREADY_DONE (no change needed)";
        case cmm::MenuResult::RegistryFailed: return "REGISTRY_FAILED";
        case cmm::MenuResult::RefreshFailed:  return "REFRESH_FAILED (registry changed; "
                                                     "applies after re-login)";
        default:                              return "UNKNOWN";
    }
}

void print_usage() {
    std::puts("CmmCli - ContextMenuMaster backend probe (debug only)\n"
              "Usage:\n"
              "  CmmCli lang             show detected system UI language tag\n"
              "  CmmCli status           show whether classic menu is enabled\n"
              "  CmmCli enable           take over -> classic menu (+ refresh)\n"
              "  CmmCli restore          restore Win11 new menu (+ refresh)\n"
              "  CmmCli toggle           swap classic <-> new\n"
              "  CmmCli enable-norefresh take over without restarting Explorer\n"
              "  CmmCli dumpicons <dll> <count> [dir]\n"
              "                          export icons #0..count-1 of <dll> as PNG\n"
              "                          (verify real resource indices on this PC)\n"
              "  CmmCli probe <scene> <clsid>\n"
              "                          probe an IContextMenu COM handler's live menu\n"
              "                          (instantiates the DLL; emits UTF-8 JSON tree)\n"
              "  CmmCli set-enabled <scene> <0|1> <id>\n"
              "                          enable(1)/disable(0) a verb or handler by id\n"
              "                          (writes real HKLM/HKCU hive; needs admin)\n"
              "  CmmCli winx list        list Win+X menu groups/items as JSON\n"
              "  CmmCli winx toggle <on|off> <lnkRelPath>\n"
              "                          enable/disable a Win+X item (renames .lnk)\n"
              "  CmmCli sendto list      list SendTo items as JSON\n"
              "  CmmCli sendto toggle <on|off> <fileName>\n"
              "                          enable/disable a SendTo item (renames)\n"
              "  CmmCli dragdrop list    list DragDropHandlers as JSON\n"
              "  CmmCli dragdrop toggle <on|off> <class\\name>\n"
              "                          enable/disable a drag-drop handler (moves key)\n"
              "  CmmCli shellnew list    list ShellNew (New menu) items as JSON\n"
              "  CmmCli shellnew toggle <on|off> <id>\n"
              "                          enable/disable a New-menu type (moves key; needs admin)\n"
              "  CmmCli openwith list    list Applications (Open with) apps as JSON\n"
              "\n"
              "  CmmCli win11 list       list Win11 modern menu items as JSON\n"
              "  CmmCli win11 toggle <on|off> <clsid> [displayName]");
}

}  // namespace

int main(int argc, char** argv) {
    // 控制台按 UTF-8 输出（本 CLI 只打印 ASCII，这一步是稳妥起见）。
    SetConsoleOutputCP(CP_UTF8);

    if (argc < 2) {
        print_usage();
        return 0;
    }

    const char* cmd = argv[1];

    if (std::strcmp(cmd, "lang") == 0) {
        std::printf("system UI language tag: %s\n", cmm::detect_ui_lang_tag().c_str());
        return 0;
    }
    if (std::strcmp(cmd, "status") == 0) {
        std::printf("classic menu enabled: %s\n",
                    cmm::is_classic_menu_enabled() ? "YES" : "NO");
        return 0;
    }
    if (std::strcmp(cmd, "enable") == 0) {
        std::printf("enable_classic_menu -> %s\n",
                    result_text(cmm::enable_classic_menu(true)));
        return 0;
    }
    if (std::strcmp(cmd, "enable-norefresh") == 0) {
        std::printf("enable_classic_menu(no refresh) -> %s\n",
                    result_text(cmm::enable_classic_menu(false)));
        return 0;
    }
    if (std::strcmp(cmd, "restore") == 0) {
        std::printf("restore_new_menu -> %s\n",
                    result_text(cmm::restore_new_menu(true)));
        return 0;
    }
    if (std::strcmp(cmd, "toggle") == 0) {
        std::printf("toggle_classic_menu -> %s\n",
                    result_text(cmm::toggle_classic_menu(true)));
        return 0;
    }
    if (std::strcmp(cmd, "dumpicons") == 0) {
        // 把某 DLL 的前 count 个图标按索引导成 PNG，肉眼确认真实资源索引。
        // 用法：CmmCli dumpicons imageres.dll 400 [输出目录]
        if (argc < 4) {
            std::puts("usage: CmmCli dumpicons <dll> <count> [dir]");
            return 1;
        }
        const char* dll = argv[2];
        int count = atoi(argv[3]);
        std::string dir = (argc >= 5) ? argv[4] : ".";
        if (count <= 0 || count > 2000) {
            std::puts("count out of range (1..2000)");
            return 1;
        }
        int ok = 0;
        for (int i = 0; i < count; ++i) {
            // 正数索引 = 零基索引；导出文件名带索引，方便对照。
            char src[512];
            std::snprintf(src, sizeof(src), "%s,%d", dll, i);
            char out[640];
            std::snprintf(out, sizeof(out), "%s/icon_%04d.png", dir.c_str(), i);
            if (cmm::extract_icon_to_file(src, 32, out)) ++ok;
        }
        std::printf("dumped %d/%d icons from %s into %s\n", ok, count, dll, dir.c_str());
        return 0;
    }

    if (std::strcmp(cmd, "scan") == 0) {
        // 扫描某场景的真实右键菜单项，打印项数 + 每项 id/kind/label（验证用）。
        // 用法：CmmCli scan <desktop|file|folder|folder_bg|drive|recycle>
        if (argc < 3) {
            std::puts("usage: CmmCli scan <desktop|file|folder|folder_bg|drive|recycle>");
            return 1;
        }
        cmm::MenuScene scene;
        if (!cmm::scene_from_id(argv[2], &scene)) {
            std::printf("unknown scene: %s\n", argv[2]);
            return 1;
        }
        std::vector<cmm::MenuItem> items = cmm::scan_scene(scene);
        std::printf("scene %s: %zu items\n", argv[2], items.size());
        for (const auto& it : items) {
            std::printf("  [%s] id=%s label=%s enabled=%d sys=%d\n",
                        it.kind == cmm::MenuItemKind::Verb ? "verb" : "hndl",
                        it.id.c_str(), it.label.c_str(),
                        it.enabled ? 1 : 0, it.system ? 1 : 0);
        }
        return 0;
    }

    if (std::strcmp(cmd, "probe") == 0) {
        // 探测某 IContextMenu COM handler 的真实动态菜单（实例化 + 跑 QueryContextMenu）。
        // 用法：CmmCli probe <scene> <clsid>
        // 这是会加载第三方 shell 扩展 DLL 的高风险操作，故独立在本 CLI 子进程跑，
        // 崩溃只崩本进程、不连累 GUI。输出一行 UTF-8 JSON（直接写原始字节，
        // 不走 SetConsoleOutputCP 的 GBK 转换——GUI 用管道按字节读，乱码会坏 JSON）。
        if (argc < 4) {
            std::puts("usage: CmmCli probe <scene> <clsid>");
            return 1;
        }
        cmm::MenuScene scene;
        if (!cmm::scene_from_id(argv[2], &scene)) {
            std::printf("{\"ok\":false,\"key\":\"bad_scene\"}\n");
            return 1;
        }
        // 可选第 4 参数：真实右键对象路径（诊断用，喂给挑剔的 handler）。
        // 中文路径在 argv（ANSI/GBK）里会丢字符，故走宽命令行取再转 UTF-8。
        std::string overridePath;
        {
            int wargc = 0;
            LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
            if (wargv) {
                if (wargc >= 5 && wargv[4] && wargv[4][0]) {
                    int n = WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                nullptr, 0, nullptr, nullptr);
                    if (n > 1) {
                        overridePath.resize(n - 1);
                        WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                            overridePath.data(), n, nullptr, nullptr);
                    }
                }
                LocalFree(wargv);
            }
        }
        std::string json = cmm::probe_handler_menu_json(scene, argv[3], overridePath);
        // 原始字节写出（stdout 默认文本模式会把 \n 转 \r\n，对 JSON 无害；
        // 但绝不能经 GBK 重编码，故用 fwrite 而非 printf 的 %s）。
        std::fwrite(json.data(), 1, json.size(), stdout);
        std::fwrite("\n", 1, 1, stdout);
        std::fflush(stdout);
        return 0;
    }

    if (std::strcmp(cmd, "set-enabled") == 0) {
        // 启停某菜单项（走 set_item_enabled 真实 hive 写路径，验证踩坑 21 三处修复）。
        // 用法：CmmCli set-enabled <scene> <on|off> <id>
        // id 可能是 handler:<名>，含中文/空格 → 与 probe 同理走宽命令行取，避免 argv
        // 的 ANSI/GBK 丢字符。输出一行 JSON。
        if (argc < 5) {
            std::puts("usage: CmmCli set-enabled <scene> <on|off> <id>");
            return 1;
        }
        cmm::MenuScene scene;
        if (!cmm::scene_from_id(argv[2], &scene)) {
            std::printf("{\"ok\":false,\"key\":\"bad_scene\"}\n");
            return 1;
        }
        bool enabled = (std::strcmp(argv[3], "on") == 0);
        // 宽命令行取 id（argv 索引 4：cmd/scene/onoff/id），转 UTF-8。
        std::string id;
        {
            int wargc = 0;
            LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
            if (wargv) {
                if (wargc >= 5 && wargv[4] && wargv[4][0]) {
                    int n = WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                nullptr, 0, nullptr, nullptr);
                    if (n > 1) {
                        id.resize(n - 1);
                        WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                            id.data(), n, nullptr, nullptr);
                    }
                }
                LocalFree(wargv);
            }
        }
        std::string json = cmm::set_item_enabled(scene, id, enabled);
        std::fwrite(json.data(), 1, json.size(), stdout);
        std::fwrite("\n", 1, 1, stdout);
        std::fflush(stdout);
        return 0;
    }

    if (std::strcmp(cmd, "winx") == 0) {
        // Win+X 菜单探针（枚举 / 启停）。winx_menu 走文件系统（%LOCALAPPDATA%\
        // Microsoft\Windows\WinX），非注册表。list 输出分组 JSON（原始 UTF-8 字节，
        // 不经 GBK，与 probe 同理）；toggle 改扩展名 .lnk<->.lnk.disabled。
        //   CmmCli winx list
        //   CmmCli winx toggle <on|off> <lnkRelPath>
        if (argc < 3) {
            std::puts("usage: CmmCli winx <list|toggle>");
            return 1;
        }
        if (std::strcmp(argv[2], "list") == 0) {
            std::string json = cmm::scan_winx_json();
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        if (std::strcmp(argv[2], "toggle") == 0) {
            // CmmCli winx toggle <on|off> <lnkRelPath>
            if (argc < 5) {
                std::puts("usage: CmmCli winx toggle <on|off> <lnkRelPath>");
                return 1;
            }
            bool enabled = (std::strcmp(argv[3], "on") == 0);
            // id（相对路径）含中文/空格 → 走宽命令行取（argv 索引 4），转 UTF-8。
            std::string id;
            {
                int wargc = 0;
                LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
                if (wargv) {
                    if (wargc >= 5 && wargv[4] && wargv[4][0]) {
                        int n = WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                    nullptr, 0, nullptr, nullptr);
                        if (n > 1) {
                            id.resize(n - 1);
                            WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                id.data(), n, nullptr, nullptr);
                        }
                    }
                    LocalFree(wargv);
                }
            }
            std::string json = cmm::set_winx_item_enabled(id, enabled);
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        std::printf("unknown winx subcommand: %s\n", argv[2]);
        return 1;
    }

    // —— 发送到（SendTo）：文件系统机制，仿 winx —— //
    //   CmmCli sendto list
    //   CmmCli sendto toggle <on|off> <fileName>
    if (std::strcmp(cmd, "sendto") == 0) {
        if (argc < 3) {
            std::puts("usage: CmmCli sendto <list|toggle>");
            return 1;
        }
        if (std::strcmp(argv[2], "list") == 0) {
            std::string json = cmm::scan_sendto_json();
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        if (std::strcmp(argv[2], "toggle") == 0) {
            if (argc < 5) {
                std::puts("usage: CmmCli sendto toggle <on|off> <fileName>");
                return 1;
            }
            bool enabled = (std::strcmp(argv[3], "on") == 0);
            // id（文件名）含中文/空格 → 走宽命令行取（索引 4）。
            std::string id;
            {
                int wargc = 0;
                LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
                if (wargv) {
                    if (wargc >= 5 && wargv[4] && wargv[4][0]) {
                        int n = WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                    nullptr, 0, nullptr, nullptr);
                        if (n > 1) {
                            id.resize(n - 1);
                            WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                id.data(), n, nullptr, nullptr);
                        }
                    }
                    LocalFree(wargv);
                }
            }
            std::string json = cmm::set_sendto_enabled(id, enabled);
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        // CmmCli sendto rename <fileName> <newName>
        // CmmCli sendto seticon <fileName> <iconSource>
        // 两个参数都可能含中文/空格 → 统一走宽命令行取（索引 3、4）。
        if (std::strcmp(argv[2], "rename") == 0 ||
            std::strcmp(argv[2], "seticon") == 0) {
            if (argc < 5) {
                std::printf("usage: CmmCli sendto %s <fileName> <arg>\n", argv[2]);
                return 1;
            }
            std::string a3, a4;
            {
                int wargc = 0;
                LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
                if (wargv) {
                    auto pick = [](LPWSTR w) -> std::string {
                        std::string s;
                        if (w && w[0]) {
                            int n = WideCharToMultiByte(CP_UTF8, 0, w, -1,
                                                        nullptr, 0, nullptr, nullptr);
                            if (n > 1) {
                                s.resize(n - 1);
                                WideCharToMultiByte(CP_UTF8, 0, w, -1,
                                                    s.data(), n, nullptr, nullptr);
                            }
                        }
                        return s;
                    };
                    if (wargc >= 5) { a3 = pick(wargv[3]); a4 = pick(wargv[4]); }
                    LocalFree(wargv);
                }
            }
            std::string json = (std::strcmp(argv[2], "rename") == 0)
                ? cmm::rename_sendto_item(a3, a4)
                : cmm::set_sendto_icon(a3, a4);
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        std::printf("unknown sendto subcommand: %s\n", argv[2]);
        return 1;
    }

    // —— 拖放处理程序（DragDropHandlers）：handler 搬家启停 —— //
    //   CmmCli dragdrop list
    //   CmmCli dragdrop toggle <on|off> <类\name>
    if (std::strcmp(cmd, "dragdrop") == 0) {
        if (argc < 3) {
            std::puts("usage: CmmCli dragdrop <list|toggle>");
            return 1;
        }
        if (std::strcmp(argv[2], "list") == 0) {
            std::string json = cmm::scan_dragdrop_json();
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        if (std::strcmp(argv[2], "toggle") == 0) {
            if (argc < 5) {
                std::puts("usage: CmmCli dragdrop toggle <on|off> <class\\name>");
                return 1;
            }
            bool enabled = (std::strcmp(argv[3], "on") == 0);
            std::string id;
            {
                int wargc = 0;
                LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
                if (wargv) {
                    if (wargc >= 5 && wargv[4] && wargv[4][0]) {
                        int n = WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                    nullptr, 0, nullptr, nullptr);
                        if (n > 1) {
                            id.resize(n - 1);
                            WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                id.data(), n, nullptr, nullptr);
                        }
                    }
                    LocalFree(wargv);
                }
            }
            std::string json = cmm::set_dragdrop_enabled(id, enabled);
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        std::printf("unknown dragdrop subcommand: %s\n", argv[2]);
        return 1;
    }

    // —— 新建菜单（ShellNew）：注册表 ShellNew<->-ShellNew 搬家启停 —— //
    //   CmmCli shellnew list
    //   CmmCli shellnew toggle <on|off> <id>
    if (std::strcmp(cmd, "shellnew") == 0) {
        if (argc < 3) {
            std::puts("usage: CmmCli shellnew <list|toggle>");
            return 1;
        }
        if (std::strcmp(argv[2], "list") == 0) {
            std::string json = cmm::scan_shellnew_json();
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        if (std::strcmp(argv[2], "toggle") == 0) {
            if (argc < 5) {
                std::puts("usage: CmmCli shellnew toggle <on|off> <id>");
                return 1;
            }
            bool enabled = (std::strcmp(argv[3], "on") == 0);
            std::string id;
            {
                int wargc = 0;
                LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
                if (wargv) {
                    if (wargc >= 5 && wargv[4] && wargv[4][0]) {
                        int n = WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                    nullptr, 0, nullptr, nullptr);
                        if (n > 1) {
                            id.resize(n - 1);
                            WideCharToMultiByte(CP_UTF8, 0, wargv[4], -1,
                                                id.data(), n, nullptr, nullptr);
                        }
                    }
                    LocalFree(wargv);
                }
            }
            std::string json = cmm::set_shellnew_enabled(id, enabled);
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        std::printf("unknown shellnew subcommand: %s\n", argv[2]);
        return 1;
    }

    // —— 打开方式（OpenWith）：只读枚举 Applications —— //
    //   CmmCli openwith list
    if (std::strcmp(cmd, "openwith") == 0) {
        if (argc < 3 || std::strcmp(argv[2], "list") != 0) {
            std::puts("usage: CmmCli openwith list");
            return 1;
        }
        std::string json = cmm::scan_openwith_json();
        std::fwrite(json.data(), 1, json.size(), stdout);
        std::fwrite("\n", 1, 1, stdout);
        std::fflush(stdout);
        return 0;
    }

    // Win11 现代菜单：
    //   CmmCli win11 list
    //   CmmCli win11 toggle <on|off> <clsid> [displayName]
    if (std::strcmp(cmd, "win11") == 0) {
        if (argc < 3) {
            std::puts("usage: CmmCli win11 <list|toggle>");
            return 1;
        }
        if (std::strcmp(argv[2], "list") == 0) {
            std::string json = cmm::scan_win11menu_json();
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        if (std::strcmp(argv[2], "toggle") == 0) {
            if (argc < 5) {
                std::puts("usage: CmmCli win11 toggle <on|off> <clsid> [displayName]");
                return 1;
            }
            bool enabled = (std::strcmp(argv[3], "on") == 0);
            std::string clsid = argv[4];
            std::string disp = (argc >= 6) ? argv[5] : "";
            std::string json = cmm::set_win11menu_enabled(clsid, disp, enabled);
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        std::printf("unknown win11 subcommand: %s\n", argv[2]);
        return 1;
    }

    //   CmmCli custom add <scene> <label> <kind> <target> [icon] [top|bottom]
    //   CmmCli custom del <scene> <id>
    //   kind = program|folder|url|command
    if (std::strcmp(cmd, "custom") == 0) {
        if (argc < 3) {
            std::puts("usage: CmmCli custom <add|del>");
            return 1;
        }
        cmm::MenuScene scene;
        if (std::strcmp(argv[2], "add") == 0) {
            if (argc < 7) {
                std::puts("usage: CmmCli custom add <scene> <label> <kind> <target> [icon] [top|bottom]");
                return 1;
            }
            if (!cmm::scene_from_id(argv[3], &scene)) {
                std::printf("unknown scene: %s\n", argv[3]);
                return 1;
            }
            std::string icon = (argc >= 8) ? argv[7] : "";
            std::string pos  = (argc >= 9) ? argv[8] : "";
            std::string json = cmm::add_custom_item(scene, argv[4], argv[5], argv[6], icon, pos);
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        if (std::strcmp(argv[2], "del") == 0) {
            if (argc < 5) {
                std::puts("usage: CmmCli custom del <scene> <id>");
                return 1;
            }
            if (!cmm::scene_from_id(argv[3], &scene)) {
                std::printf("unknown scene: %s\n", argv[3]);
                return 1;
            }
            std::string json = cmm::del_custom_item(scene, argv[4]);
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
            return 0;
        }
        std::printf("unknown custom subcommand: %s\n", argv[2]);
        return 1;
    }

    // —— 文件类型右键菜单（某具体扩展名）—— //
    //   CmmCli filetype scan   <ext>
    //   CmmCli filetype toggle <on|off> <ext> <id>
    //   CmmCli filetype add    <ext> <label> <kind> <target> [icon] [top|bottom]
    //   CmmCli filetype del    <ext> <id>
    //   CmmCli filetype backup <ext>
    // ext 为 ASCII，但 label/target/id 可能含中文 → 统一走宽命令行取参数。
    if (std::strcmp(cmd, "filetype") == 0) {
        if (argc < 3) {
            std::puts("usage: CmmCli filetype <scan|toggle|add|del|backup>");
            return 1;
        }
        // 宽命令行按 argv 下标取 UTF-8 参数。越界返回空串。
        auto wpick = [](int idx) -> std::string {
            std::string s;
            int wargc = 0;
            LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
            if (wargv) {
                if (idx < wargc && wargv[idx] && wargv[idx][0]) {
                    int n = WideCharToMultiByte(CP_UTF8, 0, wargv[idx], -1,
                                                nullptr, 0, nullptr, nullptr);
                    if (n > 1) {
                        s.resize(n - 1);
                        WideCharToMultiByte(CP_UTF8, 0, wargv[idx], -1,
                                            s.data(), n, nullptr, nullptr);
                    }
                }
                LocalFree(wargv);
            }
            return s;
        };
        auto emit = [](const std::string& json) {
            std::fwrite(json.data(), 1, json.size(), stdout);
            std::fwrite("\n", 1, 1, stdout);
            std::fflush(stdout);
        };

        if (std::strcmp(argv[2], "scan") == 0) {
            if (argc < 4) { std::puts("usage: CmmCli filetype scan <ext>"); return 1; }
            emit(cmm::scan_filetype_json(argv[3]));
            return 0;
        }
        if (std::strcmp(argv[2], "toggle") == 0) {
            if (argc < 5) {
                std::puts("usage: CmmCli filetype toggle <on|off> <ext> <id>");
                return 1;
            }
            bool enabled = (std::strcmp(argv[3], "on") == 0);
            emit(cmm::set_filetype_enabled(wpick(4), wpick(5), enabled));
            return 0;
        }
        if (std::strcmp(argv[2], "add") == 0) {
            if (argc < 7) {
                std::puts("usage: CmmCli filetype add <ext> <label> <kind> <target> [icon] [top|bottom]");
                return 1;
            }
            emit(cmm::add_filetype_item(wpick(3), wpick(4), wpick(5), wpick(6),
                                        wpick(7), wpick(8)));
            return 0;
        }
        if (std::strcmp(argv[2], "del") == 0) {
            if (argc < 5) { std::puts("usage: CmmCli filetype del <ext> <id>"); return 1; }
            emit(cmm::del_filetype_item(wpick(3), wpick(4)));
            return 0;
        }
        if (std::strcmp(argv[2], "backup") == 0) {
            if (argc < 4) { std::puts("usage: CmmCli filetype backup <ext>"); return 1; }
            emit(cmm::backup_filetype(argv[3]));
            return 0;
        }
        std::printf("unknown filetype subcommand: %s\n", argv[2]);
        return 1;
    }

    std::printf("unknown command: %s\n\n", cmd);
    print_usage();
    return 1;
}
