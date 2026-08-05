<div align="center">

<img src="../../res/app-256.png" width="128" height="128" alt="Context Menu Master" />

# Gestionnaire du menu contextuel Context Menu Master

**Reprenez le contrôle de votre menu contextuel sous Windows 11** · Interface en verre liquide · 100 % local, entièrement réversible

[![Release](https://img.shields.io/badge/release-v0.6-4d9fff?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-6a5ae0?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![License](https://img.shields.io/badge/license-MIT-22c55e?style=flat-square)](LICENSE)
[![Arch](https://img.shields.io/badge/arch-x64%20%7C%20x86%20%7C%20arm64-f59e0b?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)

<sub>Architecture Core-Shell · noyau C++17 · frontend WebView2 · interface en 16 langues</sub>

</div>

---

[简体中文](../../README.md) | [繁體中文](README.zh-TW.md) | [English](../../README.en.md) | [日本語](README.ja.md) | [한국어](README.ko.md) | **Français** | [Deutsch](README.de.md) | [Español](README.es.md) | [Português](README.pt.md) | [Русский](README.ru.md) | [Italiano](README.it.md) | [العربية](README.ar.md) | [עברית](README.he.md) | [हिन्दी](README.hi.md) | [ไทย](README.th.md) | [Tiếng Việt](README.vi.md)

> Reprenez la main sur votre menu contextuel dans Windows 11.

**Gestionnaire du menu contextuel** est un outil de productivité pour le bureau Windows. Il vous rend le contrôle du menu contextuel devenu confus, replié et encombré par toutes sortes de logiciels : renommez et changez les icônes de façon visuelle, gérez par sélection multiple, retrouvez d'un clic le menu classique masqué par Windows 11, et couvrez toutes les situations du clic droit — Bureau / Fichier / Dossier / Lecteur / Corbeille / Win+X / Envoyer vers / Nouveau / glisser-déposer.

Exécution 100 % locale, sauvegarde automatique du Registre avant toute opération d'écriture, toutes les opérations sont réversibles à tout moment. Exécutez en tant qu'administrateur pour gérer les éléments de menu à l'échelle du système et de tous les utilisateurs (HKLM).

> [!WARNING]
> **L'extension shell rich-média (`CmmShellExt.dll`) est actuellement expérimentale.** Il s'agit d'une extension shell COM in-process qui ne prend effet qu'après l'avoir enregistrée manuellement en tant qu'administrateur avec `regsvr32 CmmShellExt.dll` ; une fois enregistrée, un clic droit sur un fichier affiche des éléments de menu avec de vraies miniatures. Cette partie n'a pas encore été largement validée sur des machines réelles et peut se comporter de manière instable — voire ne pas apparaître du tout — sur certains systèmes, types de fichiers ou environnements dotés de logiciels de sécurité. Elle est dotée de garde-fous (les archives de plus de 200MB ou une énumération de plus de 800ms sont automatiquement rétrogradées en un simple élément de menu, de sorte que le menu ne se fige jamais), mais il reste déconseillé de la laisser enregistrée à long terme dans des environnements importants. Les fonctions de gestion de menu de l'application principale n'en dépendent pas et peuvent être utilisées seules.

---

## Captures d'écran

> Le premier écran est la vue visuelle : la barre latérale gauche liste toutes les situations du clic droit, le centre présente côte à côte « aperçu en lecture seule du menu moderne → menu classique modifiable ».

![Interface principale de la vue visuelle](../screenshots/main-visual.png)

<table>
  <tr>
    <td width="50%"><img src="../screenshots/list-view.png" alt="Vue liste" /><br/><sub>Vue liste : sélection multiple par cases à cocher, interrupteur activer/désactiver, barre d'outils de magie par lot</sub></td>
    <td width="50%"><img src="../screenshots/win11-zone.png" alt="Espace Windows 11" /><br/><sub>Espace Windows 11 : basculement en un clic entre menu classique/moderne, redémarrage élégant de l'Explorateur</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="../screenshots/about.png" alt="Situation Nouveau ShellNew" /><br/><sub>Nouveau (ShellNew) : gérer quels types de fichiers apparaissent dans le sous-menu « Nouveau »</sub></td>
    <td width="50%"><img src="../screenshots/desktop-visual.png" alt="Visualisation du bureau vide" /><br/><sub>Bureau vide : la vue visuelle reproduit le menu contextuel au 1:1</sub></td>
  </tr>
  <tr>
    <td colspan="2" align="center"><img src="../screenshots/shellnew.png" alt="Page À propos" width="50%" /><br/><sub>À propos : version / licence / auteur et page d'accueil du projet</sub></td>
  </tr>
</table>

---

## Quels problèmes résout-il

| Situation | Sans cet outil | Avec cet outil |
|---|---|---|
| Le menu contextuel de Win11 est trop épuré, les éléments courants sont repliés dans « Afficher plus d'options » | Un clic de plus, une attente de plus à chaque fois | Bascule en un clic vers le menu classique complet, effet immédiat |
| Les logiciels installés ont bourré le menu contextuel d'une multitude d'éléments, confus et interminable | On ne peut que fouiller le Registre à la main | Sélection multiple dans la vue liste, gestion d'un coup d'œil |
| On veut donner un nom ou une icône plus agréable à un élément de menu | Il faut comprendre la structure du Registre pour oser y toucher | Un clic dans la vue visuelle et c'est modifié |
| On veut ajouter uniformément une marque à un lot d'éléments de menu | Modifier un par un à la main | Magie par lot, sélectionnez puis ajoutez un suffixe en un clic |

---

## Fonctionnalités clés

- **Couverture de toutes les situations du clic droit** : une seule barre latérale liste verticalement toutes les situations du clic droit, un clic pour basculer ——
  - **Situations du Registre** : bureau vide, fichier, dossier, dossier vide, lecteur, Corbeille, tous les objets fichier, Ce PC, bibliothèques.
  - **Menu Win+X** : gérer le menu avancé « clic droit sur le bouton Démarrer / Win+X » (affichage par groupe, activer/désactiver, ajout d'éléments personnalisés, suppression).
  - **Envoyer vers (SendTo)** : gérer les éléments du sous-menu « Envoyer vers » du clic droit, avec activer/désactiver, ajouter, supprimer.
  - **Nouveau (ShellNew)** : gérer quels types de fichiers apparaissent dans le sous-menu « Nouveau » du clic droit, activer/désactiver par élément.
  - **Gestionnaire de glisser-déposer (DragDrop)** : gérer les extensions de menu qui apparaissent lors du glisser-déposer de fichiers par clic droit.
  - **Ouvrir avec (Open with)** : consulter les applications enregistrées comme candidates à « Ouvrir avec » (lecture seule).
- **Synchronisation en temps réel des deux vues**
  - **Vue visuelle (WYSIWYG)** : simule au 1:1 le vrai menu contextuel de Windows, cliquez sur le nom pour renommer, cliquez sur l'icône pour la changer, **un clic droit sur un élément de menu fait apparaître un menu d'actions (renommer / changer l'icône / activer désactiver)**, ce que vous voyez est ce que vous obtenez.
  - **Vue liste (List View)** : tableau traditionnel, sélection multiple par cases à cocher, interrupteur activer/désactiver, boutons renommer et changer l'icône tous présents, idéale pour gérer un lot d'éléments de menu à la fois.
- **Magie par lot** : sélectionnez plusieurs éléments de menu, ajoutez-leur à tous en un clic le même suffixe (par ex. « miaou~ », « ★ »), personnalisable aussi.
- **Espace Windows 11**
  - **Bascule en un clic du menu classique** : écrit un `InprocServer32` vide dans HKCU pour désactiver l'extension du menu moderne, l'Explorateur de fichiers revient au menu classique de Win10 ; et redémarre élégamment `explorer.exe`, le menu prend effet instantanément, sans déconnexion ni reconnexion. Un autre clic pour restaurer.
  - **Extension du menu moderne (en développement)** : interface réservée pour analyser et prendre en charge les éléments d'extension `IExplorerCommand` en package fragmenté (MSIX) de Win11.
- **Multilingue** : au premier lancement, sélection automatique selon la langue d'affichage du système (chinois simplifié/traditionnel, tout le reste en anglais), l'interface peut être changée à tout moment.
- **100 % local, réversible** : lit et écrit uniquement le Registre de l'utilisateur actuel, ne se connecte pas à Internet, ne téléverse rien, supprimer la clé restaure les valeurs par défaut du système.

---

## Prise en main rapide

1. Téléchargez depuis [Releases](https://github.com/Henglie/ContextMenuMaster/releases) le zip correspondant à votre architecture (la plupart des PC choisissent x64), décompressez dans un dossier quelconque.
2. Double-cliquez sur `ContextMenuMaster.exe`, dans l'invite UAC qui apparaît cliquez sur « Oui » pour accorder les droits d'administrateur (pour lire et écrire les éléments de menu que les logiciels tiers inscrivent dans `HKLM` ; sans élévation, vous ne pouvez modifier que les éléments de l'utilisateur actuel).
3. Aucune installation, aucune connexion Internet requises. Au premier lancement, la langue de l'interface est choisie automatiquement selon la langue du système, en haut à droite vous pouvez basculer à tout moment entre simplifié / traditionnel / English.

---

## Mode d'emploi

### 1 · Choisir la situation

La barre latérale gauche liste verticalement toutes les situations du clic droit, réparties en trois groupes :

- **Situations courantes** : bureau vide, fichier, dossier, dossier vide, lecteur, Corbeille, tous les objets fichier, Ce PC, bibliothèques —— correspondent aux menus que vous voyez en cliquant droit sur différents objets.
- **Sous-menus** : Envoyer vers, Nouveau, glisser-déposer, Ouvrir avec, types de fichiers —— les sous-menus du menu contextuel et la gestion par extension.
- **Entrée rapide** : Win+X (le menu avancé du clic droit sur le bouton Démarrer).

En haut se trouvent en outre deux entrées indépendantes : **Espace Windows 11** et **Média enrichi**. Cliquez sur n'importe quelle situation pour analyser ses vrais éléments de menu, le badge de l'onglet affiche le nombre d'éléments.

### 2 · Modifier le menu

Deux vues, à basculer en haut à droite, les données se synchronisent en temps réel :

- **Vue visuelle** : reproduit au 1:1 le menu contextuel de Windows.
  - Cliquez sur le **nom** d'un élément de menu pour le renommer directement ; cliquez sur l'**icône** pour la changer.
  - **Cliquez droit** sur n'importe quel élément de menu pour faire apparaître un menu d'actions : renommer / changer l'icône / activer désactiver / supprimer (éléments créés soi-même).
  - Le côté menu moderne est un aperçu en lecture seule (le menu moderne de Win11 n'est pas contrôlé par le Registre, voir l'explication ci-dessous), le côté menu classique est modifiable.
- **Vue liste** : tableau traditionnel, idéal pour les opérations par lot.
  - Chaque ligne dispose d'un interrupteur activer/désactiver et de boutons renommer / changer l'icône.
  - Après avoir coché plusieurs lignes, utilisez la **magie par lot** pour leur ajouter en un clic un suffixe uniforme (par ex. « ★ »), personnalisable aussi.

Renommer / changer l'icône / activer-désactiver demandent une double confirmation pour les éléments intégrés au système ; avant chaque écriture dans le Registre, une sauvegarde automatique est faite dans `%LOCALAPPDATA%\ContextMenuMaster\backups\`, restaurable en cas de problème.

### 3 · Retrouver le menu classique (Windows 11)

Entrez dans l'**Espace Windows 11**, cliquez sur « Basculer vers le menu classique » : le programme désactive l'extension du menu moderne et redémarre élégamment `explorer.exe`, le menu contextuel revient instantanément au style complet de Win10, sans déconnexion ni reconnexion. Un autre clic restaure le menu moderne.

### 4 · Créer ses propres éléments de menu

Dans la barre d'outils de la vue liste, cliquez sur « Nouvel élément de menu », renseignez le nom affiché, le type d'exécution (exécuter un programme / ouvrir un dossier / ouvrir une URL / ligne de commande personnalisée), le chemin cible, l'icône (facultatif, prend en charge la conversion automatique de n'importe quelle image en ICO), la position (haut / bas). Les éléments créés soi-même portent une marque dédiée, modifiables ou supprimables à tout moment ; la suppression n'efface que les éléments créés par cet outil, sans toucher aux associations système.

> **À propos du menu moderne de Win11** : le nouveau menu contextuel de Windows 11 n'accepte que les extensions `IExplorerCommand` dotées d'un identifiant de package (MSIX / package fragmenté), les éléments de menu traditionnels du Registre sont tous repliés dans le menu classique de « Afficher plus d'options » —— c'est un choix de conception du système, sans rapport avec les droits. Ainsi, cet outil gère la couche du menu classique ; pour ramener les éléments courants au niveau supérieur, utilisez le « Retrouver le menu classique » ci-dessus.

---

## Multilingue

Le back-end C++ lit la langue d'affichage par défaut du système via l'API Win32, et l'injecte au front-end avant la création de la page WebView2 :

- Système en chinois (zh-CN / zh-TW / zh-HK, etc.) → rendu de l'interface en chinois selon simplifié/traditionnel.
- Système dans toute autre langue → repli systématique en anglais (en).

En haut à droite de l'interface, vous pouvez basculer manuellement à tout moment entre simplifié / traditionnel / English, le choix étant enregistré dans le stockage local.

---

## Pile technique et architecture

| Couche | Choix | Description |
|---|---|---|
| Moteur back-end | C++17 moderne / MSVC | Appel natif direct de l'API Win32 |
| Couche de présentation | Microsoft Edge WebView2 | la bibliothèque webview à en-tête unique héberge le WebView2 intégré au système |
| Pont de communication | WebMessage / postMessage | `w.bind` expose les fonctions back-end, le front-end appelle via `await window.fn()` |
| Interface | Vue 3 (local) | application monofichier |
| Système visuel | FairyGlass | système de design liquid glass sombre bleuté, jetons reproduits en ligne dans `ui/index.html` |
| Build | CMake + Ninja | reconnu par « Ouvrir un dossier » de VS2022 |

**Architecture : Core-Shell (noyau générique + coque native)**, identique au projet frère FairySave : `src/platform/windows` est la couche privilégiée Win32 (détection i18n, prise en charge des menus du Registre), `ui/` est la couche de présentation.

**Compatibilité** : Windows 10 / 11 (la prise en charge du menu classique est une fonctionnalité de Win11). Nécessite le runtime WebView2 (intégré à Win10/11).

---

## Développement et build

**Environnement** : VS2022 (avec la charge de travail C++ Desktop, CMake, Ninja, Windows SDK) + runtime WebView2.

**Méthode la plus simple** : dans VS2022 « Ouvrir un dossier local » sélectionnez ce répertoire, il lit automatiquement `CMakePresets.json` et le reconnaît comme projet CMake, choisissez le préréglage `windows-debug` ou `windows-release` pour générer et exécuter.

**Méthode en ligne de commande** (dans l'environnement vcvars64) :

```bat
cmake --preset windows-release
cmake --build build/release
```

Produits :

- `build/release/bin/ContextMenuMaster.exe` —— programme principal GUI (sous-système WIN32, sans console).
- `build/release/bin/CmmCli.exe` —— sonde de débogage en ligne de commande (pour valider le back-end, non destinée à l'utilisateur final).

> Après avoir modifié `ui/`, si seul le HTML a changé, il faut synchroniser manuellement `ui/` vers le répertoire `ui/` situé à côté de l'exe (le POST_BUILD de CMake ne se déclenche qu'à la réédition des liens).

---

## Structure des répertoires

```
ContextMenuMaster/
├── CMakeLists.txt              Configuration principale CMake (≈ .sln, avec /utf-8 /EHsc + allègement Release)
├── CMakePresets.json           Reconnaissance en un clic par VS (windows-debug / release / release-x86 / release-arm64)
├── src/
│   ├── main.cpp                Point d'entrée de débogage CLI (CmmCli)
│   ├── app/
│   │   ├── gui_main.cpp        Point d'entrée GUI WinMain : fenêtre webview + bind + injection de langue
│   │   └── app.manifest        Manifeste d'élévation UAC requireAdministrator
│   ├── platform/windows/       Couche privilégiée Win32 (un module par situation de clic droit)
│   │   ├── i18n.{h,cpp}          Détection de la langue système (zh-CN / zh-TW / en)
│   │   ├── context_menu.{h,cpp}  Prise en charge du menu classique↔moderne de Win11 + redémarrage élégant de l'Explorateur
│   │   ├── menu_scanner.{h,cpp}  Analyse des situations du Registre + renommer/icône/activer-désactiver/suffixe par lot
│   │   ├── icon_extract.{h,cpp}  Icône système/ressource/.ico → data URI PNG base64
│   │   ├── image_icon.{h,cpp}    Image quelconque → PNG-in-ICO multi-tailles (média enrichi phase 1)
│   │   ├── com_menu_probe.{h,cpp} Détection de menu dynamique d'extension COM IContextMenu
│   │   ├── win11menu.{h,cpp}     Menu moderne de Win11 (PackagedCom / SystemCommandStore)
│   │   ├── winx_menu.{h,cpp}     Menu Win+X (fichiers .lnk + hash)
│   │   ├── sendto_menu.{h,cpp}   Sous-menu Envoyer vers
│   │   ├── shellnew_menu.{h,cpp} Sous-menu Nouveau
│   │   ├── dragdrop_menu.{h,cpp} Gestionnaire de glisser-déposer
│   │   ├── openwith_menu.{h,cpp} Ouvrir avec
│   │   ├── custom_menu.{h,cpp}   Éléments de menu contextuel créés soi-même
│   │   └── filetype_menu.{h,cpp} Gestion des éléments de clic droit par type de fichier selon l'extension
│   └── shellext/               DLL d'extension Shell média enrichi (phase 2, indépendante, ne se lie pas à cmm_core)
│       ├── shellext.cpp          Serveur COM in-process IContextMenu + miniature owner-draw
│       └── shellext.def          Quatre exports COM
├── third_party/                Bibliothèque webview à en-tête unique + WebView2 SDK
└── ui/
    ├── index.html              Application Vue3 monofichier (thème liquid glass + i18n trois langues)
    ├── vue.global.js           Runtime Vue 3 (local)
    └── liquidGlass.js          Utilitaire liquid glass
```

---

## Confidentialité et sécurité

- **100 % local** : ne se connecte pas à Internet, ne téléverse aucun contenu.
- **Nécessite les droits d'administrateur** : les éléments de menu des logiciels tiers sont souvent inscrits dans `HKLM` (tous les utilisateurs), et leur marque de désactivation aussi le plus souvent dans `HKLM`, impossible à modifier en simple mode utilisateur. C'est pourquoi le programme demande les droits d'administrateur au démarrage, pour lire et écrire directement à l'emplacement réel du Registre où se trouvent les éléments de menu, afin de prendre en charge « tous les éléments peuvent être activés-désactivés / renommés ».
- **Sauvegarde automatique avant écriture** : avant chaque modification du Registre, un `reg export` sauvegarde d'abord la clé correspondante dans `%LOCALAPPDATA%\ContextMenuMaster\backups\` en local.
- **Réversible** : la prise en charge du menu classique se restaure à tout moment en supprimant la clé ; l'activation-désactivation passe par « ajout/suppression d'une marque de désactivation / déplacement vers un miroir désactivé », restaurable à l'identique ; la suppression d'éléments les envoie toujours à la Corbeille. N'endommage pas les fichiers système.

---

## Téléchargement

- Version précompilée voir [Releases](https://github.com/Henglie/ContextMenuMaster/releases) : choisissez le zip selon l'architecture CPU (x64 / x86 / arm64), après décompression double-cliquez sur `ContextMenuMaster.exe` pour l'utiliser directement, sans installation. La grande majorité des PC choisissent x64.
- Nécessite le runtime WebView2 (intégré à Windows 10/11).

---

## Licence et auteur

- **Auteur** : 恒烈 (EternalBlaze / Henglie)
- **Version** : v0.6
- **Page d'accueil du projet** : <https://github.com/Henglie/ContextMenuMaster>
- **Licence** : MIT
