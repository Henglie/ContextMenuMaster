// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— 富媒体 Shell 扩展 DLL（阶段 2 地基）
//
//  这是「生产端」：我们自己写一个进程内 COM DLL 实现 IContextMenu + IShellExtInit，
//  注册到指定文件类型的 HKCR\<类型>\shellex\ContextMenuHandlers\<name>。explorer
//  在用户右键该类型文件时，会 CoCreateInstance 我们的 CLSID、喂上下文、调
//  QueryContextMenu 让我们现场往菜单里塞项。这是 Bandizip「右键压缩包看内容 /
//  预览缩略图」的机制本体。
//
//  ⚠ 与 com_menu_probe.{h,cpp} 完全相反：那个是「读别人的 COM 扩展」（消费端），
//  本模块是「造自己的 COM 扩展」（生产端），代码零复用。
//
//  阶段 2 分三步递进，本文件是 step 1 地基：
//    step 1（本批）：DLL 能被 explorer 加载，右键出一个静态项，点击弹框响应。
//    step 2：owner-draw 自绘（WM_MEASUREITEM/WM_DRAWITEM 画缩略图）。
//    step 3：动态子菜单（读压缩包/文件夹现场列内容）。
//
//  This is the *producer* side: our own in-proc COM DLL implementing IContextMenu
//  + IShellExtInit, registered under a file type's ContextMenuHandlers. Opposite
//  of com_menu_probe (the *consumer* side). Step 1 skeleton.
// ============================================================
#pragma once

#include <windows.h>
#include <shlobj.h>
#include <objbase.h>

// ------------------------------------------------------------
//  本扩展的 CLSID —— 全局唯一，注册表与 DllGetClassObject 都认它。
//  {A7E4C9F1-3D82-4B6E-9C15-2F8A6D5E1B93}
//  换 CLSID 必须同步改注册脚本；生产前保持稳定，别随手重排。
// ------------------------------------------------------------
// DEFINE_GUID 在头里只会生成 extern 声明（不分配实体），实体在 shellext.cpp
// 里显式给值。故此处仅声明，避免 LNK2001「无法解析的外部符号」。
EXTERN_C const CLSID CLSID_CmmShellExt;

// 注册时写在 ContextMenuHandlers 下的子键名（人类可读，仅标识用）。
#define CMM_SHELLEXT_HANDLER_NAME  L"ContextMenuMasterRichMedia"

// COM 服务器友好名（写进 HKCR\CLSID\<clsid> 默认值）。
#define CMM_SHELLEXT_FRIENDLY_NAME L"ContextMenuMaster Rich-Media Shell Extension"

// 进程内活动对象计数（DllCanUnloadNow 判活）。定义在 shellext.cpp。
extern long g_cmmDllRefCount;

// DLL 自身模块句柄（DllMain 里存，注册时取自身路径用）。
extern HINSTANCE g_cmmDllInstance;
