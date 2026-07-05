// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 自建右键菜单项（实现）
//  见 custom_menu.h 顶部机制说明。
// ============================================================
#include "platform/windows/custom_menu.h"

#include <windows.h>

#include <string>

namespace cmm {
namespace {

// 自建项 verb 键名前缀。scan_scene 见此前缀即知本工具自建、可编辑删除。
constexpr wchar_t kPrefix[] = L"cmm_";

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

std::string wide_to_utf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

std::string json_escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:   o += c;
        }
    }
    return o;
}

std::string result_json(bool ok, const char* key) {
    std::string o = "{\"ok\":";
    o += ok ? "true" : "false";
    o += ",\"key\":\"";
    o += json_escape(key ? key : "");
    o += "\"}";
    return o;
}

std::string result_json_id(bool ok, const char* key, const std::wstring& id) {
    std::string o = "{\"ok\":";
    o += ok ? "true" : "false";
    o += ",\"key\":\"";
    o += json_escape(key ? key : "");
    o += "\",\"id\":\"";
    o += json_escape(wide_to_utf8(id));
    o += "\"}";
    return o;
}

// 场景 -> shell 根（verb 来源，相对 Classes）。与 menu_scanner 内同名映射一致。
// 这里单列一份而非导出跨模块，是因为映射稳定（右键场景根几乎不变），且避免把
// menu_scanner 的匿名空间 helper 提成公共 API 扩大耦合面。
const wchar_t* scene_shell_root(MenuScene scene) {
    switch (scene) {
        case MenuScene::DesktopBackground:   return L"DesktopBackground\\Shell";
        case MenuScene::AllFiles:            return L"*\\shell";
        case MenuScene::Directory:           return L"Directory\\shell";
        case MenuScene::DirectoryBackground: return L"Directory\\Background\\shell";
        case MenuScene::Drive:               return L"Drive\\shell";
        case MenuScene::RecycleBin:
            return L"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\shell";
        case MenuScene::AllFileSystemObjects: return L"AllFilesystemObjects\\shell";
        case MenuScene::Computer:
            return L"CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\shell";
        case MenuScene::Library:             return L"LibraryFolder\\shell";
    }
    return L"";
}

// 背景类场景（无选中对象）：占位符用 %V（当前文件夹路径）而非 %1（选中项）。
bool scene_is_background(MenuScene scene) {
    return scene == MenuScene::DesktopBackground
        || scene == MenuScene::DirectoryBackground;
}

// 按执行类型组装 command 命令行。target 已是 wide。
//   program : "<target>" "%1"（对象场景）/ "<target>" "%V"（背景场景）
//   folder  : explorer.exe "<target>"
//   url     : cmd /c start "" "<target>"
//   command : 原样 target（高级用户自负占位符）
std::wstring build_command(MenuScene scene, const std::string& kind,
                           const std::wstring& target) {
    if (kind == "folder") {
        return L"explorer.exe \"" + target + L"\"";
    }
    if (kind == "url") {
        return L"cmd /c start \"\" \"" + target + L"\"";
    }
    if (kind == "command") {
        return target;  // 原样，用户可自带 %1/%V 与引号
    }
    // program（默认）
    const wchar_t* ph = scene_is_background(scene) ? L"%V" : L"%1";
    return L"\"" + target + L"\" \"" + ph + L"\"";
}

// 打开（或创建）某 verb 键：HKCU\Software\Classes\<shellRoot>\<verbId>。
// create=true 不存在则建。out 需调用者关闭。
bool open_verb_key(MenuScene scene, const std::wstring& verbId, bool create,
                   REGSAM access, HKEY* out) {
    std::wstring path = L"Software\\Classes\\";
    path += scene_shell_root(scene);
    path += L"\\";
    path += verbId;
    if (create) {
        return RegCreateKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, nullptr, 0,
                               access, nullptr, out, nullptr) == ERROR_SUCCESS;
    }
    return RegOpenKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, access, out) == ERROR_SUCCESS;
}

bool verb_exists(MenuScene scene, const std::wstring& verbId) {
    HKEY h = nullptr;
    if (open_verb_key(scene, verbId, false, KEY_READ, &h)) { RegCloseKey(h); return true; }
    return false;
}

