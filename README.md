# 鼠标右键主理人 Context Menu Master

**简体中文** | 繁體中文 | English

> 在 Windows 11 上重新主理你的鼠标右键菜单。

**鼠标右键主理人**是一款 Windows 桌面效能工具。它把杂乱、被折叠、被各种软件塞满的右键菜单交还到你手里：可视化地改名换图标、批量勾选管理、一键找回 Windows 11 被藏起来的经典菜单。

纯本地运行，只改当前用户注册表（HKCU），无需管理员权限，所有操作随时可逆。

---

## 它解决什么问题

| 场景 | 没有本工具 | 有了本工具 |
|---|---|---|
| Win11 右键菜单太精简，常用项被折进「显示更多选项」 | 每次多点一下、多等一下 | 一键切回完整经典菜单，瞬间生效 |
| 装的软件往右键菜单塞了一堆项，又乱又长 | 只能去注册表里手动翻 | 列表视图批量勾选，一眼管理 |
| 想给菜单项改个顺眼的名字或图标 | 得懂注册表结构才敢动 | 可视化视图里点一下就改 |
| 想统一给一批菜单项加个标记 | 一个个手动改 | 批量魔法，勾选后一键追加后缀 |

---

## 核心功能

- **双视图实时同步**
  - **可视化视图（WYSIWYG）**：1:1 模拟 Windows 真实右键菜单，点名称改名、点图标换图标，所见即所得。
  - **列表视图（List View）**：传统表格，复选框批量勾选，适合一次管理一批菜单项。
- **批量魔法**：勾选若干菜单项，一键给它们全部追加同一个后缀（如「 喵~」「 ★」），也可自定义。
- **Windows 11 专区**
  - **经典菜单一键互换**：在 HKCU 写入空 `InprocServer32` 让新版菜单扩展失效，资源管理器回落到 Win10 经典菜单；并优雅重启 `explorer.exe`，菜单瞬间生效，无需注销重登。再点一下即可恢复。
  - **新版菜单扩展（开发中）**：预留扫描与接管 Win11 `IExplorerCommand` 稀疏包（MSIX）扩展项的接口。
- **多语言**：首启按系统显示语言自动选（中文简/繁，其余一律英文），界面可随时切换。
- **纯本地、可逆**：只读写当前用户注册表，不联网、不上传，删键即恢复系统默认。

---

## 多语言

由 C++ 后端用 Win32 API 读取系统默认显示语言，在 WebView2 页面创建前注入给前端：

- 系统是中文（zh-CN / zh-TW / zh-HK 等）→ 按简繁渲染中文界面。
- 系统是其他任何语言 → 一律回退英文（en）。

界面右上角可随时手动切换简体 / 繁体 / English，选择记入本地存储。

---

## 技术栈与架构

| 层 | 选型 | 说明 |
|---|---|---|
| 后端引擎 | 现代 C++17 / MSVC | Win32 API 母语直调 |
| 展现层 | Microsoft Edge WebView2 | webview 单头库承载系统自带 WebView2 |
| 通信桥梁 | WebMessage / postMessage | `w.bind` 暴露后端函数，前端 `await window.fn()` 调用 |
| 界面 | Vue 3（本地） | 单文件应用 |
| 视觉系统 | FairyGlass | 微蓝暗色液态玻璃设计系统，令牌内联复刻进 `ui/index.html` |
| 构建 | CMake + Ninja | VS2022「打开文件夹」即识别 |

**架构：Core-Shell（通用内核 + 原生外壳）**，与姊妹项目 FairySave 一致：`src/platform/windows` 是 Win32 特权层（i18n 检测、注册表菜单接管），`ui/` 是表现层。

**兼容性**：Windows 10 / 11（经典菜单接管为 Win11 特性）。需 WebView2 运行时（Win10/11 自带）。

---

## 开发与构建

**环境**：VS2022（含 C++ 桌面负载、CMake、Ninja、Windows SDK）+ WebView2 运行时。

**最简方式**：VS2022「打开本地文件夹」选中本目录，自动读 `CMakePresets.json` 识别为 CMake 工程，选 `windows-debug` 或 `windows-release` 预设生成、运行。

**命令行方式**（vcvars64 环境下）：

```bat
cmake --preset windows-release
cmake --build build/release
```

产物：

- `build/release/bin/ContextMenuMaster.exe` —— GUI 主程序（WIN32 子系统，无控制台）。
- `build/release/bin/CmmCli.exe` —— 命令行调试探针（验证后端用，非最终用户面向）。

> 改完 `ui/` 后若只动了 HTML，需手动把 `ui/` 同步到 exe 旁的 `ui/` 目录（CMake 的 POST_BUILD 仅在重新链接时触发）。

---

## 目录结构

```
ContextMenuMaster/
├── CMakeLists.txt              CMake 主配置（≈ .sln，含 /utf-8 /EHsc + Release 瘦身）
├── CMakePresets.json           VS 一键识别（windows-debug / release / release-x86）
├── src/
│   ├── main.cpp                CLI 调试入口（CmmCli）
│   ├── app/gui_main.cpp        GUI 入口 WinMain：webview 窗口 + 绑定 + 语言注入
│   └── platform/windows/
│       ├── i18n.{h,cpp}        系统语言检测（zh-CN / zh-TW / en）
│       └── context_menu.{h,cpp} Win11 经典↔新版菜单接管 + 优雅重启 Explorer
├── third_party/                webview 单头库 + WebView2 SDK
└── ui/index.html               单文件 Vue3 应用（液态玻璃皮肤 + 三语 i18n）
```

---

## 隐私与安全

- **纯本地**：不联网、不上传任何内容。
- **只动 HKCU**：所有注册表操作限于当前用户，无需管理员权限。
- **可逆**：经典菜单接管随时删键恢复系统默认；不破坏系统文件。

---

## 许可与作者

- **作者**：恒烈 (EternalBlaze / Henglie)
- **协议**：MIT
