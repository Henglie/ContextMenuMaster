// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— GUI 入口（WinMain，无控制台）
//
//  用 webview 单头库开一个原生窗口（系统自带 WebView2 承载），加载 Vue 界面。
//  后端通过 w.bind 把「系统语言检测」「右键菜单接管」两个能力暴露给前端。
//
//  语言注入（对标产品需求「启动时由 C++ 传给前端」）：
//    用 w.init(js) 在每个文档创建时、且早于页面自身脚本之前，注入一段
//    `window.__CMM_LANG__ = "zh-CN|zh-TW|en"`。前端一加载就能同步读到，
//    无需等一次异步 IPC 往返。另保留 get_sys_lang 绑定作为兜底/对齐姊妹项目。
// ============================================================
#define WEBVIEW_STATIC
#include "webview/webview.h"

#include <windows.h>
#include <commctrl.h>     // SetWindowSubclass
#include <shellapi.h>     // ShellExecuteW
#include <string>
#include <cstdio>

#include "platform/windows/i18n.h"
#include "platform/windows/context_menu.h"
#include "platform/windows/icon_extract.h"
#include "platform/windows/menu_scanner.h"
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace {

// 取 exe 所在目录（UTF-8）。用宽字符 API，避免中文路径被 ANSI 破坏。
// 对标 FairySave：作者项目路径含中文「我的项目源码」，必须宽字符全程。
std::string exe_dir_utf8() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring wp(buf);
    size_t slash = wp.find_last_of(L"\\/");
    if (slash != std::wstring::npos) wp = wp.substr(0, slash);

    int n = WideCharToMultiByte(CP_UTF8, 0, wp.c_str(), (int)wp.size(),
                                nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wp.c_str(), (int)wp.size(),
                        s.data(), n, nullptr, nullptr);
    return s;
}

// UTF-8 -> UTF-16。拼子进程命令行用（scene/clsid 都是 ASCII，但走统一通道更稳）。
std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

// 取 exe 所在目录（宽字符）。拼 CmmCli.exe 子进程路径用，全程宽字符防中文路径被破坏。
std::wstring exe_dir_wide() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring wp(buf);
    size_t slash = wp.find_last_of(L"\\/");
    if (slash != std::wstring::npos) wp = wp.substr(0, slash);
    return wp;
}

// 跑一个子进程并捕获它 stdout 的全部原始字节（UTF-8，不做任何码页转换）。
// 用于调 CmmCli probe —— 把高风险的「加载第三方 shell 扩展 DLL」隔离到子进程，
// 崩了只崩子进程、不连累 GUI（项目设计红线）。cmdLine 为可写宽字符命令行缓冲。
// 成功返回 true，outUtf8 填子进程 stdout 原始字节。超时（默认 8s）或失败返回 false。
bool run_capture(std::wstring cmdLine, std::string* outUtf8, DWORD timeoutMs = 8000) {
    outUtf8->clear();

    SECURITY_ATTRIBUTES sa = { sizeof(sa) };
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    // 读端不被子进程继承（避免子进程持有读端导致死等）。
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWrite;
    si.hStdError  = hWrite;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hWrite);   // 父进程持有的写端关掉，否则 ReadFile 永不收到 EOF
    if (!ok) { CloseHandle(hRead); return false; }

    // 读完子进程 stdout（写端全关后 ReadFile 返回 0 字节 = EOF）。
    std::string acc;
    char buf[4096];
    DWORD start = GetTickCount();
    for (;;) {
        DWORD avail = 0;
        // 轮询管道，避免子进程挂死时父进程在 ReadFile 上无限阻塞。
        if (PeekNamedPipe(hRead, nullptr, 0, nullptr, &avail, nullptr)) {
            if (avail > 0) {
                DWORD got = 0;
                if (ReadFile(hRead, buf, sizeof(buf), &got, nullptr) && got > 0) {
                    acc.append(buf, got);
                    continue;
                }
            }
        }
        // 子进程已退出且无更多数据 -> 收尾。
        if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) {
            DWORD got = 0;
            while (PeekNamedPipe(hRead, nullptr, 0, nullptr, &avail, nullptr) &&
                   avail > 0 &&
                   ReadFile(hRead, buf, sizeof(buf), &got, nullptr) && got > 0) {
                acc.append(buf, got);
            }
            break;
        }
        if (GetTickCount() - start > timeoutMs) {
            TerminateProcess(pi.hProcess, 1);   // 子进程挂死，杀掉
            break;
        }
        Sleep(15);
    }

    CloseHandle(hRead);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // 去掉尾部换行（CmmCli 输出末尾有 \n / \r\n）。
    while (!acc.empty() && (acc.back() == '\n' || acc.back() == '\r')) acc.pop_back();
    *outUtf8 = acc;
    return !acc.empty();
}

