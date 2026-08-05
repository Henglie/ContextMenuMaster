<div align="center">

<img src="../../res/app-256.png" width="128" height="128" alt="Context Menu Master" />

# Administrador del menú contextual Context Menu Master

**Vuelve a tomar el control de tu menú contextual en Windows 11** · Interfaz liquid-glass · Totalmente local y reversible

[![Release](https://img.shields.io/badge/release-v0.6-4d9fff?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-6a5ae0?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![License](https://img.shields.io/badge/license-MIT-22c55e?style=flat-square)](LICENSE)
[![Arch](https://img.shields.io/badge/arch-x64%20%7C%20x86%20%7C%20arm64-f59e0b?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)

<sub>Arquitectura Core-Shell · core en C++17 · frontend WebView2 · interfaz en 16 idiomas</sub>

</div>

---

[简体中文](../../README.md) | [繁體中文](README.zh-TW.md) | [English](../../README.en.md) | [日本語](README.ja.md) | [한국어](README.ko.md) | [Français](README.fr.md) | [Deutsch](README.de.md) | **Español** | [Português](README.pt.md) | [Русский](README.ru.md) | [Italiano](README.it.md) | [العربية](README.ar.md) | [עברית](README.he.md) | [हिन्दी](README.hi.md) | [ไทย](README.th.md) | [Tiếng Việt](README.vi.md)

> Vuelve a tomar el control de tu menú contextual en Windows 11.

**Administrador del menú contextual** es una herramienta de productividad para el escritorio de Windows. Devuelve a tus manos el menú contextual desordenado, plegado y saturado por todo tipo de software: cambia nombres e iconos de forma visual, administra por selección múltiple, recupera con un clic el menú clásico que Windows 11 esconde y abarca todos los escenarios del menú contextual: escritorio / archivo / carpeta / unidad / Papelera de reciclaje / Win+X / Enviar a / Nuevo / arrastrar y soltar.

Ejecución puramente local, copia de seguridad automática del Registro antes de cada operación de escritura, todas las operaciones son reversibles en cualquier momento. Ejecuta como administrador para administrar los elementos de menú del ámbito del sistema y de todos los usuarios (HKLM).

> [!WARNING]
> **La extensión de shell de contenido enriquecido (`CmmShellExt.dll`) es actualmente experimental.** Es una extensión de shell COM en proceso que solo surte efecto después de registrarla manualmente como administrador con `regsvr32 CmmShellExt.dll`; una vez registrada, al hacer clic derecho en un archivo se muestran elementos de menú con miniaturas reales. Esta parte no se ha validado ampliamente en máquinas reales y puede comportarse de forma inestable — o no aparecer en absoluto — en algunos sistemas, tipos de archivo o entornos con software de seguridad. Incluye salvaguardas (los archivos comprimidos de más de 200MB o la enumeración de más de 800ms se degradan automáticamente a un elemento de menú simple, de modo que el menú nunca se congela), pero aun así no se recomienda dejarla registrada a largo plazo en entornos importantes. Las funciones de administración de menús de la aplicación principal no dependen de ella y pueden usarse por sí solas.

---

## Capturas de pantalla

> La primera pantalla es la vista visual: la barra lateral izquierda enumera todos los escenarios del menú contextual y, en el centro, se presentan lado a lado «vista previa de solo lectura del menú moderno → menú clásico editable».

![Interfaz principal de la vista visual](../screenshots/main-visual.png)

<table>
  <tr>
    <td width="50%"><img src="../screenshots/list-view.png" alt="Vista de lista" /><br/><sub>Vista de lista: selección múltiple con casillas, interruptor de activar/desactivar, barra de herramientas de magia por lotes</sub></td>
    <td width="50%"><img src="../screenshots/win11-zone.png" alt="Zona de Windows 11" /><br/><sub>Zona de Windows 11: intercambio con un clic entre menú clásico/moderno, reinicio elegante del Explorador</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="../screenshots/about.png" alt="Escenario Nuevo ShellNew" /><br/><sub>Nuevo (ShellNew): administra qué tipos de archivo aparecen en el submenú «Nuevo»</sub></td>
    <td width="50%"><img src="../screenshots/desktop-visual.png" alt="Visualización del escritorio vacío" /><br/><sub>Escritorio vacío: la vista visual simula el menú contextual 1:1</sub></td>
  </tr>
  <tr>
    <td colspan="2" align="center"><img src="../screenshots/shellnew.png" alt="Página Acerca de" width="50%" /><br/><sub>Acerca de: versión / licencia / autor y página principal del proyecto</sub></td>
  </tr>
</table>

---

## Qué problemas resuelve

| Escenario | Sin esta herramienta | Con esta herramienta |
|---|---|---|
| El menú contextual de Win11 es demasiado escueto, los elementos habituales quedan plegados en «Mostrar más opciones» | Un clic más y una espera más cada vez | Vuelve al menú clásico completo con un clic, con efecto inmediato |
| El software instalado ha metido un montón de elementos en el menú contextual, desordenado y largo | Solo puedes hurgar manualmente en el Registro | Selección múltiple en la vista de lista, administrado de un vistazo |
| Quieres dar a un elemento de menú un nombre o icono más agradable | Tienes que entender la estructura del Registro para atreverte | Un clic en la vista visual y queda cambiado |
| Quieres añadir de forma uniforme una marca a un conjunto de elementos de menú | Cambiarlos uno por uno a mano | Magia por lotes, selecciona y añade un sufijo con un clic |

---

## Funciones principales

- **Cobertura de todos los escenarios del menú contextual**: una única barra lateral enumera verticalmente todos los escenarios del menú contextual, un clic para cambiar ——
  - **Escenarios del Registro**: escritorio vacío, archivo, carpeta, carpeta vacía, unidad, Papelera de reciclaje, todos los objetos de archivo, Este equipo, Bibliotecas.
  - **Menú Win+X**: administra ese menú avanzado de «clic derecho en el botón Inicio / Win+X» (visualización agrupada, activar/desactivar, añadir elementos personalizados, eliminar).
  - **Enviar a (SendTo)**: administra los elementos del submenú «Enviar a» del menú contextual, con activar/desactivar, añadir, eliminar.
  - **Nuevo (ShellNew)**: administra qué tipos de archivo aparecen en el submenú «Nuevo» del menú contextual, activar/desactivar por elemento.
  - **Controladores de arrastrar y soltar (DragDrop)**: administra las extensiones de menú que aparecen al arrastrar archivos con el botón derecho.
  - **Abrir con (Open with)**: consulta las aplicaciones registradas como candidatas de «Abrir con» (solo lectura).
- **Sincronización en tiempo real de las dos vistas**
  - **Vista visual (WYSIWYG)**: simula el menú contextual real de Windows 1:1, haz clic en el nombre para cambiarlo, en el icono para cambiarlo, **haz clic derecho en un elemento de menú para abrir un menú de acciones (cambiar nombre / cambiar icono / activar desactivar)**, lo que ves es lo que obtienes.
  - **Vista de lista (List View)**: tabla tradicional, selección múltiple con casillas, interruptor de activar/desactivar, botones de cambiar nombre y cambiar icono, todo presente, ideal para administrar un conjunto de elementos de menú a la vez.
- **Magia por lotes**: selecciona varios elementos de menú y añádeles a todos el mismo sufijo con un clic (por ejemplo « miau~», « ★»), también personalizable.
- **Zona de Windows 11**
  - **Intercambio del menú clásico con un clic**: escribe un `InprocServer32` vacío en HKCU para deshabilitar la extensión del menú moderno, el Explorador de archivos vuelve al menú clásico de Win10; y reinicia `explorer.exe` de forma elegante, el menú tiene efecto inmediato, sin necesidad de cerrar e iniciar sesión de nuevo. Otro clic para restaurar.
  - **Extensión del menú moderno (en desarrollo)**: interfaz reservada para escanear y tomar el control de los elementos de extensión de paquete disperso (MSIX) `IExplorerCommand` de Win11.
- **Multiidioma**: selección automática en el primer inicio según el idioma de visualización del sistema (chino simplificado/tradicional, todo lo demás en inglés), la interfaz se puede cambiar en cualquier momento.
- **Puramente local, reversible**: solo lee y escribe el Registro del usuario actual, sin conexión a internet, sin subida de datos, eliminar la clave restaura los valores predeterminados del sistema.

---

## Inicio rápido

1. Descarga desde [Releases](https://github.com/Henglie/ContextMenuMaster/releases) el zip correspondiente a tu arquitectura (la mayoría de los equipos eligen x64), descomprímelo en cualquier directorio.
2. Haz doble clic en `ContextMenuMaster.exe`, en el aviso de UAC que aparece haz clic en «Sí» para conceder permisos de administrador (para leer y escribir los elementos de menú que el software de terceros escribe en `HKLM`; sin elevación solo se pueden cambiar los elementos del usuario actual).
3. Sin instalación ni conexión a internet. En el primer inicio se selecciona automáticamente el idioma de la interfaz según el idioma del sistema, arriba a la derecha puedes cambiar en cualquier momento entre simplificado / tradicional / English.

---

## Cómo usarlo

### 1 · Elegir escenario

La barra lateral izquierda enumera verticalmente todos los escenarios del menú contextual, divididos en tres grupos:

- **Escenarios habituales**: escritorio vacío, archivo, carpeta, carpeta vacía, unidad, Papelera de reciclaje, todos los objetos de archivo, Este equipo, Bibliotecas —— corresponden a los menús que ves al hacer clic derecho sobre distintos objetos.
- **Submenús**: Enviar a, Nuevo, arrastrar y soltar, Abrir con, tipos de archivo —— los submenús del menú contextual y la administración por extensión de archivo.
- **Acceso rápido**: Win+X (el menú avanzado del clic derecho en el botón Inicio).

Arriba hay además dos accesos independientes: **Zona de Windows 11** y **Contenido multimedia**. Haz clic en cualquier escenario para escanear sus elementos de menú reales, la insignia de la pestaña muestra el número de elementos.

### 2 · Modificar el menú

Dos vistas, conmutables arriba a la derecha, los datos se sincronizan en tiempo real:

- **Vista visual**: simula el menú contextual de Windows 1:1.
  - Haz clic en el **nombre** de un elemento de menú para cambiarlo directamente; haz clic en el **icono** para cambiarlo.
  - **Haz clic derecho** en cualquier elemento de menú para abrir un menú de acciones: cambiar nombre / cambiar icono / activar desactivar / eliminar (elementos creados por ti).
  - El lado del menú moderno es una vista previa de solo lectura (el menú moderno de Win11 no está controlado por el Registro, ver la explicación más abajo), el lado del menú clásico es editable.
- **Vista de lista**: tabla tradicional, ideal para operaciones por lotes.
  - Cada fila tiene un interruptor de activar/desactivar y botones de cambiar nombre / cambiar icono.
  - Tras marcar varias filas, usa la **magia por lotes** para añadirles con un clic un sufijo uniforme (por ejemplo « ★»), también personalizable.

Cambiar nombre / cambiar icono / activar-desactivar requieren una segunda confirmación en los elementos integrados del sistema; antes de cada escritura en el Registro se hace automáticamente una copia de seguridad en `%LOCALAPPDATA%\ContextMenuMaster\backups\`, restaurable si algo sale mal.

### 3 · Recuperar el menú clásico (Windows 11)

Entra en la **Zona de Windows 11**, haz clic en «Cambiar a menú clásico»: el programa deshabilitará la extensión del menú moderno y reiniciará `explorer.exe` de forma elegante, el menú contextual vuelve inmediatamente al estilo completo de Win10, sin necesidad de cerrar e iniciar sesión de nuevo. Otro clic restaura el menú moderno.

### 4 · Crear tus propios elementos de menú

En la barra de herramientas de la vista de lista haz clic en «Nuevo elemento de menú», rellena el nombre para mostrar, el tipo de ejecución (ejecutar programa / abrir carpeta / abrir URL / línea de comandos personalizada), la ruta de destino, el icono (opcional, admite la conversión automática de cualquier imagen a ICO) y la posición (arriba / abajo). Los elementos creados por ti llevan una marca propia, editables o eliminables en cualquier momento; la eliminación solo borra los elementos creados por esta herramienta, sin tocar las asociaciones del sistema.

> **Sobre el menú moderno de Win11**: el nuevo menú contextual de Windows 11 solo acepta extensiones `IExplorerCommand` con identificador de paquete (MSIX / paquete disperso), los elementos de menú tradicionales del Registro se pliegan todos en el menú clásico de «Mostrar más opciones» —— esto es diseño del sistema, ajeno a los permisos. Por eso esta herramienta administra la capa del menú clásico; para que los elementos habituales vuelvan al nivel superior, usa el «Recuperar el menú clásico» de arriba.

---

## Multiidioma

El backend en C++ lee el idioma de visualización predeterminado del sistema mediante la API de Win32 y lo inyecta en el frontend antes de crear la página de WebView2:

- Sistema en chino (zh-CN / zh-TW / zh-HK, etc.) → interfaz renderizada en chino según simplificado/tradicional.
- Sistema en cualquier otro idioma → siempre repliega a inglés (en).

Arriba a la derecha de la interfaz puedes cambiar manualmente en cualquier momento entre simplificado / tradicional / English, la elección se registra en el almacenamiento local.

---

## Pila tecnológica y arquitectura

| Capa | Elección | Descripción |
|---|---|---|
| Motor de backend | C++17 moderno / MSVC | llamada nativa directa a la API de Win32 |
| Capa de presentación | Microsoft Edge WebView2 | la biblioteca de un solo encabezado webview aloja el WebView2 propio del sistema |
| Puente de comunicación | WebMessage / postMessage | `w.bind` expone las funciones del backend, el frontend las llama con `await window.fn()` |
| Interfaz | Vue 3 (local) | aplicación de un solo archivo |
| Sistema visual | FairyGlass | sistema de diseño de vidrio líquido oscuro ligeramente azulado, tokens replicados en línea en `ui/index.html` |
| Compilación | CMake + Ninja | reconocido por «Abrir carpeta» de VS2022 |

**Arquitectura: Core-Shell (núcleo genérico + carcasa nativa)**, idéntica al proyecto hermano FairySave: `src/platform/windows` es la capa privilegiada de Win32 (detección de i18n, toma de control de los menús del Registro), `ui/` es la capa de presentación.

**Compatibilidad**: Windows 10 / 11 (la toma de control del menú clásico es una característica de Win11). Requiere el runtime de WebView2 (incluido en Win10/11).

---

## Desarrollo y compilación

**Entorno**: VS2022 (con la carga de trabajo de escritorio de C++, CMake, Ninja, Windows SDK) + runtime de WebView2.

**Vía más sencilla**: en VS2022 elige «Abrir carpeta local» y selecciona este directorio, lee automáticamente `CMakePresets.json` y lo reconoce como proyecto CMake, elige el preajuste `windows-debug` o `windows-release` para generar y ejecutar.

**Vía por línea de comandos** (en el entorno vcvars64):

```bat
cmake --preset windows-release
cmake --build build/release
```

Productos:

- `build/release/bin/ContextMenuMaster.exe` —— programa principal GUI (subsistema WIN32, sin consola).
- `build/release/bin/CmmCli.exe` —— sonda de depuración por línea de comandos (para validar el backend, no orientada al usuario final).

> Si tras modificar `ui/` solo se cambió el HTML, hay que sincronizar manualmente `ui/` al directorio `ui/` situado junto al exe (el POST_BUILD de CMake solo se activa al volver a enlazar).

---

## Estructura de directorios

```
ContextMenuMaster/
├── CMakeLists.txt              Configuración principal de CMake (≈ .sln, con /utf-8 /EHsc + adelgazamiento de Release)
├── CMakePresets.json           Reconocimiento con un clic de VS (windows-debug / release / release-x86 / release-arm64)
├── src/
│   ├── main.cpp                Punto de entrada de depuración CLI (CmmCli)
│   ├── app/
│   │   ├── gui_main.cpp        Punto de entrada GUI WinMain: ventana webview + bind + inyección de idioma
│   │   └── app.manifest        Manifiesto de elevación UAC requireAdministrator
│   ├── platform/windows/       Capa privilegiada de Win32 (un módulo por escenario del menú contextual)
│   │   ├── i18n.{h,cpp}          Detección del idioma del sistema (zh-CN / zh-TW / en)
│   │   ├── context_menu.{h,cpp}  Toma de control del menú clásico↔moderno de Win11 + reinicio elegante del Explorador
│   │   ├── menu_scanner.{h,cpp}  Escaneo de escenarios del Registro + cambiar nombre/icono/activar-desactivar/sufijo por lotes
│   │   ├── icon_extract.{h,cpp}  Icono del sistema/recurso/.ico → PNG base64 data URI
│   │   ├── image_icon.{h,cpp}    Cualquier imagen → PNG-in-ICO multitamaño (fase 1 de contenido multimedia)
│   │   ├── com_menu_probe.{h,cpp} Detección dinámica de menús de la extensión COM IContextMenu
│   │   ├── win11menu.{h,cpp}     Menú moderno de Win11 (PackagedCom / SystemCommandStore)
│   │   ├── winx_menu.{h,cpp}     Menú Win+X (sistema de archivos .lnk + hash)
│   │   ├── sendto_menu.{h,cpp}   Submenú Enviar a
│   │   ├── shellnew_menu.{h,cpp} Submenú Nuevo
│   │   ├── dragdrop_menu.{h,cpp} Controladores de arrastrar y soltar
│   │   ├── openwith_menu.{h,cpp} Abrir con
│   │   ├── custom_menu.{h,cpp}   Elementos de menú contextual creados por el usuario
│   │   └── filetype_menu.{h,cpp} Administración de los elementos de menú contextual de tipos de archivo por extensión
│   └── shellext/               DLL de extensión de shell de contenido multimedia (fase 2, independiente, sin enlazar cmm_core)
│       ├── shellext.cpp          Servidor COM en proceso IContextMenu + miniatura owner-draw
│       └── shellext.def          Cuatro exportaciones COM
├── third_party/                biblioteca de un solo encabezado webview + WebView2 SDK
└── ui/
    ├── index.html              Aplicación Vue3 de un solo archivo (piel de vidrio líquido + i18n en tres idiomas)
    ├── vue.global.js           Runtime de Vue 3 (local)
    └── liquidGlass.js          Auxiliar de vidrio líquido
```

---

## Privacidad y seguridad

- **Puramente local**: sin conexión a internet, sin subida de contenido alguno.
- **Requiere permisos de administrador**: los elementos de menú del software de terceros suelen registrarse en `HKLM` (todos los usuarios), y su marca de desactivación también en su mayoría en `HKLM`, no modificables en modo puramente de usuario. Por eso el programa solicita permisos de administrador al iniciarse, para leer y escribir directamente en la ubicación real del Registro de los elementos de menú, y así admitir que «todos los elementos se puedan activar-desactivar / cambiar de nombre».
- **Copia de seguridad automática antes de escribir**: antes de cada cambio en el Registro, un `reg export` hace primero una copia de seguridad de la clave correspondiente localmente en `%LOCALAPPDATA%\ContextMenuMaster\backups\`.
- **Reversible**: la toma de control del menú clásico se restaura en cualquier momento eliminando la clave; activar-desactivar funciona mediante «añadir/eliminar una marca de desactivación / mover a un espejo desactivado», reversible sin cambios; la eliminación de elementos los envía siempre a la Papelera de reciclaje. No daña los archivos del sistema.

---

## Descarga

- Versión precompilada en [Releases](https://github.com/Henglie/ContextMenuMaster/releases): elige el zip según la arquitectura de la CPU (x64 / x86 / arm64), tras descomprimir haz doble clic en `ContextMenuMaster.exe` y úsalo directamente, sin instalación. La gran mayoría de los equipos eligen x64.
- Requiere el runtime de WebView2 (incluido en Windows 10/11).

---

## Licencia y autor

- **Autor**: 恒烈 (EternalBlaze / Henglie)
- **Versión**: v0.6
- **Página principal del proyecto**: <https://github.com/Henglie/ContextMenuMaster>
- **Licencia**: MIT
