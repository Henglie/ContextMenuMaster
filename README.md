# 鼠标右键主理人 Context Menu Master

**简体中文** | 繁體中文 | English

> 在 Windows 11 上重新主理你的鼠标右键菜单。

**鼠标右键主理人**是一款 Windows 桌面效能工具。它把杂乱、被折叠、被各种软件塞满的右键菜单交还到你手里：可视化地改名换图标、批量勾选管理、一键找回 Windows 11 被藏起来的经典菜单，并覆盖桌面 / 文件 / 文件夹 / 驱动器 / 回收站 / Win+X / 发送到 / 新建 / 拖放等全部右键场景。

纯本地运行，写操作前自动备份注册表，所有操作随时可逆。以管理员身份运行以便管理系统与所有用户（HKLM）范围的菜单项。

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

- **全右键场景覆盖**：一个侧栏纵向平铺所有右键场景，点一下即切换——
  - **注册表场景**：桌面空白、文件、文件夹、文件夹空白、驱动器、回收站、所有文件对象、此电脑、库。
  - **Win+X 菜单**：管理「右键开始按钮 / Win+X」那个高级菜单（分组显示、启停、新增自定义项、删除）。
  - **发送到（SendTo）**：管理右键「发送到」子菜单的项，可启停、新增、删除。
  - **新建（ShellNew）**：管理右键「新建」子菜单里出现哪些文件类型，逐项启停。
  - **拖放处理程序（DragDrop）**：管理右键拖放文件时弹出的菜单扩展。
  - **打开方式（Open with）**：查看注册为「打开方式」候选的应用（只读）。
- **双视图实时同步**
  - **可视化视图（WYSIWYG）**：1:1 模拟 Windows 真实右键菜单，点名称改名、点图标换图标，**右键单击菜单项弹出操作菜单（改名 / 换图标 / 启用停用）**，所见即所得。
  - **列表视图（List View）**：传统表格，复选框批量勾选，启停开关、改名换图标按钮一应俱全，适合一次管理一批菜单项。
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
├── CMakePresets.json           VS 一键识别（windows-debug / release / release-x86 / release-arm64）
├── src/
│   ├── main.cpp                CLI 调试入口（CmmCli）
│   ├── app/
│   │   ├── gui_main.cpp        GUI 入口 WinMain：webview 窗口 + bind + 语言注入
│   │   └── app.manifest        UAC requireAdministrator 提权清单
│   ├── platform/windows/       Win32 特权层（每个右键场景一个模块）
│   │   ├── i18n.{h,cpp}          系统语言检测（zh-CN / zh-TW / en）
│   │   ├── context_menu.{h,cpp}  Win11 经典↔新版菜单接管 + 优雅重启 Explorer
│   │   ├── menu_scanner.{h,cpp}  注册表场景扫描 + 改名/图标/启停/批量后缀
│   │   ├── icon_extract.{h,cpp}  系统图标/资源/.ico → PNG base64 data URI
│   │   ├── image_icon.{h,cpp}    任意图片 → 多尺寸 PNG-in-ICO（富媒体阶段 1）
│   │   ├── com_menu_probe.{h,cpp} IContextMenu COM 扩展动态菜单探测
│   │   ├── win11menu.{h,cpp}     Win11 现代菜单（PackagedCom / SystemCommandStore）
│   │   ├── winx_menu.{h,cpp}     Win+X 菜单（文件系统 .lnk + hash）
│   │   ├── sendto_menu.{h,cpp}   发送到子菜单
│   │   ├── shellnew_menu.{h,cpp} 新建子菜单
│   │   ├── dragdrop_menu.{h,cpp} 拖放处理程序
│   │   ├── openwith_menu.{h,cpp} 打开方式
│   │   ├── custom_menu.{h,cpp}   自建右键菜单项
│   │   └── filetype_menu.{h,cpp} 按扩展名管理文件类型右键项
│   └── shellext/               富媒体 Shell 扩展 DLL（阶段 2，独立不链 cmm_core）
│       ├── shellext.cpp          IContextMenu 进程内 COM 服务器 + owner-draw 缩略图
│       └── shellext.def          COM 四导出
├── third_party/                webview 单头库 + WebView2 SDK
└── ui/
    ├── index.html              单文件 Vue3 应用（液态玻璃皮肤 + 三语 i18n）
    ├── vue.global.js           Vue 3 运行时（本地）
    └── liquidGlass.js          液态玻璃辅助
```

---

## 隐私与安全

- **纯本地**：不联网、不上传任何内容。
- **需管理员权限**：第三方软件的菜单项常注册在 `HKLM`（所有用户），其停用标记也多在 `HKLM`，纯用户态改不动。故程序启动时请求管理员权限，直接读写菜单项真实所在的注册表位置，以支持「所有项都能启停 / 改名」。
- **写前自动备份**：每次改注册表前先 `reg export` 备份对应键到本地 `%LOCALAPPDATA%\ContextMenuMaster\backups\`。
- **可逆**：经典菜单接管随时删键恢复；启停走「加删标记 / 搬到停用镜像」，可原样搬回；删除项一律送回收站。不破坏系统文件。

---

## 下载

- 预编译版见 [Releases](https://github.com/Henglie/ContextMenuMaster/releases)：按 CPU 架构选 zip（x64 / x86 / arm64），解压后双击 `ContextMenuMaster.exe` 即用，无需安装。绝大多数电脑选 x64。
- 需 WebView2 运行时（Windows 10/11 自带）。

---

## 许可与作者

- **作者**：恒烈 (EternalBlaze / Henglie)
- **版本**：v0.5
- **项目主页**：<https://github.com/Henglie/ContextMenuMaster>
- **协议**：MIT
