// ============================================================
//  鼠标右键主理人 ContextMenuMaster —— IContextMenu COM 扩展动态菜单探测
//
//  问题：像夸克AI 这种右键项不是注册表里的明文 verb，而是 `IContextMenu` COM
//  扩展（注册在 *\shellex\ContextMenuHandlers，只留一个 CLSID + DLL 路径）。
//  它显示的折叠形态、子项、本地化文字全部由 DLL 在运行时 `QueryContextMenu`
//  动态生成 —— 纯注册表静态扫描读不到，只能看到 CLSID 默认值那个英文名。
//
//  本模块把这类扩展的真实菜单「跑」出来：构造右键对象的 IDataObject，
//  CoCreateInstance 实例化 handler，IShellExtInit::Initialize 喂上下文，
//  IContextMenu::QueryContextMenu 填充一个内存 HMENU，再递归遍历 HMENU
//  （子菜单发 WM_INITMENUPOPUP 触发懒填充）dump 成菜单树。
//
//  ⚠ 风险：这要把第三方 shell 扩展 DLL 加载进本进程并执行其代码，可能崩溃 /
//  挂起 / 弹窗。因此 GUI **不直接**调本模块，而是通过 CmmCli 子进程（CmmCli
//  probe <scene> <clsid>）调用，崩了只崩子进程、不连累主界面。本模块本身只负责
//  把探测结果序列化成 JSON。探测得到的项一律「只读」（纯还原，不参与写回）。
//
//  Probe the dynamic menu of an IContextMenu COM handler (e.g. Quark AI) by
//  actually instantiating it and walking the HMENU it builds. Runs inside the
//  CmmCli subprocess for crash isolation. Results are read-only.
// ============================================================
#pragma once
#include <string>

#include "platform/windows/menu_scanner.h"  // MenuScene

namespace cmm {

// 探测某个 IContextMenu COM handler 的真实动态菜单。
//   scene  决定右键对象（file=临时文件 / folder=临时目录 / drive=C:\ /
//          desktop|folder_bg=目录背景，pdtobj 为空）。
//   clsid  handler 的 CLSID，形如 "{99D4E39A-EA7F-453E-9488-58C56BBF7B98}"。
// 返回 JSON：{"ok":true,"items":[ {"label","shortcut","separator","disabled",
//            "hasSubmenu","children":[...]}, ... ]}；失败 {"ok":false,"key":"..."}。
// 前端拿到即对象，勿再 JSON.parse（项目踩坑 1）。
//   overridePath（可选，UTF-8）：非空时用该真实文件/目录当右键对象，替代临时
//     scratch。某些 handler（如夸克AI）挑真实文件类型才填项，诊断时用它指真实样本。
std::string probe_handler_menu_json(MenuScene scene, const std::string& clsid,
                                    const std::string& overridePath = "");

}  // namespace cmm
