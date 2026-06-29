# PROGRESS · 鼠标右键主理人 ContextMenuMaster

> 项目进度与 AI 接管文档。**人看的介绍在 [README.md](./README.md)，本文件只给接手的 AI / 开发者。**
> 中文名「鼠标右键主理人」，英文名 Context Menu Master。最后更新：2026-06-26。

---

## ▍本文档规范（四项目统一，接手前先读）

库根 `开发规范.md` 是跨项目通用约定的单一真相源，**先读它再读本文件**。本文件只放本项目专属内容。

**AI 接管流程**：① 读「技术栈红线」+「踩坑记录」两节（别人用时间换来的，别重踩）→ ② 看「项目结构」定位代码、「已完成/待办」找进度 → ③ 动手前确认「用户偏好」硬约束 → ④ 干完只更新状态行（`[ ]`→`[x]`，新坑补进踩坑记录）。

**写作规范**：任务三态 `[x]`/`[ ]`/`[~]`；不写流水账、不留历史快照日志，完成的一句话带过；踩坑 = 结论+为什么+怎么绕开，三句内；写「文件:行号」别贴大段代码；中文书写。

---

## ▍一句话定位

Windows 11 右键菜单主理工具。可视化改名换图标 + 列表批量管理 + 批量魔法追加后缀，核心卖点「Win11 经典/新版菜单一键互换并瞬间生效」。纯本地、只动 HKCU、可逆。作者 Henglie，MIT。

---

## ▍技术栈红线（已定，对标 FairySave）

- 后端 C++17 / MSVC（VS2022 装在 `D:\Program Files\VS`）。构建 CMake + Ninja，预设见 `CMakePresets.json`（`windows-debug`/`windows-release`/`windows-release-x86`）。
- 界面 Vue3（本地 `ui/vue.global.js`）+ webview 单头库（承载系统自带 WebView2）。皮肤 FairyGlass 液态玻璃，令牌内联进 `ui/index.html`（改令牌须同步回 <https://github.com/Henglie/FairyGlass>）。
- 架构 **Core-Shell**，`src/platform/windows` 是 Win32 特权层。
- **i18n：后端给英文 key/语言标签，前端三语字典翻译，后端绝不返回中文**。新增文案要 zh-CN/zh-TW/en 三处同步补。
- 要 GUI 不要命令行（CLI 仅调试）。目标 Win10/11（经典菜单接管是 Win11 特性）。

---

## ▍项目结构

```
ContextMenuMaster/
  CMakeLists.txt           CMake 主配置（/utf-8 /EHsc + Release 链接瘦身）
  CMakePresets.json        VS「打开文件夹」识别（debug/release/release-x86）
  src/
    main.cpp               CLI 调试入口 CmmCli：lang/status/enable/restore/toggle/scan/probe
    app/gui_main.cpp       GUI 入口 WinMain：webview 窗口 + bind + 语言注入(w.init) + run_capture 调子进程
    platform/windows/
      i18n.{h,cpp}            系统语言检测（判简繁，非中文回退 en）
      context_menu.{h,cpp}   HKCU 空 InprocServer32 接管经典菜单 + 分离子进程重启 explorer
      menu_scanner.{h,cpp}   扫描 HKCR 各场景 verb/handler + 改名/图标/启停/批量后缀 + verb 级联子菜单静态展开
      icon_extract.{h,cpp}   系统图标/资源/.ico 提取为 PNG base64 data URI
      com_menu_probe.{h,cpp} IContextMenu COM 扩展动态菜单探测（实例化 DLL 跑 QueryContextMenu，仅 CmmCli 子进程调）
  third_party/             webview(单头) / webview2(SDK 头) —— 从 FairySave 复用
  ui/index.html            单文件 Vue3（液态玻璃 + 三语 i18n + 双视图 + 批量魔法 + Win11 专区）
  ui/{vue.global.js,liquidGlass.js}
```

**关键 CLSID** `{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}` 是 Win11 新版菜单驱动扩展。在 `HKCU\Software\Classes\CLSID\{...}\InprocServer32` 写空默认值 → 扩展加载失败 → 回落经典菜单。判据见 `context_menu.cpp:is_classic_menu_enabled`（键存在且默认值为空才算我们接管）。

---

## ▍核心架构事实（动手前必懂，别再做错）