// 把本地路径拼成合法 file:// URL：反斜杠转正斜杠，非安全字符百分号编码。
// 不要加 ?t= 查询参数（FairySave 踩坑第 7 条：触发 unique security origin 警告）。
std::string to_file_url(const std::string& path) {
    std::string url = "file:///";
    for (unsigned char c : path) {
        if (c == '\\') { url += '/'; }
        else if (c == '/' || c == ':' || c == '.' || c == '-' || c == '_' ||
                 (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                 (c >= '0' && c <= '9')) {
            url += (char)c;
        } else {
            char hex[4];
            std::snprintf(hex, sizeof(hex), "%%%02X", c);
            url += hex;
        }
    }
    return url;
}

// 把 MenuResult 转成前端用的英文 key（i18n 后端给 key、前端翻译，踩坑第 8 条）。
// 返回 JSON 对象字符串，前端拿到即对象、不要再 JSON.parse（踩坑第 1 条）。
std::string menu_result_json(cmm::MenuResult r) {
    const char* key = "unknown";
    bool ok = false;
    switch (r) {
        case cmm::MenuResult::Ok:             key = "ok";              ok = true;  break;
        case cmm::MenuResult::AlreadyDone:    key = "already_done";    ok = true;  break;
        case cmm::MenuResult::RegistryFailed: key = "registry_failed"; ok = false; break;
        case cmm::MenuResult::RefreshFailed:  key = "refresh_failed";  ok = false; break;
    }
    std::string s = "{\"ok\":";
    s += ok ? "true" : "false";
    s += ",\"key\":\"";
    s += key;
    s += "\"}";
    return s;
}

// 从 webview bind 的 JSON 数组参数里取第一个字符串，并还原 JSON 转义。
// 前端传图标源如 ["imageres.dll,-5350"] 或 ["C:\\Windows\\x.ico"]；
// JSON 里反斜杠是 \\、双引号是 \"，这里逐字符还原成真实路径字符串。
std::string parse_first_string_arg(const std::string& req) {
    size_t q1 = req.find('"');
    if (q1 == std::string::npos) return "";
    std::string out;
    for (size_t i = q1 + 1; i < req.size(); ++i) {
        char c = req[i];
        if (c == '\\' && i + 1 < req.size()) {
            char n = req[++i];
            switch (n) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '/': out += '/';  break;
                case '\\': out += '\\'; break;
                case '"': out += '"';  break;
                default: out += n;     break;   // 其余转义按原字符收
            }
        } else if (c == '"') {
            break;   // 字符串结束
        } else {
            out += c;
        }
    }
    return out;
}

// 取 webview bind JSON 数组参数里第 index 个字符串（0 基），还原 JSON 转义。
// 用于多参 bind，如 rename_item 的 ["folder","verbId","新名字"]。
// 越界或非字符串返回空串。bool 参数请用 parse_bool_arg。
std::string parse_string_arg_at(const std::string& req, int index) {
    int found = 0;
    size_t i = 0;
    // 跳过开头的 '['。逐个找字符串字面量（被未转义的 " 包裹）。
    while (i < req.size()) {
        if (req[i] == '"') {
            // 解析一个字符串字面量到结束引号。
            std::string out;
            ++i;
            for (; i < req.size(); ++i) {
                char c = req[i];
                if (c == '\\' && i + 1 < req.size()) {
                    char n = req[++i];
                    switch (n) {
                        case 'n': out += '\n'; break;
                        case 't': out += '\t'; break;
                        case 'r': out += '\r'; break;
                        case '/': out += '/';  break;
                        case '\\': out += '\\'; break;
                        case '"': out += '"';  break;
                        default: out += n;     break;
                    }
                } else if (c == '"') {
                    break;   // 字符串结束
                } else {
                    out += c;
                }
            }
            if (found == index) return out;
            ++found;
        }
        ++i;
    }
    return "";
}

// 取 JSON 数组里的布尔值（出现 true 即真），如 ["id",true] -> true。
bool parse_bool_arg(const std::string& req) {
    return req.find("true") != std::string::npos;
}

// 关窗到托盘的子类化暂不做（本工具是轻量配置器，关窗即退出更符合直觉）。
// 这里保留子类化钩子以备后续加托盘；当前只透传默认处理。
LRESULT CALLBACK SubProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                         UINT_PTR, DWORD_PTR) {
    return DefSubclassProc(hwnd, msg, wp, lp);
}

}  // namespace

