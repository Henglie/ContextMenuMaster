# PROGRESS · 鼠标右键主理人 ContextMenuMaster

> 项目进度与 AI 接管文档。**人看的介绍在 [README.md](./README.md)，本文件只给接手的 AI / 开发者。**
> 中文名「鼠标右键主理人」，英文名 Context Menu Master。最后更新：2026-07-04（v0.2beta）。

---

## ▍本文档规范（四项目统一，接手前先读）

库根 `开发规范.md` 是跨项目通用约定的单一真相源，**先读它再读本文件**。本文件只放本项目专属内容。

**AI 接管流程**：① 读「技术栈红线」+「踩坑记录」两节（别人用时间换来的，别重踩）→ ② 看「项目结构」定位代码、「已完成/待办」找进度 → ③ 动手前确认「用户偏好」硬约束 → ④ 干完只更新状态行（`[ ]`→`[x]`，新坑补进踩坑记录）。

**写作规范**：任务三态 `[x]`/`[ ]`/`[~]`；不写流水账、不留历史快照日志，完成的一句话带过；踩坑 = 结论+为什么+怎么绕开，三句内；写「文件:行号」别贴大段代码；中文书写。

---

## ▍一句话定位

Windows 11 右键菜单主理工具。可视化改名换图标 + 列表批量管理 + 批量魔法追加后缀，核心卖点「Win11 经典/新版菜单一键互换并瞬间生效」。提权后写真实 HKLM/HKCU（写前自动备份），纯本地、可逆。作者 Henglie，MIT。

**双重定位（纲领，别再丢）**：本项目不只是「系统工具（右键菜单管理器）」，更是「**美化工具**」。两大差异化特色，超出参考项目 ContextMenuMgr：
1. **所见即所得的可视化编辑**——1:1 仿真右键菜单，直接在菜单对象上改名/换图标/启停/删/新建，非纯表单。
2. **富媒体 + 高度自定义右键菜单**——终极愿景：让用户给右键菜单插图片/视频/动图、套皮肤、自定义布局样式。标杆是 Nilesoft Shell（自绘菜单）而非 ContextMenuMgr（纯管理器）。详见「▍富媒体愿景路线图」。

---

## ▍技术栈红线（已定，对标 FairySave）

