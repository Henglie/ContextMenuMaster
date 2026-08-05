<div align="center">

<img src="../../res/app-256.png" width="128" height="128" alt="Context Menu Master" />

# 滑鼠右鍵主理人 Context Menu Master

**在 Windows 11 上，把右鍵選單重新交回你手裡** · 液態玻璃介面 · 純本機、可逆

[![Release](https://img.shields.io/badge/release-v0.6-4d9fff?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-6a5ae0?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![License](https://img.shields.io/badge/license-MIT-22c55e?style=flat-square)](../../LICENSE)
[![Arch](https://img.shields.io/badge/arch-x64%20%7C%20x86%20%7C%20arm64-f59e0b?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)

<sub>Core-Shell 架構 · C++17 核心 · WebView2 前端 · 16 語言介面</sub>

</div>

---

[简体中文](../../README.md) | **繁體中文** | [English](../../README.en.md) | [日本語](README.ja.md) | [한국어](README.ko.md) | [Français](README.fr.md) | [Deutsch](README.de.md) | [Español](README.es.md) | [Português](README.pt.md) | [Русский](README.ru.md) | [Italiano](README.it.md) | [العربية](README.ar.md) | [עברית](README.he.md) | [हिन्दी](README.hi.md) | [ไทย](README.th.md) | [Tiếng Việt](README.vi.md)

> 在 Windows 11 上重新主理你的滑鼠右鍵選單。

**滑鼠右鍵主理人**是一款 Windows 桌面效能工具。它把雜亂、被摺疊、被各種軟體塞滿的右鍵選單交還到你手裡：視覺化地改名換圖示、批次勾選管理、一鍵找回 Windows 11 被藏起來的傳統選單，並涵蓋桌面 / 檔案 / 資料夾 / 磁碟機 / 資源回收筒 / Win+X / 傳送到 / 新增 / 拖放等全部右鍵情境。

純本機執行，寫入操作前自動備份登錄檔，所有操作隨時可逆。以系統管理員身分執行以便管理系統與所有使用者（HKLM）範圍的選單項目。

> [!WARNING]
> **富媒體右鍵擴充功能（`CmmShellExt.dll`）目前是實驗性功能。** 它是一個處理程序內 COM Shell 擴充功能，需要系統管理員手動 `regsvr32 CmmShellExt.dll` 註冊後才會生效；註冊後右鍵檔案才會顯示帶真實縮圖的選單項目。這部分尚未經過大範圍實機驗證，在個別系統、個別檔案類型或安全軟體環境下可能表現不穩定甚至不顯示。它內建了防護機制（壓縮檔超過 200MB 或列舉超過 800ms 自動降級為普通選單項目，不會卡住右鍵選單），但仍不建議在重要環境長期掛著。主程式的選單管理功能不依賴它，可放心單獨使用。

---

## 介面截圖

> 首屏為視覺化檢視：左側欄平鋪全部右鍵情境，中間「現代選單唯讀示意 → 傳統選單可編輯」並排呈現。

![視覺化檢視主介面](../screenshots/main-visual.png)

<table>
  <tr>
    <td width="50%"><img src="../screenshots/list-view.png" alt="清單檢視" /><br/><sub>清單檢視：核取方塊批次勾選、啟停開關、批次魔法工具列</sub></td>
    <td width="50%"><img src="../screenshots/win11-zone.png" alt="Windows 11 專區" /><br/><sub>Windows 11 專區：傳統/新版選單一鍵互換，優雅重啟 Explorer</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="../screenshots/about.png" alt="新增 ShellNew 情境" /><br/><sub>新增（ShellNew）：管理「新增」子選單裡出現哪些檔案類型</sub></td>
    <td width="50%"><img src="../screenshots/desktop-visual.png" alt="桌面空白視覺化" /><br/><sub>桌面空白：視覺化檢視 1:1 模擬右鍵選單</sub></td>
  </tr>
  <tr>
    <td colspan="2" align="center"><img src="../screenshots/shellnew.png" alt="關於頁" width="50%" /><br/><sub>關於：版本 / 授權 / 作者與專案首頁</sub></td>
  </tr>
</table>

---

## 它解決什麼問題

| 情境 | 沒有本工具 | 有了本工具 |
|---|---|---|
| Win11 右鍵選單太精簡，常用項被摺進「顯示其他選項」 | 每次多點一下、多等一下 | 一鍵切回完整傳統選單，瞬間生效 |
| 裝的軟體往右鍵選單塞了一堆項，又亂又長 | 只能去登錄檔裡手動翻 | 清單檢視批次勾選，一眼管理 |
| 想給選單項改個順眼的名字或圖示 | 得懂登錄檔結構才敢動 | 視覺化檢視裡點一下就改 |
| 想統一給一批選單項加個標記 | 一個個手動改 | 批次魔法，勾選後一鍵追加後綴 |

---

## 核心功能

- **全右鍵情境涵蓋**：一個側欄縱向平鋪所有右鍵情境，點一下即切換——
  - **登錄檔情境**：桌面空白、檔案、資料夾、資料夾空白、磁碟機、資源回收筒、所有檔案物件、本機、媒體櫃。
  - **Win+X 選單**：管理「右鍵開始按鈕 / Win+X」那個進階選單（分組顯示、啟停、新增自訂項、刪除）。
  - **傳送到（SendTo）**：管理右鍵「傳送到」子選單的項，可啟停、新增、刪除。
  - **新增（ShellNew）**：管理右鍵「新增」子選單裡出現哪些檔案類型，逐項啟停。
  - **拖放處理常式（DragDrop）**：管理右鍵拖放檔案時彈出的選單擴充功能。
  - **開啟方式（Open with）**：檢視註冊為「開啟方式」候選的應用程式（唯讀）。
- **雙檢視即時同步**
  - **視覺化檢視（WYSIWYG）**：1:1 模擬 Windows 真實右鍵選單，點名稱改名、點圖示換圖示，**右鍵點擊選單項彈出操作選單（改名 / 換圖示 / 啟用停用）**，所見即所得。
  - **清單檢視（List View）**：傳統表格，核取方塊批次勾選，啟停開關、改名換圖示按鈕一應俱全，適合一次管理一批選單項。
- **批次魔法**：勾選若干選單項，一鍵給它們全部追加同一個後綴（如「 ★」），也可自訂。
- **Windows 11 專區**
  - **傳統選單一鍵互換**：在 HKCU 寫入空 `InprocServer32` 讓新版選單擴充功能失效，檔案總管回落到 Win10 傳統選單；並優雅重啟 `explorer.exe`，選單瞬間生效，無需登出重登。再點一下即可恢復。
  - **新版選單擴充功能（開發中）**：預留掃描與接管 Win11 `IExplorerCommand` 稀疏套件（MSIX）擴充項的介面。
- **多語言**：內建 16 種語言介面，首次啟動按系統顯示語言自動選，介面右上角可隨時切換。
- **純本機、可逆**：只讀寫目前使用者登錄檔，不連網、不上傳，刪除機碼即恢復系統預設。

---

## 快速上手

1. 從 [Releases](https://github.com/Henglie/ContextMenuMaster/releases) 下載對應架構的 zip（多數電腦選 x64），解壓縮到任意目錄。
2. 按兩下 `ContextMenuMaster.exe`，在彈出的 UAC 提示裡點「是」授予系統管理員權限（用於讀寫第三方軟體寫在 `HKLM` 的選單項，不提權則只能改目前使用者的項）。
3. 無需安裝、無需連網。首次執行會按系統語言自動選擇介面語言，右上角可在 16 種語言間隨時切換。

---

## 使用方法

### 1 · 選情境

左側欄縱向平鋪全部右鍵情境，分三組：

- **常用情境**：桌面空白、檔案、資料夾、資料夾空白、磁碟機、資源回收筒、所有檔案物件、本機、媒體櫃 —— 對應你在不同物件上點右鍵看到的選單。
- **子選單**：傳送到、新增、拖放、開啟方式、檔案類型 —— 右鍵選單裡的子選單與按副檔名管理。
- **快捷入口**：Win+X（右鍵開始按鈕的進階選單）。

頂部還有 **Windows 11 專區** 和 **富媒體** 兩個獨立入口。點任意情境即掃描該情境的真實選單項，標籤角標顯示項數。

### 2 · 改選單

兩種檢視，右上角切換，資料即時同步：

- **視覺化檢視**：1:1 模擬 Windows 右鍵選單。
  - 點選單項 **名稱** 直接改名；點 **圖示** 換圖示。
  - **右鍵點擊** 任意選單項，彈出操作選單：改名 / 換圖示 / 啟用停用 / 刪除（自建項）。
  - 現代選單一側是唯讀示意（Win11 現代選單不受登錄檔控制，見下方說明），傳統選單一側可編輯。
- **清單檢視**：傳統表格，適合批次操作。
  - 每列有啟停開關、改名 / 換圖示按鈕。
  - 勾選多列後，用 **批次魔法** 給它們一鍵追加統一後綴（如「 ★」），也可自訂。

改名 / 換圖示 / 啟停對系統內建項會二次確認；每次寫登錄檔前自動備份到 `%LOCALAPPDATA%\ContextMenuMaster\backups\`，改壞了可還原。

### 3 · 找回傳統選單（Windows 11）

進 **Windows 11 專區**，點「切換為傳統選單」：程式會讓新版選單擴充功能失效並優雅重啟 `explorer.exe`，右鍵選單瞬間回到 Win10 完整樣式，無需登出重登。再點一下即恢復新版選單。

### 4 · 新增自己的選單項

清單檢視工具列點「新增選單項」，填顯示名稱、執行類型（執行程式 / 開啟資料夾 / 開啟網址 / 自訂命令列）、目標路徑、圖示（可選，支援任意圖片自動轉 ICO）、位置（頂部 / 底部）。自建項帶專屬標記，可隨時編輯或刪除，刪除只清本工具建的項，不碰系統關聯。

> **關於 Win11 現代選單**：Windows 11 的新版右鍵選單只接納帶套件識別（MSIX / 稀疏套件）的 `IExplorerCommand` 擴充功能，傳統登錄檔選單項一律被摺進「顯示其他選項」的傳統選單裡——這是系統設計，與權限無關。所以本工具管理的是傳統選單層；想讓常用項回到頂層，用上面的「找回傳統選單」。

---

## 多語言

介面支援 16 種語言：簡體中文、繁體中文、English、日本語、한국어、Français、Deutsch、Español、Português、Русский、Italiano、العربية、עברית、हिन्दी、ไทย、Tiếng Việt。

首次啟動時，C++ 後端用 Win32 API 讀取系統預設顯示語言，在 WebView2 頁面建立前注入給前端：

- 系統語言命中上述任一種 → 直接用對應語言算繪（中文按簡繁細分）。
- 系統語言不在其中 → 一律回退英文（en）。

介面右上角可隨時手動切換，選擇記入本機儲存。阿拉伯語、希伯來語為從右向左（RTL）語言，切到這兩種時介面整體鏡像佈局。

全部語言資源集中在 `ui/i18n.js` 一個檔案裡，新增一門語言只改這一處即可，無需動介面程式碼。

---

## 技術堆疊與架構

| 層 | 選型 | 說明 |
|---|---|---|
| 後端引擎 | 現代 C++17 / MSVC | Win32 API 母語直呼 |
| 展現層 | Microsoft Edge WebView2 | webview 單頭程式庫承載系統自帶 WebView2 |
| 通訊橋樑 | WebMessage / postMessage | `w.bind` 暴露後端函式，前端 `await window.fn()` 呼叫 |
| 介面 | Vue 3（本機） | 單檔應用 |
| 視覺系統 | FairyGlass | 微藍暗色液態玻璃設計系統，權杖內聯複刻進 `ui/index.html` |
| 建置 | CMake + Ninja | VS2022「開啟資料夾」即識別 |

**架構：Core-Shell（通用核心 + 原生外殼）**，與姊妹專案 FairySave 一致：`src/platform/windows` 是 Win32 特權層（i18n 偵測、登錄檔選單接管），`ui/` 是表現層。

**相容性**：Windows 10 / 11（傳統選單接管為 Win11 特性）。需 WebView2 執行階段（Win10/11 自帶）。

---

## 開發與建置

**環境**：VS2022（含 C++ 桌面工作負載、CMake、Ninja、Windows SDK）+ WebView2 執行階段。

**最簡方式**：VS2022「開啟本機資料夾」選中本目錄，自動讀 `CMakePresets.json` 識別為 CMake 專案，選 `windows-debug` 或 `windows-release` 預設建置、執行。

**命令列方式**（vcvars64 環境下）：

```bat
cmake --preset windows-release
cmake --build build/release
```

產物：

- `build/release/bin/ContextMenuMaster.exe` —— GUI 主程式（WIN32 子系統，無主控台）。
- `build/release/bin/CmmCli.exe` —— 命令列偵錯探針（驗證後端用，非最終使用者面向）。

> 改完 `ui/` 後若只動了 HTML，需手動把 `ui/` 同步到 exe 旁的 `ui/` 目錄（CMake 的 POST_BUILD 僅在重新連結時觸發）。

---

## 目錄結構

```
ContextMenuMaster/
├── CMakeLists.txt              CMake 主設定（≈ .sln，含 /utf-8 /EHsc + Release 瘦身）
├── CMakePresets.json           VS 一鍵識別（windows-debug / release / release-x86 / release-arm64）
├── src/
│   ├── main.cpp                CLI 偵錯入口（CmmCli）
│   ├── app/
│   │   ├── gui_main.cpp        GUI 入口 WinMain：webview 視窗 + bind + 語言注入
│   │   └── app.manifest        UAC requireAdministrator 提權資訊清單
│   ├── platform/windows/       Win32 特權層（每個右鍵情境一個模組）
│   │   ├── i18n.{h,cpp}          系統語言偵測（16 種語言，按系統顯示語言歸一化）
│   │   ├── context_menu.{h,cpp}  Win11 傳統↔新版選單接管 + 優雅重啟 Explorer
│   │   ├── menu_scanner.{h,cpp}  登錄檔情境掃描 + 改名/圖示/啟停/批次後綴
│   │   ├── icon_extract.{h,cpp}  系統圖示/資源/.ico → PNG base64 data URI
│   │   ├── image_icon.{h,cpp}    任意圖片 → 多尺寸 PNG-in-ICO（富媒體階段 1）
│   │   ├── com_menu_probe.{h,cpp} IContextMenu COM 擴充功能動態選單探測
│   │   ├── win11menu.{h,cpp}     Win11 現代選單（PackagedCom / SystemCommandStore）
│   │   ├── winx_menu.{h,cpp}     Win+X 選單（檔案系統 .lnk + hash）
│   │   ├── sendto_menu.{h,cpp}   傳送到子選單
│   │   ├── shellnew_menu.{h,cpp} 新增子選單
│   │   ├── dragdrop_menu.{h,cpp} 拖放處理常式
│   │   ├── openwith_menu.{h,cpp} 開啟方式
│   │   ├── custom_menu.{h,cpp}   自建右鍵選單項
│   │   └── filetype_menu.{h,cpp} 按副檔名管理檔案類型右鍵項
│   └── shellext/               富媒體 Shell 擴充功能 DLL（階段 2，獨立不連結 cmm_core）
│       ├── shellext.cpp          IContextMenu 處理程序內 COM 伺服器 + owner-draw 縮圖
│       └── shellext.def          COM 四匯出
├── third_party/                webview 單頭程式庫 + WebView2 SDK
└── ui/
    ├── index.html              單檔 Vue3 應用（液態玻璃面板）
    ├── i18n.js                 16 語言資源（新增語言只改這一個檔案）
    ├── vue.global.js           Vue 3 執行階段（本機）
    └── liquidGlass.js          液態玻璃輔助
```

---

## 隱私與安全

- **純本機**：不連網、不上傳任何內容。
- **需系統管理員權限**：第三方軟體的選單項常註冊在 `HKLM`（所有使用者），其停用標記也多在 `HKLM`，純使用者態改不動。故程式啟動時要求系統管理員權限，直接讀寫選單項真實所在的登錄檔位置，以支援「所有項都能啟停 / 改名」。
- **寫前自動備份**：每次改登錄檔前先 `reg export` 備份對應機碼到本機 `%LOCALAPPDATA%\ContextMenuMaster\backups\`。
- **可逆**：傳統選單接管隨時刪機碼恢復；啟停走「加刪標記 / 搬到停用鏡像」，可原樣搬回；刪除項一律送資源回收筒。不破壞系統檔案。

---

## 下載

- 預編譯版見 [Releases](https://github.com/Henglie/ContextMenuMaster/releases)：按 CPU 架構選 zip（x64 / x86 / arm64），解壓縮後按兩下 `ContextMenuMaster.exe` 即用，無需安裝。絕大多數電腦選 x64。
- 需 WebView2 執行階段（Windows 10/11 自帶）。

---

## 授權與作者

- **作者**：恒烈 (EternalBlaze / Henglie)
- **版本**：v0.6
- **專案首頁**：<https://github.com/Henglie/ContextMenuMaster>
- **授權**：MIT