**现代菜单（Win11 新菜单）与经典菜单是两套不同来源的项集合，不是同一份数据的两种渲染**（微软官方，已存记忆 `win11-context-menu-architecture`）：

- 现代菜单只枚举：shell 内建命令 + 云提供程序命令 + 满足 `IExplorerCommand` **且有包标识（稀疏包/MSIX）** 的扩展。传统注册表项（`HKCR\*\shell\<verb>`、`shellex\ContextMenuHandlers`）**进不了现代菜单**，一律降级到「显示更多选项」的经典菜单。这就是 WinRAR/Git 在 Win11 默认菜单「消失」的原因。
- **进不去不是权限问题**（提权 System/TrustedInstaller 也没用，shell 根本不读那套注册表）。想进现代菜单只能写包清单 + 稀疏包注册 + `IExplorerCommand` COM 组件。
- 因此本工具可写对象只有**经典菜单层**的传统注册表项，写在 `HKCU\Software\Classes` 覆盖层（不需管理员、只影响当前用户、可逆；因合并视图「HKCU 优先 HKLM」能正确覆盖系统项显示）。

**UI 体现**：可视化视图并排——左=现代菜单（只读示意，只摆 shell 内建项，**绝不渲染扫描项**，标注「此层注册表改不动」）→ 中间箭头 → 右=经典菜单（可编辑，渲染真实扫描项 `this.items`）。`view` 两档 `visual`/`list`。主题切换只作用于预览菜单（`html[data-preview-theme]` 控 `--ctx-*`），UI 外壳玻璃令牌恒暗不参与。

---

## ▍折叠（级联）菜单：两种来源，处理方式不同

1. **verb 级联**（`SubCommands` / `ExtendedSubCommandsKey`）：子项是注册表明文 verb，**静态可读** label/icon/children。WinRAR 压缩组、TortoiseGit、微软「发送到」属此类。已实现静态展开（`menu_scanner.cpp:expand_submenu`）。本机无此类样本，逻辑就位但未真机出实例。
2. **`IContextMenu` COM 扩展**（`shellex\ContextMenuHandlers\<name>` → CLSID → DLL）：菜单形态/子项/文字**全部 DLL 运行时 `QueryContextMenu` 动态生成**，注册表只有 CLSID + DLL 路径，静态读不到。已实现「真实探测」（`com_menu_probe.cpp`，见踩坑 12）——**7-Zip/百度云/SD360 实测全成功**（两级子菜单、中文、分隔线全出），**唯独夸克AI 失败（其自身反集成防护，非框架缺陷）**。

前端 `probeHandlers` 后台串行探测每个有 clsid 的 handler，拿到非空就 `normalizeProbed` 挂成只读子树（`hasSubmenu=true`）。任意深度飞出用全局递归组件 `<ctx-flyout-node>` 渲染。探测项一律只读。

---

## ▍踩坑记录（务必记住，多数继承自 FairySave）