- 后端 C++17 / MSVC（VS2022 装在 `D:\Program Files\VS`）。构建 CMake + Ninja，预设见 `CMakePresets.json`（`windows-debug`/`windows-release`/`windows-release-x86`）。
- 界面 Vue3（本地 `ui/vue.global.js`）+ webview 单头库（承载系统自带 WebView2）。皮肤 FairyGlass 液态玻璃，令牌内联进 `ui/index.html`（改令牌须同步回 <https://github.com/Henglie/FairyGlass>）。
- 架构 **Core-Shell**，`src/platform/windows` 是 Win32 特权层。
- **i18n：后端给英文 key/语言标签，前端三语字典翻译，后端绝不返回中文**。新增文案要 zh-CN/zh-TW/en 三处同步补。
- 要 GUI 不要命令行（CLI 仅调试）。目标 Win10/11（经典菜单接管是 Win11 特性）。
- **提权模型（已改）**：GUI 启动即 `requireAdministrator`（`src/app/app.manifest`，链接器 `/MANIFESTUAC:NO` 让它独占，见踩坑 14）。经典菜单项分散在 `HKLM\SOFTWARE\Classes`（所有用户，第三方软件多写这）+ `HKCU\Software\Classes`；提权后直接写项**真实所在的 hive**，不再走旧的「纯 HKCU 覆盖层」。这是支持「所有项都能开关/改名」的前提（作者本就打算管理员运行）。

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
14. **UAC 提权 manifest 与链接器冲突（c1010001）**：CMake/link 默认给 GUI 生成 `asInvoker` 的 UAC manifest 片段，与自带 `app.manifest` 的 `requireAdministrator` 合并时 mt.exe 报 `level not equal`。修：CMake 给 GUI target 加 `/MANIFESTUAC:NO`，让链接器不生成默认片段，`app.manifest` 完全接管（requireAdministrator + DPI PerMonitorV2 + activeCodePage UTF-8）。验证：`(Get-Item exe).VersionInfo` 不行，得从 exe 资源提 RT_MANIFEST 看 level 是否 requireAdministrator。
15. **「怎么点都开不了」根因 = 停用标记在 HKLM，纯 HKCU 删不掉**：123云盘/ToDesk/迅雷等第三方 verb 的 `LegacyDisable` 写在 `HKLM\SOFTWARE\Classes`，旧版纯 HKCU 覆盖层只删得掉 HKCU 那份，HKLM 那份还在 → 合并视图仍禁用。修：提权后启用时**两个 hive 的 LegacyDisable 都删**；停用写项真实所在的主 hive（`primary_write_hive`：HKLM 有就写 HKLM）。见 `menu_scanner.cpp:set_item_enabled`/`locate_verb`/`open_verb_in_hive`。
16. **改名不生效 = MUIVerb 优先于默认值**：verb 若有 `MUIVerb`，Windows 显示时它优先于默认值，只改默认值白改。修：`set_verb_default` 写默认值同时 `RegDeleteValueW(MUIVerb)`，让默认值成唯一显示源。
17. **handler 启停 = disabled mirror 搬家**：COM handler 停用不是改值，是把子键从 `shellex\ContextMenuHandlers\<name>` 整树搬到同级 `-ContextMenuHandlers\<name>`（`-` 前缀 shell 不枚举）。故 scan 必须**同时扫两个根**，停用镜像里的标 `enabled=false`，否则一停用就从列表消失再也开不回来。见 `menu_scanner.cpp:move_handler`/`copy_key_tree`/`scan_scene` 的双根扫描。
18. **WebView2 bind 回调同步阻塞 UI 线程（卡顿根因）**：webview 单头库的 `sync bind` 在 WebView2 UI 线程里同步执行 `fn(req)`（见库 line 2498）。probe 每个起子进程冷启 COM ~250ms，file 场景 14 个 handler 串行探测 ≈3.5s 全程冻结界面。修：probe/extract_icon 改**异步 3 参 bind**，后台 `std::thread` 跑完再 `w.resolve(seq,...)`，UI 线程投递即返回；前端配合 `Promise.all` 并发 + 按 clsid 缓存（切场景不重探）。见 `gui_main.cpp` 异步 bind、`index.html:probeHandlers`。
19. **主题/语言弹层被下方内容盖住**：`.header` 无层叠上下文，popover 的 `z-index:30` 只在 header 内有意义；content 里的 `.glass` 卡片带 `backdrop-filter` 会各自新建 stacking context，DOM 靠后先天压过 header 弹层。修：`.header` 加 `position:relative; z-index:20`，让整个 header（含弹层）作为一个高层栈压过 content。凡 `backdrop-filter` + 绝对定位弹层都要留意这类跨容器层叠。
20. **WebView2 内存无法到参考项目的 50MB（架构差异，非 bug）**：参考项目 ContextMenuMgr 是纯 WPF（无 Chromium），本项目是 WebView2（内嵌 Edge/Chromium），内存下限受框架决定。优化手段：① `debug=false` 关 DevTools（实测从 685MB → ~260MB，**这是最大头**）；② `WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS` 注入 `--disable-extensions --disable-features=...` 等安全参数。单实例干净测 ~337MB / 6 子进程。**别用 `--single-process`/`--disable-gpu`**：官方不支持，复杂交互会白屏/崩溃，之前试过已回退。要再降只能换 UI 框架（重写）。测内存务必先 `taskkill //F //IM ContextMenuMaster.exe` 清残留实例，否则多实例叠加数字虚高。
19. **内存优化只能到 ~337MB，到不了参考项目 50MB（架构差异，非 bug）**：本项目 WebView2 = 内嵌 Chromium，参考项目 ContextMenuMgr 是纯 WPF（无 Chromium）才 50MB。降内存两招：① `webview(false,...)` 关 DevTools（debug=true 会多拉进程 + 放宽优化，实测 685MB→260MB）；② `WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS` 注入 `--disable-extensions --disable-features=...` 等**安全**参数。**红线：别用 `--single-process`/`--disable-gpu`**——WebView2 官方不支持，复杂交互下白屏/崩溃，交接不留雷。单实例实测 6 子进程 337MB，稳定不崩。见 `gui_main.cpp:WinMain` 开头。
20. **弹层被下方内容覆盖（深浅/语言菜单）**：`.header` 无独立 stacking context 时，popover 的 z-index 只在 header 内有效，而 `.content` 里的 `.glass` 卡片各自 `backdrop-filter` 建栈、DOM 靠后绘制盖住弹层。修：给 `.header` 建高层级独立栈（`position:relative; z-index`）。见 `index.html` .header 样式。
21. **提权写路径三处「假失败/停用不生效」隐患（已真机验证闭环）**：① handler 启停 `move_handler` 原用 `||` 短路，HKLM 搬成功就跳过 HKCU；handler 同时存在两 hive 时停用只搬一份→合并视图仍启用。修：两 hive 都尝试搬，任一成功即成。② verb 停用只写主 hive，若 HKCU 覆盖层有同名无 `LegacyDisable` 项→HKCU 优先→看着没停用（与启用「两 hive 都删」不对称）。修：停用也两 hive 对称写（仅对已存在的 verb 写，不建空壳）。③ `do_backup` 硬超时 5s，`*`(file) 场景导出 `HKCR\*` 整树易超时→拿到 259 STILL_ACTIVE≠0→误报 `backup_failed` 挡写。修：超时提到 30s。见 `menu_scanner.cpp:set_item_enabled`/`move_handler`/`do_backup`。**验证**：`CmmCli set-enabled` 真机跑通——123pan（HKLM+HKCU 双 LegacyDisable 样本）对称删/写、SD360 handler 停用/启用往返 CLSID 不丢、备份每次 ok（30s 修复隐式过），幂等不报假失败。
22. **`handler_at_target` 匿名 namespace 内重复定义两次（C2084）**：踩坑 21 那批「静态预审」代码把同签名函数写了两遍 → 编译直接 fail，即从没编译过。修：删重复定义留一份。**教训**：静态预审的代码务必先编译过再声称「就位」，否则连语法都不保。
23. **Win+X 菜单是文件系统不是注册表**：项在 `%LOCALAPPDATA%\Microsoft\Windows\WinX\Group1~3\*.lnk`，启停=改扩展名 `.lnk`↔`.lnk.disabled`，菜单分组倒序（Group3 在顶）。新增自定义项必须给 lnk 写合法 `System.Winx.Hash`（GUID `{FB8D2D7B-...}` PID 2），否则 explorer 忽略不显示。hash 算法：`HashData`(shlwapi) 对 `(泛化路径+参数+"do not prehash links.  this should only be done by the user.")` 全小写的 UTF-16 字节算，取前 4 字节 UInt32。两个非显然点：① 三个 PropertyKey **硬编码**别用 `PSGetPropertyKeyFromName`（Win10 不可靠）；② 魔法盐串里 `links.` 后是**两个空格**，错一个字节 hash 就废。泛化路径映射 %ProgramFiles%/System32/Windows→特定 GUID。见 `winx_menu.cpp:write_winx_hash`。
24. **多窗口并发写丢改动（T8 的 CLI 分支被覆盖回退）**：多 Agent 各开独立窗口改同一批文件时，两个 Agent 前后脚改 `main.cpp`（一个加 winx CLI 分支、另一个改别处后整文件回写），后写者用的是不含前者改动的旧快照 → 前者的 winx list/toggle 分支被静默抹掉。表现：`gui_main.cpp` 的 winx bind 在（另一文件没被覆盖）、`winx_menu.cpp` 在，唯独 CLI 入口的 winx 分支消失，`CmmCli winx` 报 `unknown command`。**根因**：Agent 回执自报「已加 XXX」不等于磁盘现状，跨窗口后写可能覆盖。**教训**：① 关键交付以真跑/grep 磁盘为准，别信回执自述（同踩坑 22）；② 独立执行体交叉验证能逮住单 Agent 自查不到的回退（本例是专派的只读验证 Agent 揪出 CLI 缺失）；③ 改同一文件的任务尽量串行或物理隔离到不同文件。修：M 重新补 `main.cpp` winx 分支，真跑 `CmmCli winx list` 验证枚举 17 项、组倒序正确。

