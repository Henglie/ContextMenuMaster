<div align="center">

<img src="res/app-256.png" width="128" height="128" alt="Context Menu Master" />

# Context Menu Master 鼠标右键主理人

**Take back control of your right-click menu on Windows 11** · Liquid-glass UI · Fully local, fully reversible

[![Release](https://img.shields.io/badge/release-v0.6-4d9fff?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-6a5ae0?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![License](https://img.shields.io/badge/license-MIT-22c55e?style=flat-square)](LICENSE)
[![Arch](https://img.shields.io/badge/arch-x64%20%7C%20x86%20%7C%20arm64-f59e0b?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)

<sub>Core-Shell architecture · C++17 core · WebView2 frontend · 16-language UI</sub>

</div>

---

[简体中文](README.md) | [繁體中文](docs/readme/README.zh-TW.md) | **English** | [日本語](docs/readme/README.ja.md) | [한국어](docs/readme/README.ko.md) | [Français](docs/readme/README.fr.md) | [Deutsch](docs/readme/README.de.md) | [Español](docs/readme/README.es.md) | [Português](docs/readme/README.pt.md) | [Русский](docs/readme/README.ru.md) | [Italiano](docs/readme/README.it.md) | [العربية](docs/readme/README.ar.md) | [עברית](docs/readme/README.he.md) | [हिन्दी](docs/readme/README.hi.md) | [ไทย](docs/readme/README.th.md) | [Tiếng Việt](docs/readme/README.vi.md)

> Take back control of your right-click menu on Windows 11.

**Context Menu Master** is a Windows desktop productivity tool. It hands your cluttered, collapsed, third-party-stuffed context menu back to you: rename and re-icon items visually, batch-manage with checkboxes, restore the classic menu Windows 11 hides — all one click away, covering every right-click scene including Desktop / File / Folder / Drive / Recycle Bin / Win+X / Send To / New / Drag-and-Drop.

Runs fully local, auto-backs up the registry before every write, and every operation is reversible. Run as administrator to manage system- and all-user-scope (HKLM) menu items.

> [!WARNING]
> **The rich-media shell extension (`CmmShellExt.dll`) is currently experimental.** It is an in-process COM shell extension that only takes effect after you manually register it as administrator with `regsvr32 CmmShellExt.dll`; once registered, right-clicking a file shows menu items with real thumbnails. This part has not been widely validated on real machines and may behave unstably — or not appear at all — on some systems, file types, or security-software environments. It ships with safeguards (archives over 200 MB or enumeration over 800 ms auto-degrade to a plain menu item, so the menu never freezes), but it is still not recommended to leave it registered long-term in important environments. The main app's menu-management features do not depend on it and can be used on their own.

---

## Screenshots

> The startup screen is the visual view: the left sidebar lists every right-click scene, and the center shows "modern menu read-only preview → editable classic menu" side by side.

![Visual view main screen](docs/screenshots/main-visual.png)

<table>
  <tr>
    <td width="50%"><img src="docs/screenshots/list-view.png" alt="List view" /><br/><sub>List view: checkbox batch selection, enable/disable switches, Batch Magic toolbar</sub></td>
    <td width="50%"><img src="docs/screenshots/win11-zone.png" alt="Windows 11 Zone" /><br/><sub>Windows 11 Zone: swap classic/new menu in one click, gracefully restart Explorer</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="docs/screenshots/about.png" alt="New (ShellNew) scene" /><br/><sub>New (ShellNew): manage which file types appear in the "New" submenu</sub></td>
    <td width="50%"><img src="docs/screenshots/desktop-visual.png" alt="Desktop visual view" /><br/><sub>Desktop: visual view mirrors the real right-click menu 1:1</sub></td>
  </tr>
  <tr>
    <td colspan="2" align="center"><img src="docs/screenshots/shellnew.png" alt="About page" width="50%" /><br/><sub>About: version / license / author and project homepage</sub></td>
  </tr>
</table>

---

## What problem does it solve

| Scenario | Without this tool | With this tool |
|---|---|---|
| The Win11 menu is too minimal — common items get folded into "Show more options" | An extra click and an extra wait every time | One click back to the full classic menu, instantly |
| Installed software stuffs a dozen items into the right-click menu — messy and long | Dig through the registry by hand | Batch-select in the list view, manage at a glance |
| You want a nicer name or icon for a menu item | You'd need to understand the registry to dare touch it | Click once in the visual view |
| You want to tag a batch of menu items uniformly | Edit them one by one | Batch Magic — select, then append a suffix in one click |

---

## Core features

- **Full right-click coverage**: one sidebar lists every right-click scene vertically, switch with a single click —
  - **Registry scenes**: Desktop, File, Folder, Folder background, Drive, Recycle Bin, All file objects, This PC, Library.
  - **Win+X menu**: manage the power-user menu behind "right-click Start / Win+X" (grouped display, enable/disable, add custom items, delete).
  - **Send To (SendTo)**: manage items in the right-click "Send to" submenu — enable/disable, add, delete.
  - **New (ShellNew)**: manage which file types appear in the right-click "New" submenu, toggle item by item.
  - **Drag-drop handlers (DragDrop)**: manage the menu extensions that appear when you right-drag a file.
  - **Open with**: view apps registered as "Open with" candidates (read-only).
- **Two views, live sync**
  - **Visual view (WYSIWYG)**: mirrors the real Windows right-click menu 1:1 — click a name to rename, click an icon to change it, **right-click a menu item for an action menu (rename / change icon / enable-disable)**, what you see is what you get.
  - **List view**: a traditional table with checkbox batch selection, enable/disable switches, rename and change-icon buttons — ideal for managing a batch of items at once.
- **Batch Magic**: select several menu items and append the same suffix to all of them in one click (e.g. " ★"), or customize it.
- **Windows 11 Zone**
  - **One-click classic swap**: writes an empty `InprocServer32` under HKCU to disable the new-menu extensions, so Explorer falls back to the Win10 classic menu; then gracefully restarts `explorer.exe` so the menu takes effect instantly, no sign-out needed. Click again to restore.
  - **New-menu extensions (in development)**: reserved interface for scanning and managing Win11 `IExplorerCommand` sparse-package (MSIX) extension items.
- **Multilingual**: 16 built-in UI languages, auto-selected by system display language on first launch, switchable any time from the top-right.
- **Local & reversible**: reads/writes only the current-user registry, no network, no upload; delete the key to restore the system default.

---

## Quick start

1. Download the zip for your architecture from [Releases](https://github.com/Henglie/ContextMenuMaster/releases) (most PCs want x64) and extract it anywhere.
2. Double-click `ContextMenuMaster.exe` and click "Yes" on the UAC prompt to grant administrator rights (needed to read/write menu items third-party software writes to `HKLM`; without elevation you can only edit current-user items).
3. No install, no network needed. On first run it auto-selects the UI language by your system language; switch among 16 languages any time from the top-right.

---

## How to use

### 1 · Pick a scene

The left sidebar lists every right-click scene vertically, in three groups:

- **Common scenes**: Desktop, File, Folder, Folder background, Drive, Recycle Bin, All file objects, This PC, Library — matching the menus you see when right-clicking different objects.
- **Submenus**: Send To, New, Drag-drop, Open with, File Type — the submenus inside the right-click menu, plus management by extension.
- **Quick access**: Win+X (the power-user menu behind right-clicking Start).

At the top there are also two standalone entries, **Windows 11 Zone** and **Rich Media**. Click any scene to scan its real menu items; the tab badge shows the item count.

### 2 · Edit the menu

Two views, switch at the top-right, data stays in sync:

- **Visual view**: a 1:1 mockup of the Windows right-click menu.
  - Click an item's **name** to rename it; click its **icon** to change it.
  - **Right-click** any item for an action menu: rename / change icon / enable-disable / delete (custom items).
  - The modern-menu side is a read-only preview (the Win11 modern menu isn't registry-controlled, see the note below); the classic-menu side is editable.
- **List view**: a traditional table, ideal for batch operations.
  - Each row has an enable/disable switch and rename / change-icon buttons.
  - Select multiple rows, then use **Batch Magic** to append a uniform suffix (e.g. " ★") to all in one click, or customize it.

Rename / change-icon / enable-disable on system built-in items asks for confirmation; before every registry write it auto-backs up to `%LOCALAPPDATA%\ContextMenuMaster\backups\`, so you can restore if something breaks.

### 3 · Restore the classic menu (Windows 11)

Go to the **Windows 11 Zone** and click "Switch to Classic menu": the app disables the new-menu extensions and gracefully restarts `explorer.exe`, so the right-click menu snaps back to the full Win10 style instantly, no sign-out needed. Click again to restore the new menu.

### 4 · Create your own menu item

In the list-view toolbar click "New item" and fill in the display name, action type (run program / open folder / open URL / custom command line), target path, icon (optional — any image is auto-converted to ICO), and position (top / bottom). Custom items carry a dedicated marker and can be edited or deleted any time; deleting only removes items this tool created and never touches system associations.

> **About the Win11 modern menu**: The Windows 11 new right-click menu only accepts package-identified (MSIX / sparse-package) `IExplorerCommand` extensions; traditional registry menu items are all folded into the classic menu behind "Show more options" — this is by system design, unrelated to permissions. So this tool manages the classic-menu layer; to bring common items back to the top level, use "Restore the classic menu" above.

---

## Real-world examples

The scenarios below are the most common; do one once and you're up to speed.

### Example 1: Your right-click menu is stuffed by software — trim it to just the essentials

After installing a cloud drive, an archiver, a media player, and a downloader, right-clicking a file spews a dozen items and finding "Copy" takes a scan.

1. Click **File** in the left sidebar (or **All file objects**, which applies to any file).
2. Switch to **List view** at the top-right to see all items and their source (which software added them) at a glance.
3. Turn off the enable/disable switch for items you don't need — the cloud drive's "Upload to…", the player's "Add to playlist", and so on.
4. Disabled items just get a disable marker and can be re-enabled any time; no data is deleted.

![List-view batch management](docs/screenshots/list-view.png)

> Tip: if you're unsure what an item does, check its "Source" column (DLL / command line) first, or just disable it and see — everything is reversible; if something breaks, restore from `%LOCALAPPDATA%\ContextMenuMaster\backups\`.

### Example 2: The Win11 menu is too minimal — swap back to the full Win10 menu in one click

Win11 hides common items behind "Show more options" by default, an extra click every time.

1. Go to the **Windows 11 Zone** at the top.
2. Click **Switch to Classic menu**.
3. The app gracefully restarts `explorer.exe` (the desktop and taskbar flashing briefly is normal), and the right-click menu snaps back to the full Win10 style.
4. To switch back to the new menu, click **Restore New Menu** again.

![Windows 11 Zone](docs/screenshots/win11-zone.png)

### Example 3: Tag a batch of menu items uniformly

You want to mark the few menu items you added so they're easy to spot.

1. In **List view**, select the rows you want.
2. In the **Batch Magic** toolbar at the bottom, enter a suffix (e.g. " ★" or " :3").
3. Click **Append**, and the selected items all get that suffix at the end of their names.

### Example 4: Manage the file types that show up in the "New" menu

The right-click "New" submenu is packed with file types you never use (project files from various software).

1. In the **Submenus** group in the left sidebar, click **New (ShellNew)**.
2. Toggle item by item: turn off the types you don't want in "New".
3. Toggling moves items to a "disabled mirror" and can be moved back as-is any time.

![New (ShellNew) management](docs/screenshots/about.png)

### Example 5: Add your own right-click menu item

You want "Open with a certain program" or "Open a frequently-used folder" right from the right-click menu.

1. In the **List view** toolbar, click **New item**.
2. Fill in the display name, action type (run program / open folder / open URL / custom command line), and target path.
3. The icon is optional — pick any image and it's auto-converted to ICO; position can be top or bottom.
4. Save and it takes effect. Custom items carry a dedicated marker; deleting only removes items this tool created and never touches system associations.

---

## Multilingual

The UI supports 16 languages: 简体中文, 繁體中文, English, 日本語, 한국어, Français, Deutsch, Español, Português, Русский, Italiano, العربية, עברית, हिन्दी, ไทย, Tiếng Việt.

On first launch, the C++ backend reads the system default display language via Win32 API and injects it into the frontend before the WebView2 page is created:

- If the system language matches any of the above → render directly in that language (Chinese split into Simplified/Traditional).
- If the system language isn't among them → fall back to English (en).

Switch manually any time from the top-right; the choice is saved to local storage. Arabic and Hebrew are right-to-left (RTL) languages; switching to them mirrors the entire layout.

All language resources live in a single file, `ui/i18n.js` — adding a new language means editing only this one file, no UI code changes.

---

## Tech stack & architecture

| Layer | Choice | Notes |
|---|---|---|
| Backend engine | Modern C++17 / MSVC | Native Win32 API calls |
| Presentation | Microsoft Edge WebView2 | Single-header webview lib hosts the built-in WebView2 |
| Bridge | WebMessage / postMessage | `w.bind` exposes backend functions, frontend calls `await window.fn()` |
| UI | Vue 3 (local) | Single-file app |
| Visual system | FairyGlass | Subtle-blue dark liquid-glass design system, tokens inlined into `ui/index.html` |
| Build | CMake + Ninja | Recognized by VS2022 "Open Folder" |

**Architecture: Core-Shell (portable core + native shell)**, consistent with the sister project FairySave: `src/platform/windows` is the Win32 privilege layer (i18n detection, registry menu takeover), `ui/` is the presentation layer.

**Compatibility**: Windows 10 / 11 (classic-menu takeover is a Win11 feature). Requires the WebView2 runtime (built into Win10/11).

---

## Development & build

**Environment**: VS2022 (with the C++ desktop workload, CMake, Ninja, Windows SDK) + WebView2 runtime.

**Simplest way**: in VS2022 "Open a local folder" and select this directory; it auto-reads `CMakePresets.json` and recognizes it as a CMake project — pick the `windows-debug` or `windows-release` preset to build and run.

**Command line** (in a vcvars64 environment):

```bat
cmake --preset windows-release
cmake --build build/release
```

Outputs:

- `build/release/bin/ContextMenuMaster.exe` — the GUI main program (WIN32 subsystem, no console).
- `build/release/bin/CmmCli.exe` — a command-line debug probe (for backend verification, not end-user facing).

> After editing `ui/`, if you only changed HTML you must manually sync `ui/` to the `ui/` folder next to the exe (CMake's POST_BUILD only triggers on a relink).

---

## Directory structure

```
ContextMenuMaster/
├── CMakeLists.txt              CMake main config (≈ .sln, with /utf-8 /EHsc + Release trimming)
├── CMakePresets.json           VS one-click recognition (windows-debug / release / release-x86 / release-arm64)
├── src/
│   ├── main.cpp                CLI debug entry (CmmCli)
│   ├── app/
│   │   ├── gui_main.cpp        GUI entry WinMain: webview window + bind + language injection
│   │   └── app.manifest        UAC requireAdministrator elevation manifest
│   ├── platform/windows/       Win32 privilege layer (one module per right-click scene)
│   │   ├── i18n.{h,cpp}          System language detection (16 languages, normalized by system display language)
│   │   ├── context_menu.{h,cpp}  Win11 classic↔new menu takeover + graceful Explorer restart
│   │   ├── menu_scanner.{h,cpp}  Registry scene scan + rename/icon/toggle/batch suffix
│   │   ├── icon_extract.{h,cpp}  System icon/resource/.ico → PNG base64 data URI
│   │   ├── image_icon.{h,cpp}    Any image → multi-size PNG-in-ICO (rich-media stage 1)
│   │   ├── com_menu_probe.{h,cpp} IContextMenu COM extension dynamic-menu probing
│   │   ├── win11menu.{h,cpp}     Win11 modern menu (PackagedCom / SystemCommandStore)
│   │   ├── winx_menu.{h,cpp}     Win+X menu (filesystem .lnk + hash)
│   │   ├── sendto_menu.{h,cpp}   Send To submenu
│   │   ├── shellnew_menu.{h,cpp} New submenu
│   │   ├── dragdrop_menu.{h,cpp} Drag-drop handlers
│   │   ├── openwith_menu.{h,cpp} Open with
│   │   ├── custom_menu.{h,cpp}   Custom right-click items
│   │   └── filetype_menu.{h,cpp} Manage file-type right-click items by extension
│   └── shellext/               Rich-media shell extension DLL (stage 2, standalone, doesn't link cmm_core)
│       ├── shellext.cpp          IContextMenu in-process COM server + owner-draw thumbnails
│       └── shellext.def          COM four exports
├── third_party/                Single-header webview lib + WebView2 SDK
└── ui/
    ├── index.html              Single-file Vue3 app (liquid-glass skin)
    ├── i18n.js                 16-language resources (add a language by editing just this file)
    ├── vue.global.js           Vue 3 runtime (local)
    └── liquidGlass.js          Liquid-glass helper
```

---

## Privacy & security

- **Fully local**: no network, uploads nothing.
- **Requires administrator**: third-party software's menu items are often registered in `HKLM` (all users), and their disable markers usually live in `HKLM` too, which pure user-mode can't touch. So the app requests administrator rights at launch to read/write the menu items' real registry locations, so that "all items can be toggled / renamed".
- **Auto-backup before write**: before each registry change it `reg export`s the affected key to local `%LOCALAPPDATA%\ContextMenuMaster\backups\`.
- **Reversible**: classic-menu takeover restores by deleting the key any time; toggling works via "add a disable marker / move to a disabled mirror" and can be moved back as-is; deleted items always go to the Recycle Bin. Never damages system files.

---

## Download

- Prebuilt versions are on [Releases](https://github.com/Henglie/ContextMenuMaster/releases): pick the zip by CPU architecture (x64 / x86 / arm64), extract, and double-click `ContextMenuMaster.exe` — no install needed. Most PCs want x64.
- Requires the WebView2 runtime (built into Windows 10/11).

---

## License & author

- **Author**: 恒烈 (EternalBlaze / Henglie)
- **Version**: v0.6
- **Project homepage**: <https://github.com/Henglie/ContextMenuMaster>
- **License**: MIT
