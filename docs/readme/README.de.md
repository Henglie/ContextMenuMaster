<div align="center">

<img src="../../res/app-256.png" width="128" height="128" alt="Context Menu Master" />

# Kontextmenü-Verwalter Context Menu Master

**Übernehmen Sie unter Windows 11 wieder die Kontrolle über Ihr Kontextmenü** · Liquid-Glass-Oberfläche · Vollständig lokal, vollständig umkehrbar

[![Release](https://img.shields.io/badge/release-v0.6-4d9fff?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-6a5ae0?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![License](https://img.shields.io/badge/license-MIT-22c55e?style=flat-square)](LICENSE)
[![Arch](https://img.shields.io/badge/arch-x64%20%7C%20x86%20%7C%20arm64-f59e0b?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)

<sub>Core-Shell-Architektur · C++17-Kern · WebView2-Frontend · Oberfläche in 16 Sprachen</sub>

</div>

---

[简体中文](../../README.md) | [繁體中文](README.zh-TW.md) | [English](../../README.en.md) | [日本語](README.ja.md) | [한국어](README.ko.md) | [Français](README.fr.md) | **Deutsch** | [Español](README.es.md) | [Português](README.pt.md) | [Русский](README.ru.md) | [Italiano](README.it.md) | [العربية](README.ar.md) | [עברית](README.he.md) | [हिन्दी](README.hi.md) | [ไทย](README.th.md) | [Tiếng Việt](README.vi.md)

> Übernehmen Sie unter Windows 11 wieder die Kontrolle über Ihr Kontextmenü.

**Kontextmenü-Verwalter** ist ein Produktivitätswerkzeug für den Windows-Desktop. Es gibt Ihnen das unübersichtliche, zusammengeklappte und von allerlei Software vollgestopfte Kontextmenü zurück in die Hand: Namen und Symbole visuell ändern, per Mehrfachauswahl verwalten, das von Windows 11 versteckte klassische Menü mit einem Klick zurückholen und alle Kontextmenü-Szenarien abdecken — Desktop / Datei / Ordner / Laufwerk / Papierkorb / Win+X / Senden an / Neu / Drag & Drop.

Rein lokale Ausführung, automatische Sicherung der Registrierung vor jedem Schreibvorgang, alle Vorgänge sind jederzeit umkehrbar. Als Administrator ausführen, um Menüeinträge im System- und Alle-Benutzer-Bereich (HKLM) zu verwalten.

> [!WARNING]
> **Die Rich-Media-Shell-Erweiterung (`CmmShellExt.dll`) ist derzeit experimentell.** Es handelt sich um eine In-Process-COM-Shell-Erweiterung, die erst wirksam wird, nachdem Sie sie als Administrator manuell mit `regsvr32 CmmShellExt.dll` registriert haben; nach der Registrierung zeigt ein Rechtsklick auf eine Datei Menüeinträge mit echten Miniaturansichten an. Dieser Teil wurde noch nicht umfassend auf realen Geräten validiert und kann sich auf manchen Systemen, bei bestimmten Dateitypen oder in Umgebungen mit Sicherheitssoftware instabil verhalten oder gar nicht erscheinen. Er ist mit Schutzmechanismen ausgestattet (Archive über 200MB oder eine Enumeration über 800ms werden automatisch zu einem einfachen Menüeintrag herabgestuft, sodass das Menü nie einfriert), dennoch wird davon abgeraten, ihn in wichtigen Umgebungen dauerhaft registriert zu lassen. Die Menüverwaltungsfunktionen der Hauptanwendung hängen nicht davon ab und können eigenständig genutzt werden.

---

## Screenshots

> Der erste Bildschirm zeigt die visuelle Ansicht: Die linke Seitenleiste listet alle Kontextmenü-Szenarien auf, in der Mitte werden „schreibgeschützte Vorschau des modernen Menüs → bearbeitbares klassisches Menü" nebeneinander dargestellt.

![Hauptoberfläche der visuellen Ansicht](../screenshots/main-visual.png)

<table>
  <tr>
    <td width="50%"><img src="../screenshots/list-view.png" alt="Listenansicht" /><br/><sub>Listenansicht: Mehrfachauswahl per Kontrollkästchen, Ein/Aus-Schalter, Stapelzauber-Symbolleiste</sub></td>
    <td width="50%"><img src="../screenshots/win11-zone.png" alt="Windows-11-Bereich" /><br/><sub>Windows-11-Bereich: Umschalten zwischen klassischem/modernem Menü mit einem Klick, eleganter Neustart des Explorers</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="../screenshots/about.png" alt="Neu-Szenario ShellNew" /><br/><sub>Neu (ShellNew): verwalten, welche Dateitypen im Untermenü „Neu" erscheinen</sub></td>
    <td width="50%"><img src="../screenshots/desktop-visual.png" alt="Visualisierung des leeren Desktops" /><br/><sub>Leerer Desktop: die visuelle Ansicht bildet das Kontextmenü 1:1 nach</sub></td>
  </tr>
  <tr>
    <td colspan="2" align="center"><img src="../screenshots/shellnew.png" alt="Info-Seite" width="50%" /><br/><sub>Info: Version / Lizenz / Autor und Projektstartseite</sub></td>
  </tr>
</table>

---

## Welche Probleme es löst

| Szenario | Ohne dieses Werkzeug | Mit diesem Werkzeug |
|---|---|---|
| Das Kontextmenü von Win11 ist zu abgespeckt, häufige Einträge sind in „Weitere Optionen anzeigen" versteckt | Jedes Mal ein Klick mehr, eine Wartezeit mehr | Mit einem Klick zurück zum vollständigen klassischen Menü, sofort wirksam |
| Installierte Software hat das Kontextmenü mit einem Haufen Einträge vollgestopft, unübersichtlich und lang | Man kann nur manuell in der Registrierung wühlen | Mehrfachauswahl in der Listenansicht, auf einen Blick verwaltet |
| Man möchte einem Menüeintrag einen ansprechenderen Namen oder ein Symbol geben | Man muss die Registrierungsstruktur verstehen, um sich daran zu wagen | Ein Klick in der visuellen Ansicht und es ist geändert |
| Man möchte einer Reihe von Menüeinträgen einheitlich eine Markierung hinzufügen | Einen nach dem anderen manuell ändern | Stapelzauber, auswählen und mit einem Klick ein Suffix anhängen |

---

## Kernfunktionen

- **Abdeckung aller Kontextmenü-Szenarien**: eine einzige Seitenleiste listet alle Kontextmenü-Szenarien vertikal auf, ein Klick zum Umschalten ——
  - **Registrierungs-Szenarien**: leerer Desktop, Datei, Ordner, leerer Ordner, Laufwerk, Papierkorb, alle Dateiobjekte, Dieser PC, Bibliotheken.
  - **Win+X-Menü**: das erweiterte Menü „Rechtsklick auf Start-Schaltfläche / Win+X" verwalten (gruppierte Anzeige, Ein/Aus, benutzerdefinierte Einträge hinzufügen, löschen).
  - **Senden an (SendTo)**: die Einträge im Kontextmenü-Untermenü „Senden an" verwalten, mit Ein/Aus, Hinzufügen, Löschen.
  - **Neu (ShellNew)**: verwalten, welche Dateitypen im Kontextmenü-Untermenü „Neu" erscheinen, pro Eintrag ein/aus.
  - **Drag-&-Drop-Handler (DragDrop)**: die Menüerweiterungen verwalten, die beim Rechtsklick-Ziehen von Dateien erscheinen.
  - **Öffnen mit (Open with)**: die als „Öffnen mit"-Kandidaten registrierten Anwendungen anzeigen (schreibgeschützt).
- **Echtzeit-Synchronisation der beiden Ansichten**
  - **Visuelle Ansicht (WYSIWYG)**: simuliert das echte Windows-Kontextmenü 1:1, klicken Sie auf den Namen zum Umbenennen, auf das Symbol zum Ändern, **ein Rechtsklick auf einen Menüeintrag öffnet ein Aktionsmenü (umbenennen / Symbol ändern / aktivieren deaktivieren)**, was Sie sehen, ist was Sie bekommen.
  - **Listenansicht (List View)**: traditionelle Tabelle, Mehrfachauswahl per Kontrollkästchen, Ein/Aus-Schalter, Schaltflächen zum Umbenennen und Symbol ändern alle vorhanden, ideal zum gleichzeitigen Verwalten einer Reihe von Menüeinträgen.
- **Stapelzauber**: mehrere Menüeinträge auswählen, ihnen allen mit einem Klick dasselbe Suffix anhängen (z. B. „ miau~", „ ★"), auch anpassbar.
- **Windows-11-Bereich**
  - **Klassisches Menü mit einem Klick umschalten**: schreibt einen leeren `InprocServer32` in HKCU, um die Erweiterung des modernen Menüs zu deaktivieren, der Datei-Explorer fällt auf das klassische Menü von Win10 zurück; und startet `explorer.exe` elegant neu, das Menü wird sofort wirksam, ohne Ab- und Wiederanmelden. Ein weiterer Klick zum Wiederherstellen.
  - **Erweiterung des modernen Menüs (in Entwicklung)**: reservierte Schnittstelle zum Scannen und Übernehmen der `IExplorerCommand`-Sparse-Package-Erweiterungen (MSIX) von Win11.
- **Mehrsprachig**: beim ersten Start automatische Auswahl anhand der Anzeigesprache des Systems (Chinesisch vereinfacht/traditionell, alles andere durchweg Englisch), die Oberfläche kann jederzeit umgeschaltet werden.
- **Rein lokal, umkehrbar**: liest und schreibt nur die Registrierung des aktuellen Benutzers, keine Internetverbindung, kein Hochladen, das Löschen des Schlüssels stellt die Systemstandards wieder her.

---

## Schnellstart

1. Laden Sie von [Releases](https://github.com/Henglie/ContextMenuMaster/releases) das zur Architektur passende ZIP herunter (die meisten PCs wählen x64), entpacken Sie es in ein beliebiges Verzeichnis.
2. Doppelklicken Sie auf `ContextMenuMaster.exe`, klicken Sie in der erscheinenden UAC-Eingabeaufforderung auf „Ja", um Administratorrechte zu gewähren (zum Lesen und Schreiben der Menüeinträge, die Drittanbietersoftware in `HKLM` schreibt; ohne Rechteerweiterung können nur die Einträge des aktuellen Benutzers geändert werden).
3. Keine Installation, keine Internetverbindung erforderlich. Beim ersten Start wird die Oberflächensprache automatisch nach der Systemsprache gewählt, oben rechts können Sie jederzeit zwischen Vereinfacht / Traditionell / English umschalten.

---

## Bedienung

### 1 · Szenario wählen

Die linke Seitenleiste listet alle Kontextmenü-Szenarien vertikal auf, in drei Gruppen unterteilt:

- **Häufige Szenarien**: leerer Desktop, Datei, Ordner, leerer Ordner, Laufwerk, Papierkorb, alle Dateiobjekte, Dieser PC, Bibliotheken —— entsprechen den Menüs, die Sie beim Rechtsklick auf verschiedene Objekte sehen.
- **Untermenüs**: Senden an, Neu, Drag & Drop, Öffnen mit, Dateitypen —— die Untermenüs im Kontextmenü und die Verwaltung nach Dateierweiterung.
- **Schnellzugang**: Win+X (das erweiterte Menü beim Rechtsklick auf die Start-Schaltfläche).

Oben gibt es außerdem zwei eigenständige Zugänge: **Windows-11-Bereich** und **Rich Media**. Klicken Sie auf ein beliebiges Szenario, um dessen echte Menüeinträge zu scannen, das Abzeichen der Registerkarte zeigt die Anzahl der Einträge an.

### 2 · Menü bearbeiten

Zwei Ansichten, oben rechts umschaltbar, die Daten synchronisieren sich in Echtzeit:

- **Visuelle Ansicht**: bildet das Windows-Kontextmenü 1:1 nach.
  - Klicken Sie auf den **Namen** eines Menüeintrags, um ihn direkt umzubenennen; klicken Sie auf das **Symbol**, um es zu ändern.
  - **Rechtsklicken** Sie auf einen beliebigen Menüeintrag, um ein Aktionsmenü zu öffnen: umbenennen / Symbol ändern / aktivieren deaktivieren / löschen (selbst erstellte Einträge).
  - Die Seite des modernen Menüs ist eine schreibgeschützte Vorschau (das moderne Menü von Win11 wird nicht von der Registrierung gesteuert, siehe Erläuterung unten), die Seite des klassischen Menüs ist bearbeitbar.
- **Listenansicht**: traditionelle Tabelle, ideal für Stapelvorgänge.
  - Jede Zeile hat einen Ein/Aus-Schalter und Schaltflächen zum Umbenennen / Symbol ändern.
  - Nach dem Ankreuzen mehrerer Zeilen verwenden Sie den **Stapelzauber**, um ihnen mit einem Klick ein einheitliches Suffix anzuhängen (z. B. „ ★"), auch anpassbar.

Umbenennen / Symbol ändern / aktivieren-deaktivieren erfordern bei systemeigenen Einträgen eine zweite Bestätigung; vor jedem Schreibvorgang in die Registrierung wird automatisch nach `%LOCALAPPDATA%\ContextMenuMaster\backups\` gesichert, bei Problemen wiederherstellbar.

### 3 · Klassisches Menü zurückholen (Windows 11)

Gehen Sie in den **Windows-11-Bereich**, klicken Sie auf „Zum klassischen Menü umschalten": das Programm deaktiviert die Erweiterung des modernen Menüs und startet `explorer.exe` elegant neu, das Kontextmenü kehrt sofort zum vollständigen Win10-Stil zurück, ohne Ab- und Wiederanmelden. Ein weiterer Klick stellt das moderne Menü wieder her.

### 4 · Eigene Menüeinträge erstellen

Klicken Sie in der Symbolleiste der Listenansicht auf „Neuer Menüeintrag", geben Sie den Anzeigenamen, den Ausführungstyp (Programm ausführen / Ordner öffnen / URL öffnen / benutzerdefinierte Befehlszeile), den Zielpfad, das Symbol (optional, unterstützt die automatische Umwandlung beliebiger Bilder in ICO) und die Position (oben / unten) an. Selbst erstellte Einträge tragen eine eigene Markierung, jederzeit bearbeit- oder löschbar; das Löschen entfernt nur die von diesem Werkzeug erstellten Einträge, ohne die Systemzuordnungen zu berühren.

> **Zum modernen Menü von Win11**: das neue Kontextmenü von Windows 11 akzeptiert nur `IExplorerCommand`-Erweiterungen mit Paketkennung (MSIX / Sparse Package), traditionelle Registrierungs-Menüeinträge werden durchweg in das klassische Menü von „Weitere Optionen anzeigen" eingeklappt —— das ist Systemdesign, unabhängig von Rechten. Daher verwaltet dieses Werkzeug die Ebene des klassischen Menüs; um häufige Einträge auf die oberste Ebene zurückzuholen, nutzen Sie das obige „Klassisches Menü zurückholen".

---

## Mehrsprachig

Das C++-Backend liest die Standard-Anzeigesprache des Systems über die Win32-API und injiziert sie vor der Erstellung der WebView2-Seite in das Frontend:

- System auf Chinesisch (zh-CN / zh-TW / zh-HK usw.) → Oberfläche wird je nach vereinfacht/traditionell in Chinesisch gerendert.
- System in jeder anderen Sprache → durchweg Rückfall auf Englisch (en).

Oben rechts in der Oberfläche können Sie jederzeit manuell zwischen Vereinfacht / Traditionell / English umschalten, die Auswahl wird im lokalen Speicher festgehalten.

---

## Technologie-Stack und Architektur

| Ebene | Auswahl | Beschreibung |
|---|---|---|
| Backend-Engine | Modernes C++17 / MSVC | direkter nativer Aufruf der Win32-API |
| Präsentationsebene | Microsoft Edge WebView2 | die Single-Header-Bibliothek webview trägt das systemeigene WebView2 |
| Kommunikationsbrücke | WebMessage / postMessage | `w.bind` legt Backend-Funktionen offen, das Frontend ruft per `await window.fn()` auf |
| Oberfläche | Vue 3 (lokal) | Single-File-Anwendung |
| Visuelles System | FairyGlass | leicht bläuliches dunkles Liquid-Glass-Designsystem, Tokens inline nachgebildet in `ui/index.html` |
| Build | CMake + Ninja | von „Ordner öffnen" in VS2022 erkannt |

**Architektur: Core-Shell (generischer Kern + native Hülle)**, identisch mit dem Schwesterprojekt FairySave: `src/platform/windows` ist die privilegierte Win32-Ebene (i18n-Erkennung, Übernahme der Registrierungsmenüs), `ui/` ist die Präsentationsebene.

**Kompatibilität**: Windows 10 / 11 (die Übernahme des klassischen Menüs ist ein Win11-Merkmal). Erfordert die WebView2-Runtime (in Win10/11 enthalten).

---

## Entwicklung und Build

**Umgebung**: VS2022 (mit C++-Desktopworkload, CMake, Ninja, Windows SDK) + WebView2-Runtime.

**Einfachster Weg**: in VS2022 „Lokalen Ordner öffnen" dieses Verzeichnis auswählen, es liest automatisch `CMakePresets.json` und erkennt es als CMake-Projekt, wählen Sie das Preset `windows-debug` oder `windows-release` zum Generieren und Ausführen.

**Befehlszeilenweg** (in der vcvars64-Umgebung):

```bat
cmake --preset windows-release
cmake --build build/release
```

Produkte:

- `build/release/bin/ContextMenuMaster.exe` —— GUI-Hauptprogramm (WIN32-Subsystem, ohne Konsole).
- `build/release/bin/CmmCli.exe` —— Befehlszeilen-Debugsonde (zur Validierung des Backends, nicht für Endbenutzer bestimmt).

> Wenn nach dem Ändern von `ui/` nur das HTML geändert wurde, muss `ui/` manuell in das neben der exe liegende Verzeichnis `ui/` synchronisiert werden (das POST_BUILD von CMake wird nur beim erneuten Linken ausgelöst).

---

## Verzeichnisstruktur

```
ContextMenuMaster/
├── CMakeLists.txt              CMake-Hauptkonfiguration (≈ .sln, mit /utf-8 /EHsc + Release-Verschlankung)
├── CMakePresets.json           Ein-Klick-Erkennung durch VS (windows-debug / release / release-x86 / release-arm64)
├── src/
│   ├── main.cpp                CLI-Debug-Einstiegspunkt (CmmCli)
│   ├── app/
│   │   ├── gui_main.cpp        GUI-Einstiegspunkt WinMain: webview-Fenster + bind + Sprachinjektion
│   │   └── app.manifest        UAC-requireAdministrator-Rechteerweiterungsmanifest
│   ├── platform/windows/       Privilegierte Win32-Ebene (ein Modul pro Kontextmenü-Szenario)
│   │   ├── i18n.{h,cpp}          Systemspracherkennung (zh-CN / zh-TW / en)
│   │   ├── context_menu.{h,cpp}  Übernahme des klassischen↔modernen Menüs von Win11 + eleganter Neustart des Explorers
│   │   ├── menu_scanner.{h,cpp}  Scan der Registrierungs-Szenarien + Umbenennen/Symbol/Ein-Aus/Stapelsuffix
│   │   ├── icon_extract.{h,cpp}  Systemsymbol/Ressource/.ico → PNG-base64-Data-URI
│   │   ├── image_icon.{h,cpp}    Beliebiges Bild → PNG-in-ICO in mehreren Größen (Rich-Media-Phase 1)
│   │   ├── com_menu_probe.{h,cpp} Dynamische Menüerkennung der IContextMenu-COM-Erweiterung
│   │   ├── win11menu.{h,cpp}     Modernes Menü von Win11 (PackagedCom / SystemCommandStore)
│   │   ├── winx_menu.{h,cpp}     Win+X-Menü (Dateisystem .lnk + hash)
│   │   ├── sendto_menu.{h,cpp}   Untermenü Senden an
│   │   ├── shellnew_menu.{h,cpp} Untermenü Neu
│   │   ├── dragdrop_menu.{h,cpp} Drag-&-Drop-Handler
│   │   ├── openwith_menu.{h,cpp} Öffnen mit
│   │   ├── custom_menu.{h,cpp}   Selbst erstellte Kontextmenüeinträge
│   │   └── filetype_menu.{h,cpp} Verwaltung der Dateityp-Kontextmenüeinträge nach Erweiterung
│   └── shellext/               Rich-Media-Shell-Erweiterungs-DLL (Phase 2, eigenständig, ohne cmm_core-Verknüpfung)
│       ├── shellext.cpp          IContextMenu In-Process-COM-Server + Owner-Draw-Miniaturansicht
│       └── shellext.def          Vier COM-Exporte
├── third_party/                Single-Header-Bibliothek webview + WebView2 SDK
└── ui/
    ├── index.html              Single-File-Vue3-Anwendung (Liquid-Glass-Skin + dreisprachige i18n)
    ├── vue.global.js           Vue-3-Runtime (lokal)
    └── liquidGlass.js          Liquid-Glass-Hilfsmittel
```

---

## Datenschutz und Sicherheit

- **Rein lokal**: keine Internetverbindung, kein Hochladen jeglicher Inhalte.
- **Erfordert Administratorrechte**: die Menüeinträge von Drittanbietersoftware sind oft in `HKLM` (alle Benutzer) registriert, und ihre Deaktivierungsmarkierung meist ebenfalls in `HKLM`, rein im Benutzermodus nicht änderbar. Daher fordert das Programm beim Start Administratorrechte an, um direkt an den tatsächlichen Registrierungsort der Menüeinträge zu lesen und zu schreiben, um „alle Einträge lassen sich aktivieren-deaktivieren / umbenennen" zu unterstützen.
- **Automatische Sicherung vor dem Schreiben**: vor jeder Änderung der Registrierung sichert ein `reg export` zunächst den entsprechenden Schlüssel lokal nach `%LOCALAPPDATA%\ContextMenuMaster\backups\`.
- **Umkehrbar**: die Übernahme des klassischen Menüs lässt sich jederzeit durch Löschen des Schlüssels wiederherstellen; das Aktivieren-Deaktivieren läuft über „Hinzufügen/Löschen einer Deaktivierungsmarkierung / Verschieben in ein deaktiviertes Spiegelbild", unverändert zurückverschiebbar; das Löschen von Einträgen sendet sie stets in den Papierkorb. Beschädigt keine Systemdateien.

---

## Download

- Vorkompilierte Version siehe [Releases](https://github.com/Henglie/ContextMenuMaster/releases): wählen Sie das ZIP nach CPU-Architektur (x64 / x86 / arm64), doppelklicken Sie nach dem Entpacken auf `ContextMenuMaster.exe` zum direkten Verwenden, ohne Installation. Die überwiegende Mehrheit der PCs wählt x64.
- Erfordert die WebView2-Runtime (in Windows 10/11 enthalten).

---

## Lizenz und Autor

- **Autor**: 恒烈 (EternalBlaze / Henglie)
- **Version**: v0.6
- **Projektstartseite**: <https://github.com/Henglie/ContextMenuMaster>
- **Lizenz**: MIT