---

## ▍构建与运行

- VS2022「打开本地文件夹」选本目录 → 读 `CMakePresets.json` → 选预设。命令行（vcvars64 下）：`cmake --preset windows-debug && cmake --build build/debug`。
- 产物：`build/<cfg>/bin/ContextMenuMaster.exe`（GUI）+ `CmmCli.exe`（调试）。
- CLI 调试：`lang`/`status`/`enable`/`restore`/`toggle`/`enable-norefresh`/`scan <scene>`/`probe <scene> <clsid> [真实文件路径]`/`set-enabled <scene> <on|off> <id>`（启停项，验证提权写路径，id 走宽命令行支持中文/空格）。

---

## ▍已完成

**后端（C++）**
- [x] i18n 系统语言检测；输出 zh-CN/zh-TW/en 标签。
- [x] Win11 经典菜单接管：HKCU 空 InprocServer32 写/删，状态判定严谨，enable/restore/toggle 幂等返回 `MenuResult` 四态。
- [x] 分离子进程优雅重启 Explorer（修复旧版卡死事故，根因见踩坑 10）。
- [x] 真实菜单扫描 `menu_scanner`：6 场景 verb（明文，可改名/图标/启停）+ handler（COM 扩展，含 disabled mirror 双根扫描，踩坑 17），label 三档兜底解析（踩坑 11），verb 级联子菜单静态展开，handler 图标源从 CLSID DefaultIcon/InprocServer32 提取。**写回走真实 hive**（提权后 HKLM/HKCU 智能选择，`primary_write_hive`，踩坑 15/16），写前自动 `reg.exe export` 备份到 `%LOCALAPPDATA%\ContextMenuMaster\backups\`。SYS 标记按 InprocServer32 DLL 是否在 `%SystemRoot%` 下精准判定（不再误标第三方）。
- [x] **UAC 提权**：`app.manifest` requireAdministrator，启动即请求管理员，才能写 HKLM 真实位置（踩坑 14）。
- [x] **异步 bind**：probe/extract_icon 后台线程执行 + `w.resolve` 回填，不阻塞 UI（踩坑 18）。
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
- [x] 可视化视图右键编辑：经典菜单行右键弹 FairyGlass 操作菜单（改名/换图标/启停），复用列表视图同套方法（系统项二次确认、自动备份 toast 全继承），与列表视图功能对齐。现代菜单仍只读不挂右键。
- [x] 关于页「作者与版本」卡片：作者 / 版本 / 协议 / 项目主页（三语），主页链接走后端 `open_url` bind（`ShellExecuteW` 开默认浏览器，仅放行 https 防注入）；页脚版本号读 `appVersion` 与 CMake 工程版本统一。
- [x] Win11 专区：经典/新版菜单状态 + 一键互换。
- [x] Windows 内建项三语友好名兜底 `BUILTIN_NAMES`（verb 按 id、handler 按清洗后 label，含夸克AI）。
- [x] 列表操作按钮统一 FairyGlass 玻璃风（玻璃填充+描边+圆角+图标，hover 提蓝边、active 微缩）。
- [x] 启停开关关态可辨（深底+明描边+inset，修了只显示一颗球的 bug，见踩坑 13）；主题/语言弹层叠 86% 深底防透明看不清。
- [x] 主题/语言弹层被下方内容盖住的 bug：`.header` 加 `position:relative; z-index:20` 建独立层叠上下文，压过 content 里各自成栈的 `.glass` 卡片（backdrop-filter 会新建 stacking context）。见踩坑 19。

---

## ▍富媒体愿景路线图（差异化核心，别再丢）

参考项目 ContextMenuMgr 只是「注册表管理器」。本项目要做「系统工具 + 美化工具」，两大特色见「一句话定位」。

**路线定型（恒烈 2026-07-07 拍板）**：富媒体走 **IContextMenu COM 扩展 DLL**（Bandizip/7-Zip 那套官方扩展点），**不做 Nilesoft 式全局 hook 自绘**。理由：hook 全局接管有系统风险（explorer 崩则全崩、灰色 hack），违背「不折腾用户系统」定位；而 IContextMenu 是微软官方扩展点，只挂在指定文件类型上、隔离、卸载即净。这才是 Bandizip「右键 .zip 看压缩包内容 / 预览图片缩略图」的真实实现。

- **阶段 0（已具备）**：自建菜单项支持自定义图标（ICO/DLL 资源路径）+ FairyGlass 可视化 1:1 仿真编辑。这是特色 ① 的地基。
- **阶段 1 · 图片图标**（现实第一步，纯注册表）：自建项图标从「限 ICO/DLL」扩到「任意 PNG/JPG/BMP」。做法：图片自动转 ICO（GDI+ 或 WIC 编码多尺寸 ico）存到 `%LOCALAPPDATA%\ContextMenuMaster\icons\`，注册表 `Icon` 指向它。经典菜单原生就能显示。**无需 COM，无需重启 explorer。**
- **阶段 2 · IContextMenu 扩展 DLL**（中大，独立子工程）：写一个进程内 COM DLL 实现 `IContextMenu`，注册到指定文件类型的 `HKCR\<类型>\shellex\ContextMenuHandlers`。能力两层——① **动态子菜单**：右键某类型文件时 DLL 现场算内容（如读压缩包/文件夹列文件），`InsertMenuItem` 动态塞项，不写死注册表；② **owner-draw 富显示**：菜单项设 `MFT_OWNERDRAW` + 实现 `IContextMenu2/3::HandleMenuMsg`，在 `WM_MEASUREITEM`/`WM_DRAWITEM` 里自绘位图/缩略图/渐变。这是「预览压缩包、缩略图片」的机制本体。DLL 崩溃隔离、生命周期、注册/反注册都要处理，比纯注册表重一个量级。
- **皮肤系统**（跨阶段）：菜单外观预设（配色/圆角/字体/间距/背景），阶段 2 owner-draw 起可套用，导出/导入 `.cmmskin`。

**Win11 现代层天花板（硬限制，谁都绕不过）**：owner-draw 自绘缩略图**只在经典菜单层生效**（Shift+右键 /「显示更多选项」）。Win11 默认现代菜单只认 `IExplorerCommand`，第三方项**只能出「文本 + 单色图标」，不支持自绘位图/缩略图**。Bandizip 在 Win11 上的富预览也得点「显示更多选项」才出。结论：**经典层能做全套富媒体，现代层最多文本+图标**——除非退回 Nilesoft hook（已否决）。视频/动图同理，shell 菜单无原生承载，只能靠经典层 owner-draw 自绘。

---

## ▍待办

- [~] **场景补全（对标参考项目，恒烈要「以上全部」）**：顶层导航已拍平（6 场景 + Win+X 平级）。A 批注册表主场景**已落地**——`menu_scanner` 加 AllFileSystemObjects(`AllFilesystemObjects`,id=allfsobjects)、Computer(此电脑 CLSID,id=computer,无 handler)、Library(`LibraryFolder`,id=library)，enum+5 switch 补 case，CLI 真跑验证(allfsobjects=11/computer=2/library=1 项)。**注：folder 场景后端本就映射 `Directory\shell`，与参考项目 Directory 是同一根，不重复加**。B 批（特殊菜单，各独立模块）**已落地 + CLI 真跑验证**：`sendto_menu`(仿 winx 文件系统机制,无 hash,枚举/启停/增删,真跑 6 项)、`shellnew_menu`(遍历 HKCR\.ext\ShellNew,`ShellNew`↔`-ShellNew` 搬家停用,写前 reg 备份,真跑多扩展名友好名解析)、`dragdrop_menu`(handler-only 跨 Directory/Folder/Drive/AllFilesystemObjects 聚合,复用 move_handler 双 hive 搬家,真跑 9 项)、`openwith_menu`(只读枚举 Applications,canToggle=false,启停有风险暂不做)。C 批文件类型子场景 8 个(需动态 UI 输入,最后做)。前端(T19)：4 场景 + 三语文案 + 统一特殊菜单视图(sendto 增删,openwith 只读)已接,10 个 GUI bind,cp 通。**待恒烈真机 GUI 目视 + 启停/增删验证**。
- [~] **对标 ContextMenuMgr 追平 + 差异化（第四批,分三档推进,恒烈定优先级）**。第一批(轻+追平)**已落地**：① **全局搜索**(纯前端,标题栏跨场景跳转搜索器,`prefetchCounts` 顺带把所有注册表+特殊场景全量项建 `searchIndex` 内存索引,多字段忽略大小写匹配,点结果切场景+`data-item-id` 定位高亮 flash)；② **OpenWith CRUD**(`openwith_menu` 从只读升级:启停=写/删 `NoOpenWith`、新增写 HKCU Applications、删除软删到 `<app>.deleted`,系统 HKLM 项拒删;canToggle 改 true,CLI 真跑 enabled/canToggle/system 字段就位)；③ **ShellNew 创建/删除**(`add_shellnew_item` 写 HKCU `.ext\ShellNew\NullFile`、`del_shellnew_item` 仅删用户项)。前端 sp-view 的 add/del 泛化为场景感知(sendto/openwith/shellnew 各自弹层+bind),三语补齐。**第二批**(Win11 新菜单管理)**已落地**:新建 `win11menu.{h,cpp}` 枚举 PackagedCom(`HKCR\PackagedCom\Package`→PackageRepository 取安装路径→IXmlReader 流式解析 AppxManifest 的 FileExplorerContextMenus/ComServer CLSID 交集)+ SystemCommandStore 白名单(仅 `Windows.SendToMyPhone`/`Windows.Share`,避免上百内部只读命令噪音);启停走 `Shell Extensions\Blocked` 黑名单 HKLM+HKCU 双写(含大括号/无括号变体清理);PackagedCom 项可启停、system 项只读(误封锁会致现代菜单回退经典)。CLI 真跑验证本机 40 packaged + 2 system。前端 Win11 专区加分组列表(可启停/只读徽标),watch tab 懒加载,三语。**真机反馈修复轮**:① `set_win11_enabled`/`set_openwith_enabled` 的 bool 参数错用 `parse_string_arg_at=="true"` 取值(JS bool 序列化为裸 `true` 无引号,取不到恒 false → "启用"实际又执行"停用",导致关掉的项开不回来),统一改 `parse_bool_arg`;② Win11 启停后自动 `restart_explorer`(黑名单变更 explorer 有缓存,不重启不生效);③ `scene:{}` i18n 补 sendto/shellnew/dragdrop/openwith 四键(此前特殊场景侧栏显示 raw key `scene.sendto`);④ Win11 专区导航置顶(pageItems 移出 win11,nav 顶部单独入口)。**第三批**(高度自定义自建菜单项)**已落地**:新建 `custom_menu.{h,cpp}` 在任意场景写标准 `shell\<verb>\command`(verb 名带 `cmm_` 前缀标记自建项,menu_scanner 扫到即置 `custom=true`);执行类型模板——程序=`"exe" "%1"`(背景场景用 `%V`)、文件夹=`explorer.exe "path"`、URL=`cmd /c start "" "url"`、命令=原样;支持图标(Icon)、位置(Position=Top/Bottom);写 HKCU\Software\Classes,删除仅限 cmm_ 前缀项(RegDeleteTreeW 整键删)。CLI 真跑闭环(建→scan 可见 custom=true→删净)。前端:列表视图工具栏加「新建菜单项」按钮,自建项行显示「编辑/删除」(替代改名/换图标),自建编辑器弹层(名称/执行类型/目标/图标/位置),三语。**Win11 专区加可视化预览**:仿真 Win11 现代菜单外观(圆角/Mica/顶部工具条/"显示更多选项"),渲染已启用的 PackagedCom 项,只读示意(现代菜单系统管理,启停在下方列表操作)。审核/常驻服务/写保护恒烈明确不做(应属杀软/主动防御,非本工具职责)。**第五批(全菜单统一可视化编辑)**已增量落地:① 特殊菜单(sendto/shellnew/dragdrop/openwith)从扁平列表升级为「飞出子菜单」可视化对象(父项常驻+子项列,行内启停/删除),下方保留详情列表;② Win11 专区小预览卡升级为完整可编辑现代菜单对象(全部 packaged 项行内开关、停用灰显、系统项只读);③ 经典场景 visual-stage 菜单行(普通项+级联父项)补行内启停开关+自建项行内删除,启停不再只藏右键菜单。诚实记录:.spv-* 与 .ctx-* 两套菜单皮肤仍并存(未走 Agent 建议的合并重构),sp 飞出项暂只启停/删除、无改名换图标,留待后续真重构。
- [~] **第五批：全菜单统一可视化编辑界面（进行中）**。目标:所有场景/特殊菜单都用统一的「可视化右键菜单对象」1:1 仿真渲染 + 在菜单上直接编辑,取代当前的列表/弹层为主的编辑方式。分两类对象——① 主菜单类(desktop/file/folder/... + 自建项):完整右键菜单外观,项可就地改名/换图标/启停/删,自建项可视化新建;② 子菜单类(sendto/shellnew/dragdrop/openwith):渲染为从父项飞出的短子菜单(flyout),项启停/增删。Win11 专区也纳入统一框架(现有小预览卡升级为完整可视化编辑面板)。复用现有 ctx-menu/ctx-row/ctx-flyout 类。
- [x] **中文硬编码 bug 修复**：`shellnew_menu.cpp` 原拼死 `" 文件 ("` 中文(违反 i18n 红线),改后端返 `friendlyName`(注册表已本地化的类型名,可空)+`ext` 分开,前端 `spLabel` 按 friendlyName+ext 自拼、空则 i18n `unknown_type` 兜底;dragdrop 走 BUILTIN_NAMES.handler 兜底。
- [x] **侧栏滚动条 bug**：场景从 7 涨到 11+ 项后溢出窗口底、按钮点不到。修:`.side{height:100%;overflow:hidden}` + `.nav{flex:1;min-height:0;overflow-y:auto}`(min-height:0 是 flex 子项能收缩出滚动条的关键) + 品牌/页脚 flex-shrink:0 固定 + 玻璃细滚动条。
- [x] **提权写路径真机验证（踩坑 21 三处修复）**：加 `CmmCli set-enabled` 命令脱离 GUI 驱动 `set_item_enabled`，管理员下实测通过——① 123pan（HKLM+HKCU 双 LegacyDisable 样本）启用两 hive 都删、停用两 hive 对称写，scan enabled 正确翻转；② SD360 handler 停用/启用往返，CLSID 随树搬家不丢，双根扫描认出停用镜像；③ 每次写前 do_backup(HKCR\* 整树) 均 ok，30s 超时修复隐式通过。**另修一真 bug：`handler_at_target` 在匿名 namespace 重复定义两次（C2084），踩坑 21 那批代码原来从没编译过。** 遗留小瑕疵：找不到的 handler 返 `registry_failed`、verb 返 `not_found`，语义不统一（均失败态无副作用，待定要否对齐）。GUI 切场景卡顿另见踩坑 18 已改，未在此轮复测。
- [~] **提权改造 GUI 真机复测**：写路径已用 CLI 验通（上条）。仍待启 GUI 走一遍：停用后菜单实际不再出现、启用后回来、切场景不卡（踩坑 18 异步 bind 效果）。
- [ ] **verb 级联子菜单真机验证**：本机无 WinRAR 压缩组/TortoiseGit 这类样本，静态展开逻辑就位但没出过真实实例，装一个验证飞出渲染。
- [ ] **列表视图折叠父项**：暂未对折叠/COM 父项做子项展开或标注（待定要不要加）。
- [~] **Win+X 菜单管理（新功能，移植参考项目）**：后端 `winx_menu.{h,cpp}` 已完成——枚举 `%LOCALAPPDATA%\Microsoft\Windows\WinX\Group1~3` 的 `.lnk`/`.lnk.disabled`（IShellLink 读目标）、启停改扩展名、hash 算法移植（`HashData` 对「泛化路径+参数+固定盐」小写 UTF-16 算 CRC，写 `System.Winx.Hash` VT_UI4）、新增（IShellLink 建 lnk + 写 hash）/删除（送回收站）。CLI 探针 + GUI bind + 前端场景视图在接（多 Agent T8/T10）。真机待验：新增项 hash 写对后 Win+X 菜单能否真实显示。
- [ ] Win11 IExplorerCommand 稀疏包(MSIX)扫描 + 写入（README 已声明开发中）。这是独立大功能：要 `IExplorerCommand` COM 组件 + 稀疏包打包注册，不是改注册表能成，与权限无关。
- [x] 编译 x64 Release + 打包发行（v0.1.0，随 GitHub Release 附 zip）。x86 Release 待出（需 vcvars32 环境）。
- [x] git 初始化（Henglie 身份，零 AI 痕迹）+ 推送 GitHub（main 分支，ssh）。

---

## ▍用户偏好（硬约束，四项目通用）

- 作者恒烈 / Henglie。熟 Windows 桌面 + GDI+，**不熟 Rust、初学 CMake**。中文交流。
- **反感 emoji**：回复和界面都克制，导航/图标用黑白几何符号。
- git 提交用 `Henglie <ebhenglie@gmail.com>`，**绝不出现 Claude 署名 / Co-Authored-By / AI 痕迹**。
- 要 GUI 不要命令行。讲清「为什么」，给推荐而非罗列；能定的直接做，不确定才问。
- **删除文件走回收站**，不用 rm 直接抹除。


## 第八批（已完成）: 特殊菜单 sendto 改名/换图标

- 目标: 给 sendto 项右键菜单加改名/换图标(openwith/shellnew 涉及全局文件类型风险高,本批只做 sendto)。
- 后端 `sendto_menu.cpp`: `rename_sendto_item`(改文件名主干,保留原扩展名与 .disabled 后缀,启停态不变;含 \ / .. 非法校验)+ `set_sendto_icon`(仅 .lnk 项,写 IShellLink IconLocation,支持 "路径,索引")。`.h` 声明、`main.cpp` CLI(sendto rename/seticon 子命令,宽命令行取参含中文)、`gui_main.cpp` GUI bind(双参异步)全部落地。
- 前端 `ui/index.html`: data 补 `spEdit` 字段;sp 右键菜单加改名(仅 sendto)/换图标(仅 sendto 且 .lnk)入口;spEdit modal 复用 winx-add 样式;三语补 `toast.not_lnk`/`toast.icon_failed`。
- 顺带修复 `win11menu.cpp` 历史损坏(HEAD 提交里就编译不过): 5 处字符串字面量被控制字符污染(`|`/`b`/`r` 被换行/退格/回车吃掉)+ `pkgMerge` 声明缺失。现全库编译通过。
- 验证: debug 全量编译 0 error(真看 ninja); CmmCli 闭环真跑(含中文名 rename、seticon 写图标、非 .lnk 拒 not_lnk、非法名拒 bad_name、不存在拒 not_found、保留扩展名、清理无残留); JS 语法检查通过; build/debug 副本已同步。

## 第九批（已完成）: C 批文件类型场景（追平最后一块）+ 富媒体路线定调

**背景决策（本轮敲定，已写进定位/路线图）**:
- 富媒体特色改走 **IContextMenu COM DLL**（Bandizip 那套：只挂特定文件类型、标准 shell 扩展点、owner-draw 画缩略图），**砍掉原 Nilesoft hook 自绘方案**（全局接管、系统风险高，恒烈否决）。定位标杆改 Bandizip。
- Win11 硬边界：owner-draw 富显示只在经典菜单层（显示更多选项）生效，现代菜单第三方只能文本+单色图标，微软限制绕不过。详见 PROGRESS「▍富媒体愿景路线图」。
- 富媒体 COM DLL 是独立重活，未开工；本轮只推 C 批（纯注册表，追平 ContextMenuMgr 文件类型页）。**至此追平 ContextMenuMgr 全部场景完成。**

**C 批机制（真机 regedit 实测，别再重探）**: 一个扩展名的右键菜单项分散在 4 类根——
1. `HKCR\.ext\shell` + `\shellex\ContextMenuHandlers`（扩展名直挂，少见）
2. `HKCR\<ProgID>\shell`（.txt→txtfile、.zip→7-Zip.zip/CompressedFolder）
3. `HKCR\SystemFileAssociations\.ext\shell`（.jpg 的 ShellImagePreview 在此）
4. `HKCR\SystemFileAssociations\<PerceivedType>\shell`（image/text/… 一类共享）
scan 聚合这 4 根 + 各自 shellex 启用/停用镜像。新建自建项写 HKCU SFA\.ext（用户级最稳，对该扩展名所有文件生效，不碰 ProgID/全局）。

**后端 `filetype_menu.{h,cpp}`（853 行，全部落盘 + 编译通过）**:
- id 格式 `<tag>|verb:<name>` / `<tag>|handler:<name>`（tag: ext/progid/sfaext/sfaperceived），场景内唯一。
- `resolve_filetype`（规范化 .ext + 解析 ProgID/PerceivedType）、`scan_filetype`/`scan_filetype_json`（4 根聚合，去重，返 `{assoc,items}`）。
- `set_filetype_enabled`：verb 加/删 `LegacyDisable`、handler `ContextMenuHandlers`↔`-ContextMenuHandlers` 搬家，两 hive（HKLM/HKCU）对称处理，幂等（已在目标态不报假失败）。仿 menu_scanner 成熟做法。
- `add_filetype_item`（写 HKCU SFA\.ext\shell\cmm_xxx，program/folder/url/command 四型命令组装 + Icon/Position）、`del_filetype_item`（仅 cmm_ 前缀、仅 HKCU）、`backup_filetype`（reg export 4 根到 %LOCALAPPDATA%\ContextMenuMaster\backups）。写前自动备份即将改动的根。

**CLI（main.cpp）**: `CmmCli filetype scan|toggle|add|del|backup <ext> ...`，label/id 走宽命令行取（支持中文）。

**GUI（gui_main.cpp）**: 5 个 bind（scan_filetype 同步；set_filetype_enabled/add/del/backup 异步 3 参）。

**前端（ui/index.html）**: 独立第三类场景（需先输入 .ext 再 scan，区别于固定/特殊场景）——nav 项（二级菜单组内）、ft-view 主视图（扩展名输入框 + 关联信息 chips + 4 根分组标签 + 启停开关 + 自建项删除）、ftAdd 弹层（复用 custom 字段）、右键 ft 分支（启停+删自建）、8 个 ft 方法、selectScene/setScene 分支（不自动扫描，等用户输入）。三语 i18n（scene.filetype + 完整 ft 文案组）。

**验证（真机闭环全绿）**:
- scan .txt（progid txtfile 的 open/print）/.jpg（sfaperceived image）/.zip（progid 7-Zip.zip + sfaext 金山转PDF 带前导空格中文项 + Compressed Folder handler）四根聚合、中文 label、停用态读取全对。
- 自建项 add→scan(en=true)→toggle off(en=false)→on(en=true)→del→scan(custom=0) 可逆闭环，注册表零残留。
- 系统 verb（金山转PDF，HKLM LegacyDisable 停用样本）启停往返后完美复原（HKLM 保留 LegacyDisable、HKCU 无残留键）。
- debug 全量编译 0 error（真看 ninja：filetype_menu.cpp 单编 + core/CLI/GUI 三产物链接全过）。JS 语法 node 检查 RC 0。bin/ui 已手动同步（identical）。

## 第十批（已完成）: Win11 专区默认视图修复 + 侧栏导航分组重构

- **Win11 专区默认空白 bug**：`win11View` 在 data 里从未定义 → 两个 `v-show`（visual/list）都 false → 进专区什么都不显示。修：data 加 `win11View: 'visual'`（默认可视化视图）。
- **侧栏导航重构分组**：原本 11+ 场景平铺一列太乱。改为三组带小标题（`nav-group`）——① 常用场景（desktop/file/folder/folder_bg/drive/recycle/allfsobjects/computer/library）② 二级菜单（sendto/shellnew/dragdrop/openwith + filetype）③ Win+X 独立。Win11 专区保持置顶。
- **关于移出滚动列表**：原「关于」在 nav 滚动区内（`pageItems` v-for），场景多时要滚到底才看到。改为 `nav-about` 常驻侧栏底部（foot 之上），一直可见。三语补 `nav.grp_common/grp_special/grp_winx`。
- 验证：JS 语法 node RC 0，bin/ui 已同步。GUI 目视待恒烈确认。
