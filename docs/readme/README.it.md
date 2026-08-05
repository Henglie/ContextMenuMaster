<div align="center">

<img src="../../res/app-256.png" width="128" height="128" alt="Context Menu Master" />

# Gestore del menu contestuale Context Menu Master

**Riprendi il controllo del menu di scelta rapida su Windows 11** · Interfaccia liquid-glass · Completamente locale e reversibile

[![Release](https://img.shields.io/badge/release-v0.6-4d9fff?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-6a5ae0?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![License](https://img.shields.io/badge/license-MIT-22c55e?style=flat-square)](LICENSE)
[![Arch](https://img.shields.io/badge/arch-x64%20%7C%20x86%20%7C%20arm64-f59e0b?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)

<sub>Architettura Core-Shell · core C++17 · frontend WebView2 · interfaccia in 16 lingue</sub>

</div>

---

[简体中文](../../README.md) | [繁體中文](README.zh-TW.md) | [English](../../README.en.md) | [日本語](README.ja.md) | [한국어](README.ko.md) | [Français](README.fr.md) | [Deutsch](README.de.md) | [Español](README.es.md) | [Português](README.pt.md) | [Русский](README.ru.md) | **Italiano** | [العربية](README.ar.md) | [עברית](README.he.md) | [हिन्दी](README.hi.md) | [ไทย](README.th.md) | [Tiếng Việt](README.vi.md)

> Riprendi il controllo del menu di scelta rapida su Windows 11.

**Gestore del menu contestuale** è uno strumento di produttività per il desktop di Windows. Rimette nelle tue mani il menu di scelta rapida caotico, compresso e riempito da ogni sorta di software: rinomina voci e cambia icone in modo visivo, gestisci in blocco con caselle di controllo, recupera con un clic il menu classico nascosto di Windows 11, coprendo tutti gli scenari del menu contestuale — desktop / file / cartella / unità / Cestino / Win+X / Invia a / Nuovo / trascinamento.

Funzionamento puramente locale, backup automatico del Registro di sistema prima di ogni operazione di scrittura, tutte le operazioni sono reversibili in qualsiasi momento. Esegui come amministratore per gestire le voci di menu nell'ambito del sistema e di tutti gli utenti (HKLM).

> [!WARNING]
> **L'estensione shell rich-media (`CmmShellExt.dll`) è attualmente sperimentale.** È un'estensione shell COM in-process che ha effetto solo dopo averla registrata manualmente come amministratore con `regsvr32 CmmShellExt.dll`; una volta registrata, facendo clic con il tasto destro su un file compaiono voci di menu con miniature reali. Questa parte non è stata ampiamente validata su macchine reali e potrebbe comportarsi in modo instabile — o non apparire affatto — su alcuni sistemi, tipi di file o ambienti con software di sicurezza. Include delle protezioni (gli archivi oltre 200MB o l'enumerazione oltre 800ms vengono automaticamente ridotti a una voce di menu semplice, così il menu non si blocca mai), ma non è comunque consigliabile lasciarla registrata a lungo termine in ambienti importanti. Le funzioni di gestione del menu dell'app principale non dipendono da essa e possono essere usate autonomamente.

---

## Schermate

> La prima schermata è la vista visiva: la barra laterale sinistra dispone tutti gli scenari del menu contestuale, e al centro sono mostrati fianco a fianco «schema di sola lettura del menu moderno → menu classico modificabile».

![Finestra principale della vista visiva](../screenshots/main-visual.png)

<table>
  <tr>
    <td width="50%"><img src="../screenshots/list-view.png" alt="Vista elenco" /><br/><sub>Vista elenco: selezione in blocco con caselle di controllo, interruttori di attivazione/disattivazione, barra della magia in blocco</sub></td>
    <td width="50%"><img src="../screenshots/win11-zone.png" alt="Sezione Windows 11" /><br/><sub>Sezione Windows 11: scambio con un clic tra menu classico/nuovo, riavvio elegante di Explorer</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="../screenshots/about.png" alt="Scenario Nuovo (ShellNew)" /><br/><sub>Nuovo (ShellNew): gestisci quali tipi di file appaiono nel sottomenu «Nuovo»</sub></td>
    <td width="50%"><img src="../screenshots/desktop-visual.png" alt="Vista visiva del desktop vuoto" /><br/><sub>Desktop vuoto: la vista visiva simula il menu contestuale in scala 1:1</sub></td>
  </tr>
  <tr>
    <td colspan="2" align="center"><img src="../screenshots/shellnew.png" alt="Pagina Informazioni" width="50%" /><br/><sub>Informazioni: versione / licenza / autore e pagina iniziale del progetto</sub></td>
  </tr>
</table>

---

## Quale problema risolve

| Scenario | Senza questo strumento | Con questo strumento |
|---|---|---|
| Il menu contestuale di Win11 è troppo essenziale, le voci frequenti sono compresse in «Mostra altre opzioni» | Ogni volta un clic e un'attesa in più | Con un clic torni al menu classico completo, effetto immediato |
| I software installati hanno infilato un mucchio di voci nel menu contestuale, disordinato e lungo | Puoi solo frugare manualmente nel Registro di sistema | Selezione in blocco nella vista elenco, gestione a colpo d'occhio |
| Vuoi dare a una voce di menu un nome o un'icona più gradevole | Devi capire la struttura del Registro di sistema per osare toccarlo | Un clic nella vista visiva |
| Vuoi contrassegnare in modo uniforme un gruppo di voci | Modificarle una a una manualmente | Magia in blocco: seleziona e aggiungi un suffisso con un clic |

---

## Funzionalità principali

- **Copertura di tutti gli scenari del menu contestuale**: un'unica barra laterale dispone in verticale tutti gli scenari del menu contestuale; un clic e passi da uno all'altro —
  - **Scenari del Registro di sistema**: desktop vuoto, file, cartella, area vuota della cartella, unità, Cestino, tutti gli oggetti file, Questo PC, raccolte.
  - **Menu Win+X**: gestisci quel menu avanzato «clic destro sul pulsante Start / Win+X» (visualizzazione per gruppi, attivazione/disattivazione, aggiunta di voci personalizzate, eliminazione).
  - **Invia a (SendTo)**: gestisci le voci del sottomenu «Invia a» del menu contestuale, con attivazione/disattivazione, aggiunta ed eliminazione.
  - **Nuovo (ShellNew)**: gestisci quali tipi di file appaiono nel sottomenu «Nuovo» del menu contestuale, con attivazione/disattivazione di ciascuna voce.
  - **Gestori di trascinamento (DragDrop)**: gestisci le estensioni del menu che compaiono quando trascini file con il tasto destro.
  - **Apri con (Open with)**: visualizza le app registrate come candidate per «Apri con» (sola lettura).
- **Sincronizzazione in tempo reale delle due viste**
  - **Vista visiva (WYSIWYG)**: simula in scala 1:1 il vero menu contestuale di Windows; clicca sul nome per rinominare, clicca sull'icona per cambiarla, **fai clic con il tasto destro su una voce di menu per aprire il menu delle azioni (rinomina / cambia icona / attiva o disattiva)** — ciò che vedi è ciò che ottieni.
  - **Vista elenco (List View)**: tabella tradizionale con selezione in blocco tramite caselle di controllo, interruttori di attivazione/disattivazione, pulsanti per rinominare e cambiare icona — comoda per gestire un gruppo di voci in una volta sola.
- **Magia in blocco**: seleziona diverse voci di menu e aggiungi loro tutte con un clic lo stesso suffisso (ad esempio « miao~», « ★»), anche personalizzabile.
- **Sezione Windows 11**
  - **Scambio del menu classico con un clic**: scrive un `InprocServer32` vuoto in HKCU per disabilitare le estensioni del nuovo menu, così Esplora file torna al menu classico di Win10; e riavvia elegantemente `explorer.exe`, il menu si applica all'istante, senza disconnettersi e riaccedere. Un altro clic e tutto è ripristinato.
  - **Estensioni del nuovo menu (in sviluppo)**: interfaccia riservata per scansionare e gestire le voci di estensione `IExplorerCommand` dei pacchetti sparsi (MSIX) di Win11.
- **Multilingue**: al primo avvio viene selezionata automaticamente in base alla lingua di visualizzazione del sistema (cinese semplificato/tradizionale, per tutto il resto sempre inglese); l'interfaccia è commutabile in qualsiasi momento.
- **Puramente locale e reversibile**: legge e scrive solo il Registro di sistema dell'utente corrente, senza connessione a Internet e senza caricamenti; eliminando la chiave si ripristinano i valori predefiniti del sistema.

---

## Guida rapida

1. Scarica lo zip dell'architettura corrispondente da [Releases](https://github.com/Henglie/ContextMenuMaster/releases) (per la maggior parte dei computer scegli x64) ed estrailo in una cartella qualsiasi.
2. Fai doppio clic su `ContextMenuMaster.exe` e nel prompt UAC che compare fai clic su «Sì» per concedere i privilegi di amministratore (servono per leggere e scrivere le voci di menu che i software di terze parti registrano in `HKLM`; senza elevazione puoi modificare solo le voci dell'utente corrente).
3. Nessuna installazione, nessuna connessione. Al primo avvio la lingua dell'interfaccia viene selezionata automaticamente in base alla lingua del sistema; in alto a destra puoi passare in qualsiasi momento a cinese semplificato / tradizionale / English.

---

## Come si usa

### 1 · Scegli lo scenario

La barra laterale sinistra dispone in verticale tutti gli scenari del menu contestuale, divisi in tre gruppi:

- **Scenari frequenti**: desktop vuoto, file, cartella, area vuota della cartella, unità, Cestino, tutti gli oggetti file, Questo PC, raccolte — corrispondono ai menu che vedi facendo clic con il tasto destro su oggetti diversi.
- **Sottomenu**: Invia a, Nuovo, trascinamento, Apri con, tipi di file — sottomenu del menu contestuale e gestione per estensione.
- **Accessi rapidi**: Win+X (il menu avanzato del pulsante Start richiamato con il tasto destro).

In alto ci sono altri due accessi separati: **Sezione Windows 11** e **Rich Media**. Clicca su un qualsiasi scenario e verranno scansionate le sue voci di menu reali; il badge sull'etichetta mostra il numero di voci.

### 2 · Modifica il menu

Due viste, commutabili in alto a destra, con dati sincronizzati in tempo reale:

- **Vista visiva**: simula in scala 1:1 il menu contestuale di Windows.
  - Clicca sul **nome** di una voce di menu per rinominarla direttamente; clicca sull'**icona** per cambiarla.
  - **Fai clic con il tasto destro** su una qualsiasi voce di menu per aprire il menu delle azioni: rinomina / cambia icona / attiva o disattiva / elimina (per le voci create da te).
  - Il lato del menu moderno è uno schema di sola lettura (il menu moderno di Win11 non è controllato dal Registro di sistema, vedi la nota sotto); il lato del menu classico è modificabile.
- **Vista elenco**: tabella tradizionale, comoda per le operazioni in blocco.
  - Ogni riga ha un interruttore di attivazione/disattivazione e i pulsanti per rinominare / cambiare icona.
  - Dopo aver selezionato più righe, usa la **magia in blocco** per aggiungere loro con un clic un suffisso uniforme (ad esempio « ★»), anche personalizzabile.

Rinominare / cambiare icona / attivare o disattivare le voci integrate del sistema richiede una doppia conferma; prima di ogni scrittura nel Registro di sistema viene eseguito automaticamente un backup in `%LOCALAPPDATA%\ContextMenuMaster\backups\`, così puoi ripristinare in caso di problemi.

### 3 · Recupera il menu classico (Windows 11)

Entra nella **Sezione Windows 11** e fai clic su «Passa al menu classico»: il programma disabiliterà le estensioni del nuovo menu e riavvierà elegantemente `explorer.exe`; il menu contestuale torna all'istante allo stile completo di Win10, senza disconnettersi e riaccedere. Un altro clic e il nuovo menu è ripristinato.

### 4 · Crea le tue voci di menu

Nella barra degli strumenti della vista elenco fai clic su «Nuova voce di menu» e compila il nome visualizzato, il tipo di esecuzione (esegui programma / apri cartella / apri URL / riga di comando personalizzata), il percorso di destinazione, l'icona (facoltativa, supporta qualsiasi immagine con conversione automatica in ICO) e la posizione (in alto / in basso). Le voci create da te hanno un contrassegno dedicato e possono essere modificate o eliminate in qualsiasi momento; l'eliminazione rimuove solo le voci create da questo strumento e non tocca le associazioni di sistema.

> **Sul menu moderno di Win11**: il nuovo menu contestuale di Windows 11 accetta solo estensioni `IExplorerCommand` con identità di pacchetto (MSIX / pacchetto sparso); le voci di menu tradizionali del Registro di sistema vengono tutte compresse nel menu classico all'interno di «Mostra altre opzioni» — è una scelta di progettazione del sistema, non legata ai permessi. Perciò questo strumento gestisce il livello del menu classico; per riportare le voci frequenti al livello superiore, usa «Recupera il menu classico» qui sopra.

---

## Multilingue

Il backend in C++ usa le Win32 API per leggere la lingua di visualizzazione predefinita del sistema e la inietta nel frontend prima della creazione della pagina WebView2:

- Sistema in cinese (zh-CN / zh-TW / zh-HK ecc.) → l'interfaccia viene renderizzata in cinese, semplificato o tradizionale.
- Sistema in qualsiasi altra lingua → sempre ripiego sull'inglese (en).

In alto a destra nell'interfaccia puoi passare manualmente in qualsiasi momento a cinese semplificato / tradizionale / English; la scelta viene salvata nella memoria locale.

---

## Stack tecnologico e architettura

| Livello | Scelta | Descrizione |
|---|---|---|
| Motore backend | C++17 moderno / MSVC | Chiamate native dirette alle Win32 API |
| Livello di presentazione | Microsoft Edge WebView2 | La libreria a singolo header webview ospita il WebView2 integrato nel sistema |
| Ponte di comunicazione | WebMessage / postMessage | `w.bind` espone le funzioni del backend, il frontend chiama con `await window.fn()` |
| Interfaccia | Vue 3 (locale) | Applicazione a file singolo |
| Sistema visivo | FairyGlass | Sistema di design «vetro liquido» in tono scuro blu tenue, token replicati inline in `ui/index.html` |
| Build | CMake + Ninja | Riconosciuto da «Apri cartella» di VS2022 |

**Architettura: Core-Shell (kernel universale + shell nativa)**, come nel progetto gemello FairySave: `src/platform/windows` è il livello privilegiato Win32 (rilevamento i18n, gestione del menu del Registro di sistema), e `ui/` è il livello di presentazione.

**Compatibilità**: Windows 10 / 11 (la gestione del menu classico è una caratteristica di Win11). Richiede il runtime WebView2 (integrato in Win10/11).

---

## Sviluppo e build

**Ambiente**: VS2022 (con carico di lavoro C++ per desktop, CMake, Ninja e Windows SDK) + runtime WebView2.

**Metodo più semplice**: in VS2022 scegli «Apri cartella locale» e seleziona questa cartella; leggerà automaticamente `CMakePresets.json`, la riconoscerà come progetto CMake, e potrai scegliere il preset `windows-debug` o `windows-release` per generare ed eseguire.

**Metodo da riga di comando** (nell'ambiente vcvars64):

```bat
cmake --preset windows-release
cmake --build build/release
```

Prodotti:

- `build/release/bin/ContextMenuMaster.exe` — programma principale con GUI (sottosistema WIN32, senza console).
- `build/release/bin/CmmCli.exe` — sonda di debug da riga di comando (per verificare il backend, non destinata all'utente finale).

> Dopo aver modificato `ui/`, se hai toccato solo l'HTML, devi sincronizzare manualmente `ui/` nella cartella `ui/` accanto all'exe (il POST_BUILD di CMake si attiva solo al rilink).

---

## Struttura delle cartelle

```
ContextMenuMaster/
├── CMakeLists.txt              Configurazione principale CMake (≈ .sln, con /utf-8 /EHsc + alleggerimento Release)
├── CMakePresets.json           Riconoscimento con un clic in VS (windows-debug / release / release-x86 / release-arm64)
├── src/
│   ├── main.cpp                Punto di ingresso di debug CLI (CmmCli)
│   ├── app/
│   │   ├── gui_main.cpp        Punto di ingresso GUI WinMain: finestra webview + bind + iniezione della lingua
│   │   └── app.manifest        Manifest di elevazione UAC requireAdministrator
│   ├── platform/windows/       Livello privilegiato Win32 (un modulo per ogni scenario del menu contestuale)
│   │   ├── i18n.{h,cpp}          Rilevamento della lingua del sistema (zh-CN / zh-TW / en)
│   │   ├── context_menu.{h,cpp}  Gestione del menu classico↔nuovo di Win11 + riavvio elegante di Explorer
│   │   ├── menu_scanner.{h,cpp}  Scansione degli scenari del Registro di sistema + rinomina/icona/attivazione-disattivazione/suffisso in blocco
│   │   ├── icon_extract.{h,cpp}  Icona di sistema/risorsa/.ico → PNG base64 data URI
│   │   ├── image_icon.{h,cpp}    Qualsiasi immagine → PNG-in-ICO multidimensione (fase 1 rich media)
│   │   ├── com_menu_probe.{h,cpp} Sondaggio dinamico del menu delle estensioni COM IContextMenu
│   │   ├── win11menu.{h,cpp}     Menu moderno di Win11 (PackagedCom / SystemCommandStore)
│   │   ├── winx_menu.{h,cpp}     Menu Win+X (file system .lnk + hash)
│   │   ├── sendto_menu.{h,cpp}   Sottomenu Invia a
│   │   ├── shellnew_menu.{h,cpp} Sottomenu Nuovo
│   │   ├── dragdrop_menu.{h,cpp} Gestori di trascinamento
│   │   ├── openwith_menu.{h,cpp} Apri con
│   │   ├── custom_menu.{h,cpp}   Voci di menu contestuale create dall'utente
│   │   └── filetype_menu.{h,cpp} Gestione delle voci di menu contestuale per tipo di file/estensione
│   └── shellext/               DLL dell'estensione shell rich media (fase 2, indipendente, non collega cmm_core)
│       ├── shellext.cpp          Server COM in-process IContextMenu + miniatura owner-draw
│       └── shellext.def          Quattro esportazioni COM
├── third_party/                Libreria a singolo header webview + WebView2 SDK
└── ui/
    ├── index.html              Applicazione Vue3 a file singolo (skin «vetro liquido» + i18n in tre lingue)
    ├── vue.global.js           Runtime di Vue 3 (locale)
    └── liquidGlass.js          Helper «vetro liquido»
```

---

## Privacy e sicurezza

- **Puramente locale**: nessuna connessione a Internet e nessun caricamento di alcun contenuto.
- **Richiede privilegi di amministratore**: le voci di menu dei software di terze parti sono spesso registrate in `HKLM` (tutti gli utenti), e anche i loro contrassegni di disattivazione risiedono per lo più in `HKLM`, non modificabili in modalità utente. Perciò all'avvio il programma richiede i privilegi di amministratore per leggere e scrivere direttamente nella posizione reale delle voci di menu nel Registro di sistema, così da supportare «attivare/disattivare e rinominare tutte le voci».
- **Backup automatico prima della scrittura**: prima di ogni modifica al Registro di sistema viene eseguito prima un `reg export` della chiave corrispondente nel locale `%LOCALAPPDATA%\ContextMenuMaster\backups\`.
- **Reversibile**: la gestione del menu classico si ripristina in qualsiasi momento eliminando la chiave; l'attivazione/disattivazione passa per «aggiungere-eliminare contrassegno / spostare nell'immagine dei disattivati» e può essere riportata com'era; le voci eliminate vengono sempre inviate al Cestino. Non danneggia i file di sistema.

---

## Download

- Per le versioni precompilate vedi [Releases](https://github.com/Henglie/ContextMenuMaster/releases): scegli lo zip in base all'architettura della CPU (x64 / x86 / arm64), estrailo e fai doppio clic su `ContextMenuMaster.exe` — pronto all'uso, senza installazione. Per la stragrande maggioranza dei computer scegli x64.
- Richiede il runtime WebView2 (integrato in Windows 10/11).

---

## Licenza e autore

- **Autore**: 恒烈 (EternalBlaze / Henglie)
- **Versione**: v0.6
- **Pagina iniziale del progetto**: <https://github.com/Henglie/ContextMenuMaster>
- **Licenza**: MIT
