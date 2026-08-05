<div align="center">

<img src="../../res/app-256.png" width="128" height="128" alt="Context Menu Master" />

# 마우스 오른쪽 클릭 메뉴 매니저 Context Menu Master

**Windows 11에서 오른쪽 클릭 메뉴를 다시 당신의 손에** · 리퀴드 글래스 UI · 완전 로컬, 완전 되돌리기 가능

[![Release](https://img.shields.io/badge/release-v0.6-4d9fff?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-6a5ae0?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![License](https://img.shields.io/badge/license-MIT-22c55e?style=flat-square)](LICENSE)
[![Arch](https://img.shields.io/badge/arch-x64%20%7C%20x86%20%7C%20arm64-f59e0b?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)

<sub>Core-Shell 아키텍처 · C++17 코어 · WebView2 프런트엔드 · 16개 언어 UI</sub>

</div>

---

[简体中文](../../README.md) | [繁體中文](README.zh-TW.md) | [English](../../README.en.md) | [日本語](README.ja.md) | **한국어** | [Français](README.fr.md) | [Deutsch](README.de.md) | [Español](README.es.md) | [Português](README.pt.md) | [Русский](README.ru.md) | [Italiano](README.it.md) | [العربية](README.ar.md) | [עברית](README.he.md) | [हिन्दी](README.hi.md) | [ไทย](README.th.md) | [Tiếng Việt](README.vi.md)

> Windows 11에서 마우스 오른쪽 클릭 메뉴를 다시 당신의 손에.

**마우스 오른쪽 클릭 메뉴 매니저**는 Windows 데스크톱용 생산성 도구입니다. 어지럽고, 접혀 있고, 온갖 소프트웨어로 가득 찬 상황에 맞는 메뉴를 당신의 손에 돌려줍니다. 시각적으로 이름과 아이콘을 바꾸고, 일괄 선택으로 관리하며, Windows 11이 숨긴 클래식 메뉴를 원클릭으로 되찾을 수 있습니다. 또한 바탕 화면 / 파일 / 폴더 / 드라이브 / 휴지통 / Win+X / 보내기 / 새로 만들기 / 끌어서 놓기 등 모든 오른쪽 클릭 상황을 아우릅니다.

완전 로컬로 실행되며, 쓰기 작업 전에 레지스트리를 자동 백업하고, 모든 작업은 언제든 되돌릴 수 있습니다. 시스템 및 모든 사용자(HKLM) 범위의 메뉴 항목을 관리하려면 관리자 권한으로 실행하세요.

> [!WARNING]
> **리치 미디어 오른쪽 클릭 확장(`CmmShellExt.dll`)은 현재 실험적 기능입니다.** 이는 프로세스 내 COM 셸 확장으로, 관리자 권한으로 직접 `regsvr32 CmmShellExt.dll`을(를) 실행하여 등록한 후에만 적용됩니다. 등록 후 파일을 오른쪽 클릭하면 실제 썸네일이 있는 메뉴 항목이 표시됩니다. 이 부분은 아직 실제 기기에서 광범위하게 검증되지 않았으며, 일부 시스템, 파일 형식 또는 보안 소프트웨어 환경에서는 동작이 불안정하거나 아예 표시되지 않을 수 있습니다. 안전장치(압축 파일이 200MB를 초과하거나 열거가 800ms를 초과하면 자동으로 일반 메뉴 항목으로 강등되어 오른쪽 클릭 메뉴가 멈추지 않음)를 내장하고 있지만, 그래도 중요한 환경에서 장기간 등록해 두는 것은 권장하지 않습니다. 본체 앱의 메뉴 관리 기능은 이에 의존하지 않으며 단독으로 안심하고 사용할 수 있습니다.

---

## 화면 스크린샷

> 첫 화면은 시각적 보기입니다. 왼쪽 사이드바에 모든 오른쪽 클릭 상황이 나열되고, 가운데에는 "최신 메뉴 읽기 전용 미리보기 → 클래식 메뉴 편집 가능"이 나란히 표시됩니다.

![시각적 보기 메인 화면](../screenshots/main-visual.png)

<table>
  <tr>
    <td width="50%"><img src="../screenshots/list-view.png" alt="목록 보기" /><br/><sub>목록 보기: 확인란으로 일괄 선택, 켜기/끄기 스위치, 일괄 매직 도구 모음</sub></td>
    <td width="50%"><img src="../screenshots/win11-zone.png" alt="Windows 11 존" /><br/><sub>Windows 11 존: 클래식/최신 메뉴 원클릭 전환, Explorer 우아하게 다시 시작</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="../screenshots/about.png" alt="새로 만들기 ShellNew 상황" /><br/><sub>새로 만들기(ShellNew): "새로 만들기" 하위 메뉴에 표시되는 파일 형식 관리</sub></td>
    <td width="50%"><img src="../screenshots/desktop-visual.png" alt="바탕 화면 빈 공간 시각화" /><br/><sub>바탕 화면 빈 공간: 시각적 보기가 오른쪽 클릭 메뉴를 1:1로 재현</sub></td>
  </tr>
  <tr>
    <td colspan="2" align="center"><img src="../screenshots/shellnew.png" alt="정보 페이지" width="50%" /><br/><sub>정보: 버전 / 라이선스 / 작성자와 프로젝트 홈페이지</sub></td>
  </tr>
</table>

---

## 어떤 문제를 해결하나

| 상황 | 이 도구가 없을 때 | 이 도구가 있을 때 |
|---|---|---|
| Win11 오른쪽 클릭 메뉴가 너무 간소해서, 자주 쓰는 항목이 "추가 옵션 표시"로 접힌다 | 매번 한 번 더 클릭, 한 번 더 기다림 | 원클릭으로 완전한 클래식 메뉴로 전환, 즉시 적용 |
| 설치한 소프트웨어가 오른쪽 클릭 메뉴에 항목을 잔뜩 밀어 넣어 어지럽고 길다 | 레지스트리를 직접 뒤질 수밖에 없다 | 목록 보기에서 일괄 선택, 한눈에 관리 |
| 메뉴 항목의 이름이나 아이콘을 보기 좋게 바꾸고 싶다 | 레지스트리 구조를 알아야 손댈 수 있다 | 시각적 보기에서 클릭 한 번으로 변경 |
| 여러 메뉴 항목에 표시를 일괄로 붙이고 싶다 | 하나씩 수동으로 변경 | 일괄 매직, 선택 후 원클릭으로 접미사 추가 |

---

## 핵심 기능

- **모든 오른쪽 클릭 상황 지원**: 하나의 사이드바에 모든 오른쪽 클릭 상황을 세로로 나열, 클릭 한 번으로 전환 ——
  - **레지스트리 상황**: 바탕 화면 빈 공간, 파일, 폴더, 폴더 빈 공간, 드라이브, 휴지통, 모든 파일 개체, 내 PC, 라이브러리.
  - **Win+X 메뉴**: "시작 단추 오른쪽 클릭 / Win+X"의 고급 메뉴를 관리(그룹 표시, 켜기/끄기, 사용자 지정 항목 추가, 삭제).
  - **보내기(SendTo)**: 오른쪽 클릭 "보내기" 하위 메뉴의 항목을 관리, 켜기/끄기, 추가, 삭제 가능.
  - **새로 만들기(ShellNew)**: 오른쪽 클릭 "새로 만들기" 하위 메뉴에 표시되는 파일 형식을 관리, 항목별 켜기/끄기.
  - **끌어서 놓기 처리기(DragDrop)**: 오른쪽 클릭으로 파일을 끌어서 놓을 때 나타나는 메뉴 확장을 관리.
  - **연결 프로그램(Open with)**: "연결 프로그램" 후보로 등록된 앱 보기(읽기 전용).
- **이중 보기 실시간 동기화**
  - **시각적 보기(WYSIWYG)**: Windows의 실제 오른쪽 클릭 메뉴를 1:1로 재현, 이름을 클릭해 이름 변경, 아이콘을 클릭해 아이콘 변경, **메뉴 항목을 마우스 오른쪽 단추로 클릭하면 작업 메뉴가 나타남(이름 변경 / 아이콘 변경 / 사용·사용 안 함)**, 보이는 그대로 얻는다.
  - **목록 보기(List View)**: 전통적인 표, 확인란으로 일괄 선택, 켜기/끄기 스위치, 이름 변경·아이콘 변경 단추를 모두 갖춤, 한 번에 여러 메뉴 항목을 관리하기에 적합.
- **일괄 매직**: 여러 메뉴 항목을 선택하고, 원클릭으로 모두에 동일한 접미사(예 " 냥~", " ★")를 추가, 사용자 지정도 가능.
- **Windows 11 존**
  - **클래식 메뉴 원클릭 전환**: HKCU에 빈 `InprocServer32`를 써서 최신 메뉴 확장을 무력화하고, 파일 탐색기를 Win10 클래식 메뉴로 되돌린다. 또한 `explorer.exe`를 우아하게 다시 시작해 메뉴가 즉시 적용, 로그오프·재로그인 불필요. 한 번 더 클릭하면 복원.
  - **최신 메뉴 확장(개발 중)**: Win11 `IExplorerCommand` 스파스 패키지(MSIX) 확장 항목을 스캔하고 인계받는 인터페이스를 예약.
- **다국어**: 최초 실행 시 시스템 표시 언어로 자동 선택(중국어 간체/번체, 그 외는 모두 영어), UI는 언제든 전환 가능.
- **완전 로컬·되돌리기 가능**: 현재 사용자 레지스트리만 읽고 쓰며, 네트워크에 연결하지 않고, 업로드하지 않으며, 키를 삭제하면 시스템 기본값으로 복원.

---

## 빠른 시작

1. [Releases](https://github.com/Henglie/ContextMenuMaster/releases)에서 해당 아키텍처의 zip을 다운로드(대부분의 PC는 x64 선택), 임의의 디렉터리에 압축 해제.
2. `ContextMenuMaster.exe`를 두 번 클릭하고, 나타나는 UAC 알림에서 "예"를 클릭해 관리자 권한 부여(타사 소프트웨어가 `HKLM`에 쓴 메뉴 항목을 읽고 쓰는 데 사용, 권한 상승을 하지 않으면 현재 사용자의 항목만 변경 가능).
3. 설치 불필요, 인터넷 연결 불필요. 최초 실행 시 시스템 언어로 UI 언어를 자동 선택, 오른쪽 위에서 언제든 간체 / 번체 / English 전환 가능.

---

## 사용 방법

### 1 · 상황 선택

왼쪽 사이드바에 모든 오른쪽 클릭 상황이 세로로 나열되며, 세 그룹으로 나뉩니다:

- **자주 쓰는 상황**: 바탕 화면 빈 공간, 파일, 폴더, 폴더 빈 공간, 드라이브, 휴지통, 모든 파일 개체, 내 PC, 라이브러리 —— 서로 다른 개체를 오른쪽 클릭할 때 보이는 메뉴에 대응.
- **하위 메뉴**: 보내기, 새로 만들기, 끌어서 놓기, 연결 프로그램, 파일 형식 —— 오른쪽 클릭 메뉴 안의 하위 메뉴와 확장자별 관리.
- **바로 가기 입구**: Win+X(시작 단추 오른쪽 클릭의 고급 메뉴).

상단에는 추가로 **Windows 11 존**과 **리치 미디어** 두 개의 독립 입구가 있습니다. 아무 상황이나 클릭하면 해당 상황의 실제 메뉴 항목을 스캔하고, 탭 배지에 항목 수가 표시됩니다.

### 2 · 메뉴 편집

두 가지 보기가 있으며, 오른쪽 위에서 전환하고, 데이터는 실시간으로 동기화됩니다:

- **시각적 보기**: Windows 오른쪽 클릭 메뉴를 1:1로 재현.
  - 메뉴 항목의 **이름**을 클릭해 바로 이름 변경, **아이콘**을 클릭해 아이콘 변경.
  - 아무 메뉴 항목이나 **마우스 오른쪽 단추로 클릭**하면 작업 메뉴가 나타남: 이름 변경 / 아이콘 변경 / 사용·사용 안 함 / 삭제(직접 만든 항목).
  - 최신 메뉴 쪽은 읽기 전용 미리보기(Win11 최신 메뉴는 레지스트리의 제어를 받지 않음, 아래 설명 참조), 클래식 메뉴 쪽은 편집 가능.
- **목록 보기**: 전통적인 표, 일괄 작업에 적합.
  - 각 행에 켜기/끄기 스위치, 이름 변경 / 아이콘 변경 단추.
  - 여러 행을 선택한 후, **일괄 매직**으로 원클릭해 통일된 접미사(예 " ★")를 추가, 사용자 지정도 가능.

이름 변경 / 아이콘 변경 / 켜기·끄기는 시스템 기본 항목에 대해 재확인합니다. 레지스트리에 쓸 때마다 `%LOCALAPPDATA%\ContextMenuMaster\backups\`에 자동 백업되므로, 잘못돼도 복원할 수 있습니다.

### 3 · 클래식 메뉴 되찾기(Windows 11)

**Windows 11 존**에 들어가 "클래식 메뉴로 전환"을 클릭: 프로그램이 최신 메뉴 확장을 무력화하고 `explorer.exe`를 우아하게 다시 시작하여, 오른쪽 클릭 메뉴가 즉시 Win10의 완전한 스타일로 돌아가며, 로그오프·재로그인 불필요. 한 번 더 클릭하면 최신 메뉴로 복원.

### 4 · 자신의 메뉴 항목 새로 만들기

목록 보기 도구 모음에서 "메뉴 항목 새로 만들기"를 클릭하고, 표시 이름, 실행 유형(프로그램 실행 / 폴더 열기 / URL 열기 / 사용자 지정 명령줄), 대상 경로, 아이콘(선택 사항, 임의의 이미지를 자동으로 ICO로 변환 지원), 위치(맨 위 / 맨 아래)를 입력. 직접 만든 항목에는 전용 표시가 붙고, 언제든 편집·삭제할 수 있으며, 삭제는 이 도구가 만든 항목만 지우고, 시스템 연결에는 손대지 않습니다.

> **Win11 최신 메뉴에 관하여**: Windows 11의 최신 오른쪽 클릭 메뉴는 패키지 식별자(MSIX / 스파스 패키지)가 있는 `IExplorerCommand` 확장만 받아들이며, 전통적인 레지스트리 메뉴 항목은 모두 "추가 옵션 표시"의 클래식 메뉴로 접힙니다 —— 이는 시스템 설계이며, 권한과 무관합니다. 따라서 이 도구가 관리하는 것은 클래식 메뉴 층입니다. 자주 쓰는 항목을 최상위로 되돌리려면, 위의 "클래식 메뉴 되찾기"를 사용하세요.

---

## 다국어

C++ 백엔드가 Win32 API로 시스템 기본 표시 언어를 읽고, WebView2 페이지 생성 전에 프런트엔드에 주입합니다:

- 시스템이 중국어(zh-CN / zh-TW / zh-HK 등) → 간체/번체에 따라 중국어 UI 렌더링.
- 시스템이 그 외 어떤 언어든 → 일률적으로 영어(en)로 폴백.

UI 오른쪽 위에서 간체 / 번체 / English를 언제든 수동으로 전환할 수 있으며, 선택은 로컬 저장소에 기록됩니다.

---

## 기술 스택과 아키텍처

| 층 | 선택 | 설명 |
|---|---|---|
| 백엔드 엔진 | 모던 C++17 / MSVC | Win32 API 네이티브 직접 호출 |
| 표현 층 | Microsoft Edge WebView2 | webview 단일 헤더 라이브러리가 시스템 내장 WebView2를 담당 |
| 통신 브리지 | WebMessage / postMessage | `w.bind`로 백엔드 함수를 노출, 프런트엔드가 `await window.fn()`으로 호출 |
| UI | Vue 3(로컬) | 단일 파일 애플리케이션 |
| 비주얼 시스템 | FairyGlass | 옅은 푸른빛 다크 리퀴드 글래스 디자인 시스템, 토큰을 인라인으로 `ui/index.html`에 복제 |
| 빌드 | CMake + Ninja | VS2022 "폴더 열기"에서 즉시 인식 |

**아키텍처: Core-Shell(범용 커널 + 네이티브 셸)**, 자매 프로젝트 FairySave와 동일: `src/platform/windows`는 Win32 특권 층(i18n 감지, 레지스트리 메뉴 인계), `ui/`는 표현 층입니다.

**호환성**: Windows 10 / 11(클래식 메뉴 인계는 Win11 기능). WebView2 런타임 필요(Win10/11에 내장).

---

## 개발과 빌드

**환경**: VS2022(C++ 데스크톱 워크로드, CMake, Ninja, Windows SDK 포함) + WebView2 런타임.

**가장 간단한 방법**: VS2022 "로컬 폴더 열기"에서 이 디렉터리를 선택하면, 자동으로 `CMakePresets.json`을 읽어 CMake 프로젝트로 인식, `windows-debug` 또는 `windows-release` 프리셋을 선택해 빌드·실행.

**명령줄 방식**(vcvars64 환경 아래):

```bat
cmake --preset windows-release
cmake --build build/release
```

산출물:

- `build/release/bin/ContextMenuMaster.exe` —— GUI 메인 프로그램(WIN32 하위 시스템, 콘솔 없음).
- `build/release/bin/CmmCli.exe` —— 명령줄 디버그 프로브(백엔드 검증용, 최종 사용자 대상 아님).

> `ui/`를 편집한 후 HTML만 변경했다면, `ui/`를 exe 옆의 `ui/` 디렉터리로 수동 동기화해야 합니다(CMake의 POST_BUILD는 다시 링크할 때만 트리거됩니다).

---

## 디렉터리 구조

```
ContextMenuMaster/
├── CMakeLists.txt              CMake 메인 구성(≈ .sln, /utf-8 /EHsc + Release 슬림화 포함)
├── CMakePresets.json           VS 원클릭 인식(windows-debug / release / release-x86 / release-arm64)
├── src/
│   ├── main.cpp                CLI 디버그 입구(CmmCli)
│   ├── app/
│   │   ├── gui_main.cpp        GUI 입구 WinMain: webview 창 + bind + 언어 주입
│   │   └── app.manifest        UAC requireAdministrator 권한 상승 매니페스트
│   ├── platform/windows/       Win32 특권 층(오른쪽 클릭 상황마다 모듈 하나)
│   │   ├── i18n.{h,cpp}          시스템 언어 감지(zh-CN / zh-TW / en)
│   │   ├── context_menu.{h,cpp}  Win11 클래식↔최신 메뉴 인계 + Explorer 우아한 재시작
│   │   ├── menu_scanner.{h,cpp}  레지스트리 상황 스캔 + 이름 변경/아이콘/켜기·끄기/일괄 접미사
│   │   ├── icon_extract.{h,cpp}  시스템 아이콘/리소스/.ico → PNG base64 data URI
│   │   ├── image_icon.{h,cpp}    임의 이미지 → 다중 크기 PNG-in-ICO(리치 미디어 단계 1)
│   │   ├── com_menu_probe.{h,cpp} IContextMenu COM 확장 동적 메뉴 탐지
│   │   ├── win11menu.{h,cpp}     Win11 최신 메뉴(PackagedCom / SystemCommandStore)
│   │   ├── winx_menu.{h,cpp}     Win+X 메뉴(파일 시스템 .lnk + hash)
│   │   ├── sendto_menu.{h,cpp}   보내기 하위 메뉴
│   │   ├── shellnew_menu.{h,cpp} 새로 만들기 하위 메뉴
│   │   ├── dragdrop_menu.{h,cpp} 끌어서 놓기 처리기
│   │   ├── openwith_menu.{h,cpp} 연결 프로그램
│   │   ├── custom_menu.{h,cpp}   직접 만든 오른쪽 클릭 메뉴 항목
│   │   └── filetype_menu.{h,cpp} 확장자별 파일 형식 오른쪽 클릭 항목 관리
│   └── shellext/               리치 미디어 Shell 확장 DLL(단계 2, cmm_core에 링크하지 않는 독립)
│       ├── shellext.cpp          IContextMenu 프로세스 내 COM 서버 + owner-draw 썸네일
│       └── shellext.def          COM 4개 내보내기
├── third_party/                webview 단일 헤더 라이브러리 + WebView2 SDK
└── ui/
    ├── index.html              단일 파일 Vue3 애플리케이션(리퀴드 글래스 스킨 + 3개 국어 i18n)
    ├── vue.global.js           Vue 3 런타임(로컬)
    └── liquidGlass.js          리퀴드 글래스 보조
```

---

## 개인정보와 보안

- **완전 로컬**: 네트워크에 연결하지 않고, 어떤 내용도 업로드하지 않습니다.
- **관리자 권한 필요**: 타사 소프트웨어의 메뉴 항목은 대부분 `HKLM`(모든 사용자)에 등록되고, 그 사용 안 함 표시도 대부분 `HKLM`에 있어, 순수한 사용자 권한으로는 변경할 수 없습니다. 그래서 프로그램 시작 시 관리자 권한을 요청하여, 메뉴 항목이 실제로 존재하는 레지스트리 위치를 직접 읽고 써서 "모든 항목을 켜기·끄기 / 이름 변경 가능"을 지원합니다.
- **쓰기 전 자동 백업**: 레지스트리를 변경할 때마다 먼저 `reg export`로 해당 키를 로컬 `%LOCALAPPDATA%\ContextMenuMaster\backups\`에 백업합니다.
- **되돌리기 가능**: 클래식 메뉴 인계는 언제든 키를 삭제해 복원. 켜기·끄기는 "사용 안 함 표시 추가/삭제 / 사용 안 함 미러로 이동" 방식으로, 원래대로 되돌릴 수 있습니다. 항목 삭제는 반드시 휴지통으로 보냅니다. 시스템 파일을 손상시키지 않습니다.

---

## 다운로드

- 미리 컴파일된 버전은 [Releases](https://github.com/Henglie/ContextMenuMaster/releases) 참조: CPU 아키텍처로 zip 선택(x64 / x86 / arm64), 압축 해제 후 `ContextMenuMaster.exe`를 두 번 클릭하면 바로 사용, 설치 불필요. 대부분의 PC는 x64 선택.
- WebView2 런타임 필요(Windows 10/11에 내장).

---

## 라이선스와 작성자

- **작성자**: 恒烈 (EternalBlaze / Henglie)
- **버전**: v0.6
- **프로젝트 홈페이지**: <https://github.com/Henglie/ContextMenuMaster>
- **라이선스**: MIT