1. **webview bind 返回值在 JS 端已是对象/字符串，不要再 `JSON.parse`**。后端返回 JSON 字符串字面量即可，前端直接用。否则 refresh 中断、界面空白。
2. **MSVC 必须 `/utf-8`**（CMakeLists 已加），否则中文注释按 GBK 误读破坏语法。源码全 UTF-8。
3. **必须 `/EHsc`**：用 std::string/iostream 会抛异常，不开则栈展开不保证 + C4530。
4. **改 ui/ 后必须手动同步 bin**：`cp -r ui/* build/<cfg>/bin/ui/`。POST_BUILD 只在 exe 重链时触发，纯改 HTML 时 ninja "no work to do" → 界面看着没更新。
5. **构建/链接前先 `taskkill //F //IM ContextMenuMaster.exe`**（实例占用锁致 LNK1168）。
6. **bash 跑 bat 编译**：`cmd.exe //c xxx.bat`（双斜杠防 MSYS 转路径）；路径含中文，bat 第二行 `chcp 65001 >nul` 再 cd；bat 必须 CRLF。
7. **不要给 `file://` 加 `?t=` 查询参数**（触发 unique security origin 警告）。
8. **i18n 后端给 key 前端翻译，后端不返中文**。新增消息三语同步补，否则显示原始 key。
9. **CLI 中文输出在 bash→cmd 下 GBK 乱码**：CmmCli 输出一律 ASCII，调试别靠 CLI 看中文（不影响数据正确性）。
10. **接管/恢复重启 explorer 绝不能在 UI 线程同步杀+等（血泪事故）**：bind 回调跑在 WebView2 UI 线程，旧版在此杀 shell 后自己先卡死、再没机会重启 → 任务栏/桌面全没回不来。现行 `restart_explorer` 把「taskkill→timeout→start explorer」整串交给 `DETACHED_PROCESS` 分离 cmd 子进程，本函数投递完立即返回不阻塞 UI；叠加 `AutoRestartShell=1` 双保险。**红线：任何可能关 shell 的路径，都必须由独立于 GUI 的执行体保证把 shell 拉回来。**
11. **注册表 verb label 三档兜底，缺一环就乱码**：MUIVerb / 默认值都可能是 `@dll,-id` 间接资源，必须 `SHLoadIndirectString` 解，且解前预展开 `%systemroot%`（混合大小写会让它直接失败）。默认值漏走 resolve 就会把 `@...dll,-10` 当 label 显示。三档：MUIVerb resolve → 默认值 resolve → humanize 键名（去前导 `.`、驼峰拆词）。handler 的 label 若是 CLSID 去 `HKCR\CLSID\{...}` 读 LocalizedString/默认值。见 `menu_scanner.cpp:resolve_indirect_string`/`read_verb_generic`/`humanize_verb_name`。另：label 落定前过 `clean_label`（trim 含全角空格 U+3000、去零宽 U+200B/U+FEFF、去尾部 ` Class` COM 噪音）；id 保留真实键名不动。Windows 内建项裸键名/英文 COM 描述由前端 `BUILTIN_NAMES` 三语兜底。
12. **真实 COM 动态菜单探测（IContextMenu 扩展），三个关键点**：① `IShellExtInit::Initialize(pidlFolder, pdtobj, hkeyProgID)` 三参都要对——选中对象类（file/folder/drive）的 `pidlFolder` 必须是**父文件夹** pidl（`ILCloneFull`+`ILRemoveLastID`，不是对象自身绝对 pidl），背景类才传文件夹自身；`hkeyProgID` 传场景类键（`HKCR\*`/`Directory` 等）。宽容 handler（7-Zip）传错也忽略，严格的直接 `E_INVALIDARG`。② **加载第三方 DLL 有崩溃/挂死风险，必须在 `CmmCli probe` 子进程跑**（崩了只崩子进程），GUI 用管道按 UTF-8 原始字节读（probe 输出走 `fwrite` 不经 `SetConsoleOutputCP` 的 GBK，否则坏 JSON）。③ 子菜单先发 `WM_INITMENUPOPUP`（`IContextMenu2::HandleMenuMsg`）触发懒填充再遍历 HMENU，顶层空时降级 `CMF_NORMAL` 重试。见 `com_menu_probe.cpp`。**夸克AI 例外**：三参全对仍 `E_INVALIDARG`，是厂商反集成（疑似校验调用方是否真 explorer / 要 `IObjectWithSite` 站点），属夸克自身限制非框架缺陷，已退回前端友好名兜底。**别再为夸克空转**——啃它得逆向 quarkshellext.dll 或起真窗口喂站点，性价比极低。
13. **CSS：`<span>` 开关只显示一颗球的根因**——`.switch` 是 span 默认 `display:inline`，width/height 对 inline 无效，38×22 轨道没撑开，只剩内部 absolute 的 `.knob` 可见。修：`display:inline-block` + `vertical-align:middle`。任何用 span 当定尺寸控件的都要注意。

---

## ▍构建与运行

- VS2022「打开本地文件夹」选本目录 → 读 `CMakePresets.json` → 选预设。命令行（vcvars64 下）：`cmake --preset windows-debug && cmake --build build/debug`。
- 产物：`build/<cfg>/bin/ContextMenuMaster.exe`（GUI）+ `CmmCli.exe`（调试）。
- CLI 调试：`lang`/`status`/`enable`/`restore`/`toggle`/`enable-norefresh`/`scan <scene>`/`probe <scene> <clsid> [真实文件路径]`。

---

## ▍已完成

