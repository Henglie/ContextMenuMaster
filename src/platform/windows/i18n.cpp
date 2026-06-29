// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— i18n 系统语言检测（实现）
// ============================================================
#include "platform/windows/i18n.h"

#include <windows.h>

namespace cmm {

// 取系统 UI 语言并细分简繁。
//
// 为什么用 GetUserDefaultUILanguage 而不是 GetUserDefaultLCID：
//   前者拿的是「Windows 显示语言」（用户在设置里选的界面语言），正是我们
//   想跟随的对象；后者是「区域格式」（管日期/数字），用户常把界面留英文却
//   把区域设成中国，两者会打架。跟界面走才符合直觉。
//
// 简繁判定靠 SUBLANGID：
//   zh-CN(简) / zh-SG(简) -> ZhHans
//   zh-TW / zh-HK / zh-MO(繁) -> ZhHant
//   其余 Chinese 子语言保守按简体处理。
//
// Use GetUserDefaultUILanguage (the Windows *display* language the user picked),
// not the regional LCID — we want to follow the UI, not the date/number region.
// Simplified vs Traditional is decided by SUBLANGID.
Lang detect_ui_lang() {
    LANGID lid = GetUserDefaultUILanguage();
    WORD primary = PRIMARYLANGID(lid);

    if (primary != LANG_CHINESE) {
        // 非中文：英文兜底（这是产品既定回退规则）
        // Non-Chinese: fall back to English (the product's fixed rule).
        return Lang::En;
    }

    WORD sub = SUBLANGID(lid);
    switch (sub) {
        case SUBLANG_CHINESE_TRADITIONAL:  // zh-TW
        case SUBLANG_CHINESE_HONGKONG:     // zh-HK
        case SUBLANG_CHINESE_MACAU:        // zh-MO
            return Lang::ZhHant;
        case SUBLANG_CHINESE_SIMPLIFIED:   // zh-CN
        case SUBLANG_CHINESE_SINGAPORE:    // zh-SG
        default:
            // 其余中文子语言（含未知）保守按简体
            // Other/unknown Chinese sub-langs default to Simplified.
            return Lang::ZhHans;
    }
}

const char* lang_tag(Lang lang) {
    switch (lang) {
        case Lang::ZhHans: return "zh-CN";
        case Lang::ZhHant: return "zh-TW";
        case Lang::En:
        default:           return "en";
    }
}

std::string detect_ui_lang_tag() {
    return lang_tag(detect_ui_lang());
}

}  // namespace cmm
