// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— i18n 系统语言检测
//
//  C++ 侧只做一件事：问操作系统「当前显示语言是什么」，归一化成
//  前端认识的三档语言标签之一，交给 WebView2 在页面创建前注入。
//  判定规则（对标既有项目）：中文（简/繁）按子语言细分，其余一律回退英文。
//
//  This module asks the OS for its UI language and normalizes it into one of
//  the three tags the frontend understands. Chinese is split into Simplified /
//  Traditional by sub-language; everything else falls back to English.
// ============================================================
#pragma once
#include <string>

namespace cmm {

// 前端认识的三档语言标签。非中文系统一律 En（英文兜底）。
// The three language tags the frontend knows. Non-Chinese systems fall back to En.
enum class Lang {
    ZhHans,   // 简体中文 zh-CN
    ZhHant,   // 繁体中文 zh-TW / zh-HK / zh-MO
    En        // 英文（默认回退）/ English (default fallback)
};

// 调用 Win32 API 读系统默认显示语言，归一化为 Lang。
// Reads the system default UI language via Win32 and normalizes it.
Lang detect_ui_lang();

// 转成前端用的 BCP-47 风格标签字符串："zh-CN" / "zh-TW" / "en"。
// To the BCP-47-ish tag string the frontend consumes.
const char* lang_tag(Lang lang);

// 便捷封装：直接得到标签字符串（detect_ui_lang + lang_tag）。
// Convenience: detect + stringify in one call.
std::string detect_ui_lang_tag();

}  // namespace cmm