bool set_str(HKEY h, const wchar_t* name, const std::wstring& val) {
    return RegSetValueExW(h, name, 0, REG_SZ,
                          reinterpret_cast<const BYTE*>(val.c_str()),
                          (DWORD)((val.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
}

// 写一个自建项的全部值（verb 键 + command 子键）。verb 键须已打开为可写 hVerb。
bool write_item_values(MenuScene scene, HKEY hVerb, const std::wstring& verbId,
                       const std::string& label, const std::string& kind,
                       const std::string& target, const std::string& iconSource,
                       const std::string& position) {
    std::wstring wLabel  = utf8_to_wide(label);
    std::wstring wTarget = utf8_to_wide(target);
    std::wstring wIcon   = utf8_to_wide(iconSource);

    // 显示名：写 MUIVerb（现代）与默认值（兼容旧解析）双写，稳妥。
    set_str(hVerb, L"MUIVerb", wLabel);
    set_str(hVerb, nullptr, wLabel);

    // 图标：非空才写；空则删（编辑时清除图标）。
    if (!wIcon.empty()) set_str(hVerb, L"Icon", wIcon);
    else RegDeleteValueW(hVerb, L"Icon");

    // 位置：top/bottom 才写 Position；空则删。
    if (position == "top")         set_str(hVerb, L"Position", L"Top");
    else if (position == "bottom") set_str(hVerb, L"Position", L"Bottom");
    else RegDeleteValueW(hVerb, L"Position");

    // command 子键。
    std::wstring cmd = build_command(scene, kind, wTarget);
    HKEY hCmd = nullptr;
    std::wstring cmdPath = L"Software\\Classes\\";
    cmdPath += scene_shell_root(scene);
    cmdPath += L"\\";
    cmdPath += verbId;
    cmdPath += L"\\command";
    if (RegCreateKeyExW(HKEY_CURRENT_USER, cmdPath.c_str(), 0, nullptr, 0,
                        KEY_SET_VALUE, nullptr, &hCmd, nullptr) != ERROR_SUCCESS)
        return false;
    bool ok = set_str(hCmd, nullptr, cmd);
    RegCloseKey(hCmd);
    return ok;
}

bool is_custom_id(const std::wstring& id) {
    return id.rfind(kPrefix, 0) == 0;
}

}  // namespace

std::string add_custom_item(MenuScene scene, const std::string& label,
                            const std::string& kind, const std::string& target,
                            const std::string& iconSource, const std::string& position) {
    if (label.empty() || target.empty()) return result_json(false, "bad_arg");
    if (*scene_shell_root(scene) == L'\0') return result_json(false, "bad_scene");

    // 生成唯一 verb id：cmm_<tick>，冲突则递增后缀。
    std::wstring verbId = kPrefix + std::to_wstring(GetTickCount64());
    for (int i = 0; verb_exists(scene, verbId) && i < 1000; ++i)
        verbId = kPrefix + std::to_wstring(GetTickCount64()) + L"_" + std::to_wstring(i);

    HKEY hVerb = nullptr;
    if (!open_verb_key(scene, verbId, true, KEY_SET_VALUE, &hVerb))
        return result_json(false, "registry_failed");

    bool ok = write_item_values(scene, hVerb, verbId, label, kind, target, iconSource, position);
    RegCloseKey(hVerb);
    return ok ? result_json_id(true, "ok", verbId) : result_json(false, "registry_failed");
}

std::string edit_custom_item(MenuScene scene, const std::string& id,
                             const std::string& label, const std::string& kind,
                             const std::string& target, const std::string& iconSource,
                             const std::string& position) {
    std::wstring verbId = utf8_to_wide(id);
    if (!is_custom_id(verbId)) return result_json(false, "not_custom");
    if (label.empty() || target.empty()) return result_json(false, "bad_arg");
    if (!verb_exists(scene, verbId)) return result_json(false, "not_found");

    HKEY hVerb = nullptr;
    if (!open_verb_key(scene, verbId, false, KEY_SET_VALUE, &hVerb))
        return result_json(false, "registry_failed");

    bool ok = write_item_values(scene, hVerb, verbId, label, kind, target, iconSource, position);
    RegCloseKey(hVerb);
    return ok ? result_json(true, "ok") : result_json(false, "registry_failed");
}

std::string del_custom_item(MenuScene scene, const std::string& id) {
    std::wstring verbId = utf8_to_wide(id);
    if (!is_custom_id(verbId)) return result_json(false, "not_custom");
    if (!verb_exists(scene, verbId)) return result_json(false, "not_found");

    // 删整个 verb 键（含 command 子键）。RegDeleteTreeW 递归删。
    std::wstring path = L"Software\\Classes\\";
    path += scene_shell_root(scene);
    path += L"\\";
    path += verbId;
    LSTATUS st = RegDeleteTreeW(HKEY_CURRENT_USER, path.c_str());
    return st == ERROR_SUCCESS ? result_json(true, "ok") : result_json(false, "registry_failed");
}

}  // namespace cmm