**后端（C++）**
- [x] i18n 系统语言检测；输出 zh-CN/zh-TW/en 标签。
- [x] Win11 经典菜单接管：HKCU 空 InprocServer32 写/删，状态判定严谨，enable/restore/toggle 幂等返回 `MenuResult` 四态。
- [x] 分离子进程优雅重启 Explorer（修复旧版卡死事故，根因见踩坑 10）。
- [x] 真实菜单扫描 `menu_scanner`：6 场景 verb（明文，可改名/图标/启停）+ handler（COM 扩展），label 三档兜底解析（踩坑 11），verb 级联子菜单静态展开。写回走 HKCU 覆盖层，写前自动 `reg.exe export` 备份到 `%LOCALAPPDATA%\ContextMenuMaster\backups\`。SYS 标记按 InprocServer32 DLL 是否在 `%SystemRoot%` 下精准判定（不再误标第三方）。
- [x] COM 扩展真实菜单探测 `com_menu_probe`（核心功能，见踩坑 12）：子进程隔离跑 `QueryContextMenu`，递归 dump 任意深度菜单树。7-Zip/百度云/SD360 实测成功。
- [x] icon_extract：系统图标/资源/.ico → PNG base64 data URI。
- [x] GUI 入口 + bind 全套；CmmCli 调试探针（含 scan/probe）；CMake 工程（core 静态库 + GUI + CLI，/utf-8 /EHsc + Release 瘦身）。

**前端 / UI（液态玻璃）**
- [x] 单文件 Vue3 + 内联 FairyGlass 令牌 + 三语 i18n（首启读 `window.__CMM_LANG__`，可手动切）。
- [x] 6 场景选择器（tab 角标显示项数），切换即 `scan_scene` 拉真数据。
- [x] 双视图：① 可视化（现代菜单只读示意 + 经典菜单可编辑并排，见架构事实）② 列表（表格 + 复选框批量 + 改名/图标按钮 + 启停开关 + 批量魔法面板）。Win11 菜单本体高保真：max-content 自适应宽 + min-width、圆角 8、Segoe UI Variable、Mica blur(30px)、行高 32。
- [x] 折叠菜单飞出：verb 级联静态展开 + COM 探测动态子菜单，统一用递归组件 `<ctx-flyout-node>` 渲染任意深度。
- [x] 助记符处理：`stripAccel`（Win11/列表剥 `(&X)`）/`renderAccel`（经典视图 `&X`→`<u>X</u>`）。
- [x] 真实图标接入（`extract_icon` data URI，失败回落 ■/▸ 几何字符，绝不 emoji）。
- [x] 主题预览三态（跟随系统/浅色/深色，只作用于预览菜单，存 localStorage）。
- [x] CRUD 接线 + 系统项二次确认弹窗 + 写操作「已自动备份」toast。
- [x] Win11 专区：经典/新版菜单状态 + 一键互换。
- [x] Windows 内建项三语友好名兜底 `BUILTIN_NAMES`（verb 按 id、handler 按清洗后 label，含夸克AI）。
- [x] 列表操作按钮统一 FairyGlass 玻璃风（玻璃填充+描边+圆角+图标，hover 提蓝边、active 微缩）。
- [x] 启停开关关态可辨（深底+明描边+inset，修了只显示一颗球的 bug，见踩坑 13）；主题/语言弹层叠 86% 深底防透明看不清。

---

## ▍待办

- [ ] **verb 级联子菜单真机验证**：本机无 WinRAR 压缩组/TortoiseGit 这类样本，静态展开逻辑就位但没出过真实实例，装一个验证飞出渲染。
- [ ] **列表视图折叠父项**：暂未对折叠/COM 父项做子项展开或标注（待定要不要加）。
- [ ] Win11 IExplorerCommand 稀疏包(MSIX)扫描 + 写入（README 已声明开发中）。这是独立大功能：要 `IExplorerCommand` COM 组件 + 稀疏包打包注册，不是改注册表能成，与权限无关。
- [ ] 编译 x64/x86 Release + 0 警告 + 打包。
- [ ] git 初始化（Henglie 身份，零 AI 痕迹）。

---

## ▍用户偏好（硬约束，四项目通用）

- 作者恒烈 / Henglie。熟 Windows 桌面 + GDI+，**不熟 Rust、初学 CMake**。中文交流。
- **反感 emoji**：回复和界面都克制，导航/图标用黑白几何符号。
- git 提交用 `Henglie <ebhenglie@gmail.com>`，**绝不出现 Claude 署名 / Co-Authored-By / AI 痕迹**。
- 要 GUI 不要命令行。讲清「为什么」，给推荐而非罗列；能定的直接做，不确定才问。
- **删除文件走回收站**，不用 rm 直接抹除。
