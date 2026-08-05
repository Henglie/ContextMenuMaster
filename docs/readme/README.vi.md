<div align="center">

<img src="../../res/app-256.png" width="128" height="128" alt="Context Menu Master" />

# Người quản lý menu chuột phải Context Menu Master

**Nắm lại quyền quản lý menu chuột phải của bạn trên Windows 11** · Giao diện kính lỏng · Hoàn toàn cục bộ, hoàn toàn có thể hoàn tác

[![Release](https://img.shields.io/badge/release-v0.6-4d9fff?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-6a5ae0?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)
[![License](https://img.shields.io/badge/license-MIT-22c55e?style=flat-square)](LICENSE)
[![Arch](https://img.shields.io/badge/arch-x64%20%7C%20x86%20%7C%20arm64-f59e0b?style=flat-square)](https://github.com/Henglie/ContextMenuMaster/releases)

<sub>Kiến trúc Core-Shell · Lõi C++17 · Frontend WebView2 · Giao diện 16 ngôn ngữ</sub>

</div>

---

[简体中文](../../README.md) | [繁體中文](README.zh-TW.md) | [English](../../README.en.md) | [日本語](README.ja.md) | [한국어](README.ko.md) | [Français](README.fr.md) | [Deutsch](README.de.md) | [Español](README.es.md) | [Português](README.pt.md) | [Русский](README.ru.md) | [Italiano](README.it.md) | [العربية](README.ar.md) | [עברית](README.he.md) | [हिन्दी](README.hi.md) | [ไทย](README.th.md) | **Tiếng Việt**

> Nắm lại quyền quản lý menu chuột phải của bạn trên Windows 11.

**Người quản lý menu chuột phải** là một công cụ tăng hiệu suất trên máy tính để bàn Windows. Nó trả lại cho bạn menu chuột phải vốn lộn xộn, bị gấp lại và bị đủ loại phần mềm nhồi nhét đầy ứng: đổi tên và biểu tượng một cách trực quan, quản lý chọn hàng loạt, khôi phục menu cổ điển bị ẩn của Windows 11 chỉ bằng một cú nhấp, và bao phủ toàn bộ các bối cảnh chuột phải bao gồm màn hình nền / tập tin / thư mục / ổ đĩa / thùng rác / Win+X / gửi tới / mới / kéo thả.

Chạy hoàn toàn cục bộ, tự động sao lưu registry trước mỗi thao tác ghi, mọi thao tác đều có thể hoàn tác bất cứ lúc nào. Chạy với quyền quản trị viên để quản lý các mục menu ở phạm vi hệ thống và tất cả người dùng (HKLM).

> [!WARNING]
> **Phần mở rộng shell đa phương tiện (`CmmShellExt.dll`) hiện đang trong giai đoạn thử nghiệm.** Đây là một phần mở rộng shell COM in-process chỉ có hiệu lực sau khi bạn tự đăng ký nó với quyền quản trị viên bằng `regsvr32 CmmShellExt.dll`; sau khi đăng ký, nhấp chuột phải vào một tập tin sẽ hiển thị các mục menu với hình thu nhỏ thật. Phần này chưa được kiểm chứng rộng rãi trên máy thật và có thể hoạt động không ổn định — hoặc không xuất hiện chút nào — trên một số hệ thống, loại tập tin hoặc môi trường phần mềm bảo mật. Nó đi kèm các cơ chế bảo vệ (kho lưu trữ trên 200MB hoặc liệt kê quá 800ms sẽ tự động hạ cấp xuống một mục menu thông thường, để menu không bao giờ bị treo), nhưng vẫn không nên để nó đăng ký lâu dài trong các môi trường quan trọng. Các tính năng quản lý menu của ứng dụng chính không phụ thuộc vào nó và có thể được sử dụng độc lập.

---

## Ảnh chụp màn hình giao diện

> Màn hình đầu tiên là chế độ xem trực quan: thanh bên trái trải phẳng toàn bộ các bối cảnh chuột phải, ở giữa trình bày "mô phỏng menu hiện đại chỉ đọc → menu cổ điển có thể chỉnh sửa" cạnh nhau.

![Giao diện chính chế độ xem trực quan](../screenshots/main-visual.png)

<table>
  <tr>
    <td width="50%"><img src="../screenshots/list-view.png" alt="Chế độ xem danh sách" /><br/><sub>Chế độ xem danh sách: hộp kiểm chọn hàng loạt, công tắc bật/tắt, thanh công cụ phép thuật hàng loạt</sub></td>
    <td width="50%"><img src="../screenshots/win11-zone.png" alt="Khu vực Windows 11" /><br/><sub>Khu vực Windows 11: đổi qua lại menu cổ điển/mới bằng một cú nhấp, khởi động lại Explorer một cách nhẹ nhàng</sub></td>
  </tr>
  <tr>
    <td width="50%"><img src="../screenshots/about.png" alt="Bối cảnh Mới ShellNew" /><br/><sub>Mới (ShellNew): quản lý những loại tập tin nào xuất hiện trong menu con "Mới"</sub></td>
    <td width="50%"><img src="../screenshots/desktop-visual.png" alt="Vùng trống màn hình nền trực quan" /><br/><sub>Vùng trống màn hình nền: chế độ xem trực quan mô phỏng menu chuột phải 1:1</sub></td>
  </tr>
  <tr>
    <td colspan="2" align="center"><img src="../screenshots/shellnew.png" alt="Trang giới thiệu" width="50%" /><br/><sub>Giới thiệu: phiên bản / giấy phép / tác giả và trang chủ dự án</sub></td>
  </tr>
</table>

---

## Nó giải quyết vấn đề gì

| Bối cảnh | Không có công cụ này | Có công cụ này |
|---|---|---|
| Menu chuột phải của Win11 quá tinh giản, các mục thường dùng bị gấp vào "Hiển thị thêm tùy chọn" | Mỗi lần phải nhấp thêm, chờ thêm | Đổi lại menu cổ điển đầy đủ bằng một cú nhấp, có hiệu lực tức thì |
| Phần mềm đã cài nhồi một đống mục vào menu chuột phải, vừa lộn xộn vừa dài | Chỉ có thể tự lật tìm trong registry | Chế độ xem danh sách chọn hàng loạt, quản lý trong nháy mắt |
| Muốn đổi cho mục menu một cái tên hoặc biểu tượng ưng mắt | Phải hiểu cấu trúc registry mới dám động vào | Trong chế độ xem trực quan, nhấp một cái là đổi |
| Muốn đồng loạt thêm một dấu hiệu cho một nhóm mục menu | Phải sửa thủ công từng cái một | Phép thuật hàng loạt, chọn xong thêm hậu tố bằng một cú nhấp |

---

## Chức năng cốt lõi

- **Bao phủ toàn bộ bối cảnh chuột phải**: một thanh bên trải phẳng dọc toàn bộ các bối cảnh chuột phải, nhấp một cái là chuyển đổi——
  - **Bối cảnh registry**: vùng trống màn hình nền, tập tin, thư mục, vùng trống thư mục, ổ đĩa, thùng rác, mọi đối tượng tập tin, PC này, thư viện.
  - **Menu Win+X**: quản lý menu nâng cao "chuột phải nút Bắt đầu / Win+X" (hiển thị theo nhóm, bật/tắt, thêm mục tùy chỉnh, xóa).
  - **Gửi tới (SendTo)**: quản lý các mục trong menu con "Gửi tới" của chuột phải, có thể bật/tắt, thêm, xóa.
  - **Mới (ShellNew)**: quản lý những loại tập tin nào xuất hiện trong menu con "Mới" của chuột phải, bật/tắt từng mục.
  - **Trình xử lý kéo thả (DragDrop)**: quản lý các phần mở rộng menu bật lên khi kéo thả tập tin bằng chuột phải.
  - **Mở bằng (Open with)**: xem các ứng dụng được đăng ký làm ứng viên "Mở bằng" (chỉ đọc).
- **Hai chế độ xem đồng bộ thời gian thực**
  - **Chế độ xem trực quan (WYSIWYG)**: mô phỏng menu chuột phải thật của Windows theo tỷ lệ 1:1, nhấp tên để đổi tên, nhấp biểu tượng để đổi biểu tượng, **nhấp chuột phải vào mục menu để bật menu thao tác (đổi tên / đổi biểu tượng / bật tắt)**, thấy gì được nấy.
  - **Chế độ xem danh sách (List View)**: bảng truyền thống, hộp kiểm chọn hàng loạt, công tắc bật/tắt, nút đổi tên và đổi biểu tượng đầy đủ, thích hợp để quản lý một loạt mục menu cùng lúc.
- **Phép thuật hàng loạt**: chọn một số mục menu, thêm cùng một hậu tố cho tất cả bằng một cú nhấp (như " meo~", " ★"), cũng có thể tùy chỉnh.
- **Khu vực Windows 11**
  - **Đổi menu cổ điển bằng một cú nhấp**: ghi `InprocServer32` rỗng vào HKCU để làm phần mở rộng menu mới mất hiệu lực, khiến trình quản lý tập tin lùi về menu cổ điển kiểu Win10; đồng thời khởi động lại `explorer.exe` một cách nhẹ nhàng, menu có hiệu lực tức thì, không cần đăng xuất đăng nhập lại. Nhấp thêm một cái là khôi phục.
  - **Phần mở rộng menu mới (đang phát triển)**: dành sẵn giao diện để quét và tiếp quản các mục phần mở rộng `IExplorerCommand` gói thưa (MSIX) của Win11.
- **Đa ngôn ngữ**: khi khởi động lần đầu tự động chọn theo ngôn ngữ hiển thị của hệ thống (tiếng Trung giản/phồn thể, còn lại đều dùng tiếng Anh), giao diện có thể chuyển đổi bất cứ lúc nào.
- **Hoàn toàn cục bộ, có thể hoàn tác**: chỉ đọc ghi registry của người dùng hiện tại, không kết nối mạng, không tải lên, xóa khóa là khôi phục mặc định hệ thống.

---

## Bắt đầu nhanh

1. Tải zip theo kiến trúc tương ứng từ [Releases](https://github.com/Henglie/ContextMenuMaster/releases) (đa số máy tính chọn x64), giải nén vào thư mục bất kỳ.
2. Nhấp đúp `ContextMenuMaster.exe`, nhấp "Có" trong hộp thông báo UAC bật lên để cấp quyền quản trị viên (dùng để đọc ghi các mục menu do phần mềm bên thứ ba ghi vào `HKLM`, nếu không nâng quyền thì chỉ sửa được các mục của người dùng hiện tại).
3. Không cần cài đặt, không cần kết nối mạng. Lần chạy đầu tiên sẽ tự động chọn ngôn ngữ giao diện theo ngôn ngữ hệ thống, góc trên bên phải chuyển đổi giữa giản / phồn / English bất cứ lúc nào.

---

## Cách sử dụng

### 1 · Chọn bối cảnh

Thanh bên trái trải phẳng dọc toàn bộ các bối cảnh chuột phải, chia thành ba nhóm:

- **Bối cảnh thường dùng**: vùng trống màn hình nền, tập tin, thư mục, vùng trống thư mục, ổ đĩa, thùng rác, mọi đối tượng tập tin, PC này, thư viện —— tương ứng với menu bạn thấy khi nhấp chuột phải trên các đối tượng khác nhau.
- **Menu cấp hai**: gửi tới, mới, kéo thả, mở bằng, loại tập tin —— các menu con trong menu chuột phải và quản lý theo phần mở rộng tên tập tin.
- **Lối vào nhanh**: Win+X (menu nâng cao khi chuột phải nút Bắt đầu).

Phía trên còn có hai lối vào riêng là **Khu vực Windows 11** và **Đa phương tiện**. Nhấp bối cảnh bất kỳ để quét các mục menu thật của bối cảnh đó, huy hiệu góc của thẻ hiển thị số lượng mục.

### 2 · Sửa menu

Hai chế độ xem, chuyển đổi ở góc trên bên phải, dữ liệu đồng bộ thời gian thực:

- **Chế độ xem trực quan**: mô phỏng menu chuột phải Windows theo tỷ lệ 1:1.
  - Nhấp **tên** mục menu để đổi tên trực tiếp; nhấp **biểu tượng** để đổi biểu tượng.
  - **Nhấp chuột phải** vào mục menu bất kỳ để bật menu thao tác: đổi tên / đổi biểu tượng / bật tắt / xóa (mục tự tạo).
  - Phía menu hiện đại là mô phỏng chỉ đọc (menu hiện đại của Win11 không do registry kiểm soát, xem giải thích bên dưới), phía menu cổ điển có thể chỉnh sửa.
- **Chế độ xem danh sách**: bảng truyền thống, thích hợp cho thao tác hàng loạt.
  - Mỗi hàng có công tắc bật/tắt, nút đổi tên / đổi biểu tượng.
  - Sau khi chọn nhiều hàng, dùng **phép thuật hàng loạt** thêm cùng một hậu tố cho tất cả bằng một cú nhấp (như " ★"), cũng có thể tùy chỉnh.

Đổi tên / đổi biểu tượng / bật tắt đối với các mục tích hợp của hệ thống sẽ có xác nhận lần hai; trước mỗi lần ghi registry tự động sao lưu vào `%LOCALAPPDATA%\ContextMenuMaster\backups\`, sửa hỏng có thể khôi phục.

### 3 · Khôi phục menu cổ điển (Windows 11)

Vào **Khu vực Windows 11**, nhấp "Chuyển sang menu cổ điển": chương trình sẽ làm phần mở rộng menu mới mất hiệu lực và khởi động lại `explorer.exe` một cách nhẹ nhàng, menu chuột phải tức thì trở về kiểu đầy đủ của Win10, không cần đăng xuất đăng nhập lại. Nhấp thêm một cái là khôi phục menu mới.

### 4 · Tạo mục menu của riêng bạn

Trên thanh công cụ chế độ xem danh sách nhấp "Tạo mục menu", điền tên hiển thị, loại thực thi (chạy chương trình / mở thư mục / mở trang web / dòng lệnh tùy chỉnh), đường dẫn đích, biểu tượng (tùy chọn, hỗ trợ bất kỳ hình ảnh nào tự động chuyển thành ICO), vị trí (trên cùng / dưới cùng). Mục tự tạo có dấu hiệu riêng, có thể chỉnh sửa hoặc xóa bất cứ lúc nào, việc xóa chỉ dọn các mục do công cụ này tạo, không đụng đến liên kết hệ thống.

> **Về menu hiện đại của Win11**: menu chuột phải mới của Windows 11 chỉ chấp nhận các phần mở rộng `IExplorerCommand` có định danh gói (MSIX / gói thưa), các mục menu registry truyền thống đều bị gấp vào menu cổ điển dưới "Hiển thị thêm tùy chọn" —— đây là thiết kế của hệ thống, không liên quan đến quyền. Vì vậy công cụ này quản lý ở lớp menu cổ điển; muốn cho các mục thường dùng trở về lớp trên cùng, hãy dùng "Khôi phục menu cổ điển" ở trên.

---

## Đa ngôn ngữ

Backend C++ dùng Win32 API đọc ngôn ngữ hiển thị mặc định của hệ thống, tiêm cho frontend trước khi tạo trang WebView2:

- Hệ thống là tiếng Trung (zh-CN / zh-TW / zh-HK v.v.) → hiển thị giao diện tiếng Trung theo giản hoặc phồn thể.
- Hệ thống là bất kỳ ngôn ngữ nào khác → đều lùi về tiếng Anh (en).

Góc trên bên phải giao diện có thể chuyển đổi thủ công giữa giản thể / phồn thể / English bất cứ lúc nào, lựa chọn được ghi vào bộ lưu trữ cục bộ.

---

## Ngăn xếp công nghệ và kiến trúc

| Lớp | Lựa chọn | Mô tả |
|---|---|---|
| Engine backend | C++17 hiện đại / MSVC | Gọi trực tiếp Win32 API bản địa |
| Lớp trình bày | Microsoft Edge WebView2 | Thư viện header đơn webview chứa WebView2 tích hợp sẵn của hệ thống |
| Cầu nối giao tiếp | WebMessage / postMessage | `w.bind` phơi bày hàm backend, frontend gọi bằng `await window.fn()` |
| Giao diện | Vue 3 (cục bộ) | Ứng dụng tập tin đơn |
| Hệ thống thị giác | FairyGlass | Hệ thống thiết kế kính lỏng tông tối xanh nhạt, token được sao chép nội tuyến vào `ui/index.html` |
| Xây dựng | CMake + Ninja | VS2022 "Mở thư mục" là nhận diện ngay |

**Kiến trúc: Core-Shell (nhân phổ quát + vỏ bản địa)**, nhất quán với dự án chị em FairySave: `src/platform/windows` là lớp đặc quyền Win32 (phát hiện i18n, tiếp quản menu registry), `ui/` là lớp trình bày.

**Tính tương thích**: Windows 10 / 11 (tiếp quản menu cổ điển là tính năng của Win11). Cần WebView2 runtime (Win10/11 có sẵn).

---

## Phát triển và xây dựng

**Môi trường**: VS2022 (bao gồm khối lượng công việc desktop C++, CMake, Ninja, Windows SDK) + WebView2 runtime.

**Cách đơn giản nhất**: VS2022 "Mở thư mục cục bộ" chọn thư mục này, tự động đọc `CMakePresets.json` nhận diện là dự án CMake, chọn preset `windows-debug` hoặc `windows-release` để tạo và chạy.

**Cách dòng lệnh** (trong môi trường vcvars64):

```bat
cmake --preset windows-release
cmake --build build/release
```

Sản phẩm:

- `build/release/bin/ContextMenuMaster.exe` —— chương trình chính GUI (hệ con WIN32, không có console).
- `build/release/bin/CmmCli.exe` —— đầu dò gỡ lỗi dòng lệnh (dùng để xác minh backend, không hướng đến người dùng cuối).

> Sau khi sửa `ui/` nếu chỉ động vào HTML, cần tự thủ công đồng bộ `ui/` vào thư mục `ui/` cạnh exe (POST_BUILD của CMake chỉ kích hoạt khi liên kết lại).

---

## Cấu trúc thư mục

```
ContextMenuMaster/
├── CMakeLists.txt              Cấu hình chính CMake (≈ .sln, gồm /utf-8 /EHsc + tối giản Release)
├── CMakePresets.json           VS nhận diện một cú nhấp (windows-debug / release / release-x86 / release-arm64)
├── src/
│   ├── main.cpp                Điểm vào gỡ lỗi CLI (CmmCli)
│   ├── app/
│   │   ├── gui_main.cpp        Điểm vào GUI WinMain: cửa sổ webview + bind + tiêm ngôn ngữ
│   │   └── app.manifest        Manifest nâng quyền UAC requireAdministrator
│   ├── platform/windows/       Lớp đặc quyền Win32 (mỗi bối cảnh chuột phải một mô-đun)
│   │   ├── i18n.{h,cpp}          Phát hiện ngôn ngữ hệ thống (zh-CN / zh-TW / en)
│   │   ├── context_menu.{h,cpp}  Tiếp quản menu cổ điển↔mới của Win11 + khởi động lại Explorer nhẹ nhàng
│   │   ├── menu_scanner.{h,cpp}  Quét bối cảnh registry + đổi tên/biểu tượng/bật tắt/hậu tố hàng loạt
│   │   ├── icon_extract.{h,cpp}  Biểu tượng hệ thống/tài nguyên/.ico → PNG base64 data URI
│   │   ├── image_icon.{h,cpp}    Hình ảnh bất kỳ → PNG-in-ICO nhiều kích thước (đa phương tiện giai đoạn 1)
│   │   ├── com_menu_probe.{h,cpp} Dò menu động của phần mở rộng COM IContextMenu
│   │   ├── win11menu.{h,cpp}     Menu hiện đại Win11 (PackagedCom / SystemCommandStore)
│   │   ├── winx_menu.{h,cpp}     Menu Win+X (.lnk hệ thống tập tin + hash)
│   │   ├── sendto_menu.{h,cpp}   Menu con gửi tới
│   │   ├── shellnew_menu.{h,cpp} Menu con mới
│   │   ├── dragdrop_menu.{h,cpp} Trình xử lý kéo thả
│   │   ├── openwith_menu.{h,cpp} Mở bằng
│   │   ├── custom_menu.{h,cpp}   Mục menu chuột phải tự tạo
│   │   └── filetype_menu.{h,cpp} Quản lý mục chuột phải loại tập tin theo phần mở rộng tên
│   └── shellext/               DLL phần mở rộng Shell đa phương tiện (giai đoạn 2, độc lập không liên kết cmm_core)
│       ├── shellext.cpp          Máy chủ COM trong tiến trình IContextMenu + hình thu nhỏ owner-draw
│       └── shellext.def          Bốn xuất COM
├── third_party/                Thư viện header đơn webview + WebView2 SDK
└── ui/
    ├── index.html              Ứng dụng Vue3 tập tin đơn (giao diện kính lỏng + i18n ba ngôn ngữ)
    ├── vue.global.js           Vue 3 runtime (cục bộ)
    └── liquidGlass.js          Trợ giúp kính lỏng
```

---

## Quyền riêng tư và bảo mật

- **Hoàn toàn cục bộ**: không kết nối mạng, không tải lên bất kỳ nội dung nào.
- **Cần quyền quản trị viên**: các mục menu của phần mềm bên thứ ba thường đăng ký trong `HKLM` (tất cả người dùng), dấu vô hiệu hóa của chúng cũng phần lớn ở `HKLM`, chế độ người dùng thuần không sửa được. Vì vậy khi khởi động chương trình xin quyền quản trị viên, đọc ghi trực tiếp vị trí registry thật nơi mục menu tồn tại, để hỗ trợ "mọi mục đều bật tắt / đổi tên được".
- **Tự động sao lưu trước khi ghi**: trước mỗi lần sửa registry `reg export` sao lưu khóa tương ứng vào `%LOCALAPPDATA%\ContextMenuMaster\backups\` cục bộ.
- **Có thể hoàn tác**: tiếp quản menu cổ điển bất cứ lúc nào xóa khóa là khôi phục; bật tắt đi theo "thêm dấu xóa / chuyển sang ảnh vô hiệu hóa", có thể chuyển về nguyên trạng; xóa mục đều đưa vào thùng rác. Không phá hỏng tập tin hệ thống.

---

## Tải xuống

- Phiên bản biên dịch sẵn xem tại [Releases](https://github.com/Henglie/ContextMenuMaster/releases): chọn zip theo kiến trúc CPU (x64 / x86 / arm64), giải nén xong nhấp đúp `ContextMenuMaster.exe` là dùng, không cần cài đặt. Đại đa số máy tính chọn x64.
- Cần WebView2 runtime (Windows 10/11 có sẵn).

---

## Giấy phép và tác giả

- **Tác giả**: 恒烈 (EternalBlaze / Henglie)
- **Phiên bản**: v0.6
- **Trang chủ dự án**: <https://github.com/Henglie/ContextMenuMaster>
- **Giấy phép**: MIT
