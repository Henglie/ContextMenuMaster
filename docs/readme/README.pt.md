<div align="center">

<img src="../../res/app-256.png" width="128" height="128" alt="Context Menu Master" />

# Gerenciador do Menu de Contexto Context Menu Master

**Retome o controle do menu de contexto no Windows 11** · Interface liquid-glass · Totalmente local e reversível

[![Release](https://img.shields.io/badge/release-v0.6-4d9fff?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-6a5ae0?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![License](https://img.shields.io/badge/license-MIT-22c55e?style=flat-square)](LICENSE)
[![Arch](https://img.shields.io/badge/arch-x64%20%7C%20x86%20%7C%20arm64-f59e0b?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)

<sub>Arquitetura Core-Shell · core em C++17 · frontend WebView2 · interface em 16 idiomas</sub>

</div>

---

[简体中文](../../README.md) | [繁體中文](README.zh-TW.md) | [English](../../README.en.md) | [日本語](README.ja.md) | [한국어](README.ko.md) | [Français](README.fr.md) | [Deutsch](README.de.md) | [Español](README.es.md) | **Português** | [Русский](README.ru.md) | [Italiano](README.it.md) | [العربية](README.ar.md) | [עברית](README.he.md) | [हिन्दी](README.hi.md) | [ไทย](README.th.md) | [Tiếng Việt](README.vi.md)

> Retome o controle do menu de contexto no Windows 11.

O **Gerenciador do Menu de Contexto** é uma ferramenta de produtividade para desktop Windows. Ela devolve às suas mãos o menu de contexto bagunçado, recolhido e entupido por diversos programas: renomeie e troque ícones de forma visual, gerencie em lote por caixas de seleção, recupere com um clique o menu clássico que o Windows 11 escondeu, cobrindo todos os cenários de menu de contexto — área de trabalho / arquivo / pasta / unidade / Lixeira / Win+X / Enviar para / Novo / arrastar e soltar.

Execução totalmente local, com backup automático do Registro antes de qualquer operação de escrita e todas as operações reversíveis a qualquer momento. Execute como administrador para gerenciar os itens de menu no escopo do sistema e de todos os usuários (HKLM).

> [!WARNING]
> **A extensão de shell de mídia avançada (`CmmShellExt.dll`) é atualmente experimental.** É uma extensão de shell COM in-process que só entra em vigor após você registrá-la manualmente como administrador com `regsvr32 CmmShellExt.dll`; uma vez registrada, ao clicar com o botão direito em um arquivo aparecem itens de menu com miniaturas reais. Essa parte ainda não foi amplamente validada em máquinas reais e pode se comportar de forma instável — ou não aparecer — em alguns sistemas, tipos de arquivo ou ambientes com software de segurança. Ela vem com salvaguardas (arquivos compactados acima de 200MB ou enumeração acima de 800ms são automaticamente rebaixados para um item de menu simples, de modo que o menu nunca congela), mas ainda assim não é recomendável deixá-la registrada a longo prazo em ambientes importantes. Os recursos de gerenciamento de menu do aplicativo principal não dependem dela e podem ser usados por conta própria.

---

## Capturas de tela

> A primeira tela é a visualização visual: a barra lateral esquerda distribui todos os cenários de menu de contexto e, ao centro, "esquema somente leitura do menu moderno → menu clássico editável" são apresentados lado a lado.

![Interface principal da visualização visual](../screenshots/main-visual.png)

<table>
  <tr>
    <td width="50%"><img src="../screenshots/list-view.png" alt="Visualização em lista" /><br/><sub>Visualização em lista: seleção em lote por caixas de seleção, interruptores de ativar/desativar, barra de ferramentas de mágica em lote</sub></td>
    <td width="50%"><img src="../screenshots/win11-zone.png" alt="Zona do Windows 11" /><br/><sub>Zona do Windows 11: troca entre menu clássico/novo com um clique, reinício elegante do Explorer</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="../screenshots/about.png" alt="Cenário Novo (ShellNew)" /><br/><sub>Novo (ShellNew): gerencie quais tipos de arquivo aparecem no submenu "Novo"</sub></td>
    <td width="50%"><img src="../screenshots/desktop-visual.png" alt="Visualização da área de trabalho vazia" /><br/><sub>Área de trabalho vazia: a visualização visual simula o menu de contexto em escala 1:1</sub></td>
  </tr>
  <tr>
    <td colspan="2" align="center"><img src="../screenshots/shellnew.png" alt="Página Sobre" width="50%" /><br/><sub>Sobre: versão / licença / autor e página do projeto</sub></td>
  </tr>
</table>

---

## Que problema ele resolve

| Cenário | Sem esta ferramenta | Com esta ferramenta |
|---|---|---|
| O menu de contexto do Win11 é enxuto demais e os itens frequentes ficam recolhidos em "Mostrar mais opções" | Um clique e uma espera a mais toda vez | Volte ao menu clássico completo com um clique, com efeito imediato |
| Os programas instalados enfiaram vários itens no menu de contexto, deixando-o longo e confuso | Só resta vasculhar o Registro manualmente | Selecione em lote na visualização em lista, gerencie de relance |
| Quer dar um nome ou ícone mais agradável a um item de menu | Precisa entender a estrutura do Registro para ousar mexer | Basta um clique na visualização visual |
| Quer marcar um conjunto de itens de forma uniforme | Alterar um por um manualmente | Mágica em lote: selecione e adicione um sufixo com um clique |

---

## Recursos principais

- **Cobertura de todos os cenários de menu de contexto**: uma barra lateral distribui verticalmente todos os cenários; basta um clique para alternar —
  - **Cenários do Registro**: área de trabalho vazia, arquivo, pasta, área vazia de pasta, unidade, Lixeira, todos os objetos de arquivo, Este Computador, bibliotecas.
  - **Menu Win+X**: gerencie o menu avançado do "botão Iniciar com o botão direito / Win+X" (exibição por grupos, ativar/desativar, adicionar itens personalizados, excluir).
  - **Enviar para (SendTo)**: gerencie os itens do submenu "Enviar para" do menu de contexto, com ativar/desativar, adicionar e excluir.
  - **Novo (ShellNew)**: gerencie quais tipos de arquivo aparecem no submenu "Novo" do menu de contexto, ativando/desativando item a item.
  - **Manipuladores de arrastar e soltar (DragDrop)**: gerencie as extensões de menu que aparecem ao arrastar arquivos com o botão direito.
  - **Abrir com (Open with)**: veja os aplicativos registrados como candidatos a "Abrir com" (somente leitura).
- **Sincronização em tempo real das duas visualizações**
  - **Visualização visual (WYSIWYG)**: simula em escala 1:1 o menu de contexto real do Windows; clique no nome para renomear, clique no ícone para trocá-lo, **clique com o botão direito em um item de menu para abrir o menu de ações (renomear / trocar ícone / ativar ou desativar)** — o que você vê é o que você obtém.
  - **Visualização em lista (List View)**: tabela tradicional, com seleção em lote por caixas de seleção, interruptores de ativar/desativar e botões de renomear e trocar ícone, ideal para gerenciar um conjunto de itens de uma vez.
- **Mágica em lote**: selecione vários itens de menu e adicione a todos eles, com um clique, o mesmo sufixo (como " miau~", " ★"), também personalizável.
- **Zona do Windows 11**
  - **Troca do menu clássico com um clique**: grava um `InprocServer32` vazio em HKCU para desativar as extensões do menu novo, fazendo o Explorador de Arquivos recair no menu clássico do Win10; e reinicia o `explorer.exe` de forma elegante, aplicando o menu instantaneamente, sem precisar sair e entrar novamente. Basta clicar de novo para restaurar.
  - **Extensões do menu novo (em desenvolvimento)**: interface reservada para escanear e assumir o controle dos itens de extensão `IExplorerCommand` de pacotes esparsos (MSIX) do Win11.
- **Multilíngue**: na primeira execução, seleciona automaticamente pelo idioma de exibição do sistema (chinês simplificado/tradicional, e inglês para todo o resto); a interface pode ser alternada a qualquer momento.
- **Totalmente local e reversível**: lê e escreve apenas no Registro do usuário atual, sem conexão à internet nem envio de dados; excluir a chave restaura o padrão do sistema.

---

## Início rápido

1. Baixe o zip da arquitetura correspondente em [Releases](https://github.com/Henglie/ContextMenuMaster/releases) (a maioria dos computadores usa x64) e extraia para qualquer diretório.
2. Dê dois cliques em `ContextMenuMaster.exe` e, no prompt do UAC que aparecer, clique em "Sim" para conceder permissões de administrador (usadas para ler e escrever nos itens de menu que programas de terceiros gravam em `HKLM`; sem elevação, só é possível alterar os itens do usuário atual).
3. Sem instalação e sem internet. Na primeira execução, o idioma da interface é escolhido automaticamente pelo idioma do sistema; no canto superior direito é possível alternar entre chinês simplificado / tradicional / English a qualquer momento.

---

## Como usar

### 1 · Escolher o cenário

A barra lateral esquerda distribui verticalmente todos os cenários de menu de contexto, em três grupos:

- **Cenários frequentes**: área de trabalho vazia, arquivo, pasta, área vazia de pasta, unidade, Lixeira, todos os objetos de arquivo, Este Computador, bibliotecas — correspondem aos menus que você vê ao clicar com o botão direito em diferentes objetos.
- **Submenus**: Enviar para, Novo, arrastar e soltar, Abrir com, tipos de arquivo — os submenus do menu de contexto e o gerenciamento por extensão.
- **Entradas rápidas**: Win+X (o menu avançado do botão Iniciar acionado com o botão direito).

No topo há ainda duas entradas independentes: **Zona do Windows 11** e **Rich Media**. Clique em qualquer cenário para escanear os itens de menu reais dele; o rótulo mostra o número de itens.

### 2 · Alterar o menu

Duas visualizações, alternadas no canto superior direito, com dados sincronizados em tempo real:

- **Visualização visual**: simula em escala 1:1 o menu de contexto do Windows.
  - Clique no **nome** do item de menu para renomeá-lo diretamente; clique no **ícone** para trocá-lo.
  - **Clique com o botão direito** em qualquer item de menu para abrir o menu de ações: renomear / trocar ícone / ativar ou desativar / excluir (itens criados por você).
  - O lado do menu moderno é um esquema somente leitura (o menu moderno do Win11 não é controlado pelo Registro, veja a explicação abaixo); o lado do menu clássico é editável.
- **Visualização em lista**: tabela tradicional, ideal para operações em lote.
  - Cada linha tem um interruptor de ativar/desativar e botões de renomear / trocar ícone.
  - Depois de marcar várias linhas, use a **mágica em lote** para adicionar a todas, com um clique, um sufixo uniforme (como " ★"), também personalizável.

Renomear / trocar ícone / ativar ou desativar itens internos do sistema exige uma segunda confirmação; antes de cada escrita no Registro, é feito backup automático em `%LOCALAPPDATA%\ContextMenuMaster\backups\`, para restaurar caso algo dê errado.

### 3 · Recuperar o menu clássico (Windows 11)

Vá até a **Zona do Windows 11** e clique em "Alternar para o menu clássico": o programa desativa as extensões do menu novo e reinicia o `explorer.exe` de forma elegante; o menu de contexto volta instantaneamente ao estilo completo do Win10, sem precisar sair e entrar novamente. Basta clicar de novo para restaurar o menu novo.

### 4 · Criar seus próprios itens de menu

Na barra de ferramentas da visualização em lista, clique em "Novo item de menu" e preencha o nome de exibição, o tipo de execução (executar programa / abrir pasta / abrir URL / linha de comando personalizada), o caminho de destino, o ícone (opcional, aceita qualquer imagem com conversão automática para ICO) e a posição (topo / base). Os itens criados por você levam uma marca própria e podem ser editados ou excluídos a qualquer momento; a exclusão limpa apenas os itens criados por esta ferramenta, sem tocar nas associações do sistema.

> **Sobre o menu moderno do Win11**: o novo menu de contexto do Windows 11 só aceita extensões `IExplorerCommand` com identidade de pacote (MSIX / pacote esparso); os itens de menu tradicionais do Registro são todos recolhidos no menu clássico dentro de "Mostrar mais opções" — isso é design do sistema e não tem relação com permissões. Por isso, esta ferramenta gerencia a camada do menu clássico; para trazer os itens frequentes de volta ao nível superior, use o "Recuperar o menu clássico" acima.

---

## Multilíngue

O backend em C++ usa a Win32 API para ler o idioma de exibição padrão do sistema e o injeta no frontend antes de criar a página do WebView2:

- Sistema em chinês (zh-CN / zh-TW / zh-HK etc.) → renderiza a interface em chinês, simplificado ou tradicional.
- Sistema em qualquer outro idioma → recai sempre no inglês (en).

No canto superior direito da interface é possível alternar manualmente entre chinês simplificado / tradicional / English a qualquer momento; a escolha é registrada no armazenamento local.

---

## Pilha tecnológica e arquitetura

| Camada | Escolha | Descrição |
|---|---|---|
| Motor do backend | C++17 moderno / MSVC | Chamada nativa direta à Win32 API |
| Camada de apresentação | Microsoft Edge WebView2 | A biblioteca de cabeçalho único webview hospeda o WebView2 nativo do sistema |
| Ponte de comunicação | WebMessage / postMessage | `w.bind` expõe funções do backend; o frontend chama com `await window.fn()` |
| Interface | Vue 3 (local) | Aplicativo de arquivo único |
| Sistema visual | FairyGlass | Sistema de design de vidro líquido em azul-escuro sutil, com tokens replicados inline em `ui/index.html` |
| Build | CMake + Ninja | Reconhecido pelo "Abrir Pasta" do VS2022 |

**Arquitetura: Core-Shell (núcleo genérico + casca nativa)**, igual ao projeto irmão FairySave: `src/platform/windows` é a camada privilegiada Win32 (detecção de i18n, controle dos menus do Registro) e `ui/` é a camada de apresentação.

**Compatibilidade**: Windows 10 / 11 (o controle do menu clássico é um recurso do Win11). Requer o runtime do WebView2 (nativo no Win10/11).

---

## Desenvolvimento e build

**Ambiente**: VS2022 (com a carga de trabalho de desktop C++, CMake, Ninja e Windows SDK) + runtime do WebView2.

**Forma mais simples**: no VS2022, use "Abrir Pasta Local" e selecione este diretório; ele lê automaticamente o `CMakePresets.json`, reconhece como projeto CMake, e você seleciona o preset `windows-debug` ou `windows-release` para gerar e executar.

**Forma por linha de comando** (no ambiente vcvars64):

```bat
cmake --preset windows-release
cmake --build build/release
```

Produtos:

- `build/release/bin/ContextMenuMaster.exe` — programa principal com GUI (subsistema WIN32, sem console).
- `build/release/bin/CmmCli.exe` — sonda de depuração por linha de comando (para validar o backend, não voltada ao usuário final).

> Depois de alterar `ui/`, se você mexeu apenas no HTML, é preciso sincronizar manualmente `ui/` para o diretório `ui/` ao lado do exe (o POST_BUILD do CMake só é acionado ao relinkar).

---

## Estrutura de diretórios

```
ContextMenuMaster/
├── CMakeLists.txt              Configuração principal do CMake (≈ .sln, com /utf-8 /EHsc + enxugamento do Release)
├── CMakePresets.json           Reconhecimento em um clique pelo VS (windows-debug / release / release-x86 / release-arm64)
├── src/
│   ├── main.cpp                Ponto de entrada de depuração CLI (CmmCli)
│   ├── app/
│   │   ├── gui_main.cpp        Entrada da GUI WinMain: janela webview + bind + injeção de idioma
│   │   └── app.manifest        Manifesto de elevação UAC requireAdministrator
│   ├── platform/windows/       Camada privilegiada Win32 (um módulo por cenário de menu de contexto)
│   │   ├── i18n.{h,cpp}          Detecção do idioma do sistema (zh-CN / zh-TW / en)
│   │   ├── context_menu.{h,cpp}  Controle do menu clássico↔novo do Win11 + reinício elegante do Explorer
│   │   ├── menu_scanner.{h,cpp}  Varredura de cenários do Registro + renomear/ícone/ativar-desativar/sufixo em lote
│   │   ├── icon_extract.{h,cpp}  Ícone do sistema/recurso/.ico → PNG base64 data URI
│   │   ├── image_icon.{h,cpp}    Qualquer imagem → PNG-in-ICO multitamanho (fase 1 de rich media)
│   │   ├── com_menu_probe.{h,cpp} Detecção dinâmica de menu de extensões COM IContextMenu
│   │   ├── win11menu.{h,cpp}     Menu moderno do Win11 (PackagedCom / SystemCommandStore)
│   │   ├── winx_menu.{h,cpp}     Menu Win+X (arquivos .lnk + hash)
│   │   ├── sendto_menu.{h,cpp}   Submenu Enviar para
│   │   ├── shellnew_menu.{h,cpp} Submenu Novo
│   │   ├── dragdrop_menu.{h,cpp} Manipuladores de arrastar e soltar
│   │   ├── openwith_menu.{h,cpp} Abrir com
│   │   ├── custom_menu.{h,cpp}   Itens de menu de contexto criados pelo usuário
│   │   └── filetype_menu.{h,cpp} Gerenciamento dos itens de contexto por tipo de arquivo/extensão
│   └── shellext/               DLL de extensão de shell rich media (fase 2, independente, não linka cmm_core)
│       ├── shellext.cpp          Servidor COM in-process IContextMenu + miniatura owner-draw
│       └── shellext.def          As quatro exportações COM
├── third_party/                Biblioteca de cabeçalho único webview + WebView2 SDK
└── ui/
    ├── index.html              Aplicativo Vue3 de arquivo único (tema de vidro líquido + i18n em três idiomas)
    ├── vue.global.js           Runtime do Vue 3 (local)
    └── liquidGlass.js          Auxiliar do vidro líquido
```

---

## Privacidade e segurança

- **Totalmente local**: sem conexão à internet e sem envio de qualquer conteúdo.
- **Requer permissões de administrador**: os itens de menu de programas de terceiros costumam ser registrados em `HKLM` (todos os usuários), e suas marcas de desativação também ficam em grande parte em `HKLM`, impossíveis de alterar apenas no modo de usuário. Por isso o programa solicita permissões de administrador ao iniciar, para ler e escrever diretamente no local real do Registro onde os itens de menu residem, permitindo que "todos os itens possam ser ativados/desativados / renomeados".
- **Backup automático antes de escrever**: antes de cada alteração no Registro, é feito um `reg export` da chave correspondente para `%LOCALAPPDATA%\ContextMenuMaster\backups\` local.
- **Reversível**: o controle do menu clássico é restaurado a qualquer momento excluindo a chave; ativar/desativar usa "adicionar-remover marca / mover para o espelho de desativados" e pode ser trazido de volta ao estado original; itens excluídos vão sempre para a Lixeira. Não danifica arquivos do sistema.

---

## Download

- Versões pré-compiladas em [Releases](https://github.com/Henglie/ContextMenuMaster/releases): escolha o zip pela arquitetura da CPU (x64 / x86 / arm64), extraia e dê dois cliques em `ContextMenuMaster.exe` para usar, sem instalação. A grande maioria dos computadores usa x64.
- Requer o runtime do WebView2 (nativo no Windows 10/11).

---

## Licença e autor

- **Autor**: 恒烈 (EternalBlaze / Henglie)
- **Versão**: v0.6
- **Página do projeto**: <https://github.com/Henglie/ContextMenuMaster>
- **Licença**: MIT