int APIENTRY WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // 第一个参数 true = 开调试（可右键检查，方便开发期调 UI）。
    webview::webview w(true, nullptr);
    w.set_title("鼠标右键主理人 Context Menu Master");
    w.set_size(980, 680, WEBVIEW_HINT_NONE);

    // —— 语言注入 ——
    // 在任何页面脚本之前注入系统语言标签，前端开局即可读 window.__CMM_LANG__。
    // 用 \" 转义，标签本身只可能是 zh-CN / zh-TW / en，无注入风险。
    {
        std::string tag = cmm::detect_ui_lang_tag();
        std::string js = "window.__CMM_LANG__ = \"" + tag + "\";";
        w.init(js);
    }

    // —— 绑定：系统语言（兜底/对齐 FairySave；前端首选 window.__CMM_LANG__）——
    // 返回带引号的 JSON 字符串字面量，前端拿到即字符串。
    w.bind("get_sys_lang", [](const std::string&) -> std::string {
        return "\"" + cmm::detect_ui_lang_tag() + "\"";
    });

    // —— 绑定：查询当前是否已接管为经典菜单 ——
    w.bind("get_menu_status", [](const std::string&) -> std::string {
        bool classic = cmm::is_classic_menu_enabled();
        return std::string("{\"classic\":") + (classic ? "true" : "false") + "}";
    });

    // —— 绑定：接管为经典菜单（写 HKCU 空 InprocServer32 + 优雅刷新 Explorer）——
    w.bind("enable_classic_menu", [](const std::string&) -> std::string {
        return menu_result_json(cmm::enable_classic_menu(true));
    });

    // —— 绑定：恢复 Win11 新版菜单（删 HKCU 键 + 优雅刷新 Explorer）——
    w.bind("restore_new_menu", [](const std::string&) -> std::string {
        return menu_result_json(cmm::restore_new_menu(true));
    });

    // —— 绑定：一键互换（按当前状态自动切换）——
    w.bind("toggle_classic_menu", [](const std::string&) -> std::string {
        return menu_result_json(cmm::toggle_classic_menu(true));
    });

    // —— 绑定：提取系统图标为 PNG data URI ——
    // 前端传图标源 ["imageres.dll,-5350"] / ["stock:32"] / ["C:\\x.ico"]，
    // 返回带引号的 JSON 字符串（data URI 或失败时空串 ""），前端拿到即字符串。
    // 失败回落黑白几何字符由前端负责，这里只忠实返回提取结果。
    w.bind("extract_icon", [](const std::string& req) -> std::string {
        std::string src = parse_first_string_arg(req);
        std::string uri = cmm::extract_icon_data_uri(src, 32);
        // data URI 字符集（base64 + "data:image/png;base64,"）对 JSON 安全，
        // 不含需转义的字符，直接两端加引号即可。
        return "\"" + uri + "\"";
    });

    // —— 绑定：扫描某场景的真实右键菜单项 ——
    // 前端传场景 id（"desktop"/"file"/"folder"/"folder_bg"/"drive"/"recycle"），
    // 返回 JSON 数组（前端拿到即数组，勿再 JSON.parse）。未知场景返回空数组。
    w.bind("scan_scene", [](const std::string& req) -> std::string {
        cmm::MenuScene scene;
        if (!cmm::scene_from_id(parse_first_string_arg(req), &scene)) return "[]";
        return cmm::scan_scene_json(scene);
    });

    // —— 绑定：探测 IContextMenu COM 扩展的真实动态菜单 —— 参数 ["scene","{clsid}"]
    // 像 7-Zip / 百度云 这类 handler 的折叠子菜单、本地化文字全是 DLL 运行时
    // QueryContextMenu 动态生成的，注册表静态读不到。这里把探测交给 CmmCli 子进程
    // （CmmCli probe <scene> <clsid>）跑——加载第三方扩展 DLL 有崩溃风险，隔离在
    // 子进程里崩了不连累 GUI。返回子进程吐出的 UTF-8 JSON（前端拿到即对象）。
    w.bind("probe_handler_menu", [](const std::string& req) -> std::string {
        std::string scene = parse_string_arg_at(req, 0);
        std::string clsid = parse_string_arg_at(req, 1);
        // 基本校验：scene 非空、clsid 形如 {....}（防命令行注入，只允许 CLSID 字符集）。
        if (scene.empty() || clsid.size() < 2 || clsid.front() != '{' || clsid.back() != '}')
            return "{\"ok\":false,\"key\":\"bad_clsid\"}";
        for (char c : clsid) {
            bool okc = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                       (c >= 'A' && c <= 'F') || c == '-' || c == '{' || c == '}';
            if (!okc) return "{\"ok\":false,\"key\":\"bad_clsid\"}";
        }
        for (char c : scene) {
            if (!((c >= 'a' && c <= 'z') || c == '_'))
                return "{\"ok\":false,\"key\":\"bad_scene\"}";
        }
        // 命令行：CmmCli.exe probe <scene> <clsid>（scene/clsid 已过白名单，安全）。
        std::wstring cli = exe_dir_wide() + L"\\CmmCli.exe";
        std::wstring cmd = L"\"" + cli + L"\" probe " +
                           utf8_to_wide(scene) + L" " + utf8_to_wide(clsid);
        std::string out;
        if (!run_capture(cmd, &out)) return "{\"ok\":false,\"key\":\"probe_failed\"}";
        // 去掉尾部换行/空白。子进程吐的就是合法 JSON，原样回传。
        while (!out.empty() && (out.back() == '\n' || out.back() == '\r' ||
                               out.back() == ' ' || out.back() == '\0'))
            out.pop_back();
        if (out.empty()) return "{\"ok\":false,\"key\":\"probe_failed\"}";
        return out;
    });

    // —— 绑定：改名 —— 参数 ["folder","verbId","新名字"]
    w.bind("rename_item", [](const std::string& req) -> std::string {
        cmm::MenuScene scene;
        if (!cmm::scene_from_id(parse_string_arg_at(req, 0), &scene))
            return "{\"ok\":false,\"key\":\"bad_scene\"}";
        return cmm::rename_item(scene, parse_string_arg_at(req, 1),
                                parse_string_arg_at(req, 2));
    });

    // —— 绑定：启用/停用 —— 参数 ["folder","verbId",true]
    w.bind("set_item_enabled", [](const std::string& req) -> std::string {
        cmm::MenuScene scene;
        if (!cmm::scene_from_id(parse_string_arg_at(req, 0), &scene))
            return "{\"ok\":false,\"key\":\"bad_scene\"}";
        return cmm::set_item_enabled(scene, parse_string_arg_at(req, 1),
                                     parse_bool_arg(req));
    });

    // —— 绑定：换图标 —— 参数 ["folder","verbId","imageres.dll,-100"]
    w.bind("set_item_icon", [](const std::string& req) -> std::string {
        cmm::MenuScene scene;
        if (!cmm::scene_from_id(parse_string_arg_at(req, 0), &scene))
            return "{\"ok\":false,\"key\":\"bad_scene\"}";
        return cmm::set_item_icon(scene, parse_string_arg_at(req, 1),
                                  parse_string_arg_at(req, 2));
    });

    // —— 绑定：批量后缀魔法 —— 参数 ["folder","id1,id2,id3"," 喵~"]
    w.bind("batch_append_suffix", [](const std::string& req) -> std::string {
        cmm::MenuScene scene;
        if (!cmm::scene_from_id(parse_string_arg_at(req, 0), &scene))
            return "{\"ok\":false,\"key\":\"bad_scene\"}";
        return cmm::batch_append_suffix(scene, parse_string_arg_at(req, 1),
                                        parse_string_arg_at(req, 2));
    });

    // —— 绑定：手动备份某场景 —— 参数 ["folder"]
    w.bind("backup_scene", [](const std::string& req) -> std::string {
        cmm::MenuScene scene;
        if (!cmm::scene_from_id(parse_first_string_arg(req), &scene))
            return "{\"ok\":false,\"key\":\"bad_scene\"}";
        return cmm::backup_scene(scene);
    });

    // —— 绑定：用默认浏览器打开外链 —— 参数 ["https://..."]
    // 只放行 https:// 开头的网址（白名单 schema），杜绝 file:// / 任意命令注入。
    // 关于页「项目主页」链接走这里，用 ShellExecuteW 交给系统默认浏览器。
    w.bind("open_url", [](const std::string& req) -> std::string {
        std::string url = parse_first_string_arg(req);
        if (url.rfind("https://", 0) != 0) return "{\"ok\":false,\"key\":\"bad_url\"}";
        ShellExecuteW(nullptr, L"open", utf8_to_wide(url).c_str(),
                      nullptr, nullptr, SW_SHOWNORMAL);
        return "{\"ok\":true}";
    });

    // 加载本地 Vue 界面（路径正确编码，支持中文目录）。
    std::string index = to_file_url(exe_dir_utf8() + "/ui/index.html");
    w.navigate(index);

    HWND hwnd = (HWND)w.window();
    SetWindowSubclass(hwnd, SubProc, 1, 0);

    w.run();   // 阻塞直到窗口关闭
    return 0;
}
