# ============================================================
#  鼠标右键主理人 ContextMenuMaster —— CMake 主配置
#
#  这个文件相当于 Visual Studio 的 .sln + .vcxproj：你在属性页里用鼠标
#  点的设置，这里用文字写。CMake 读完它生成真正的工程给 VS / Ninja 用。
#  约定对标 FairySave（同一作者的姊妹项目）：/utf-8 /EHsc + Release 瘦身。
# ============================================================
cmake_minimum_required(VERSION 3.20)

project(ContextMenuMaster
    VERSION 0.1.0
    LANGUAGES CXX
)

# --- 全局编译设置 ---
set(CMAKE_CXX_STANDARD 17)          # C++17（对标姊妹项目）
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制要求，不允许降级
set(CMAKE_CXX_EXTENSIONS OFF)       # 关编译器私有扩展，保持标准

# .exe / .lib 统一输出到 build/<cfg>/bin、build/<cfg>/lib，方便找
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(MSVC)
    # /utf-8 ：按 UTF-8 读源文件（含中文注释），否则 MSVC 按 GBK 误读破坏语法。
    #          写中文注释的项目几乎必加（FairySave 踩坑记录第 2 条）。
    # /EHsc  ：启用 C++ 异常展开语义。代码用了 std::string 等，发布版必须开，
    #          否则栈展开（局部对象析构）不保证 + C4530 警告（踩坑记录第 3 条）。
    add_compile_options(/utf-8 /EHsc)

    # ========================================================
    #  发布版瘦身 / 提速（仅 Release 生效，Debug 保持可调试不优化）
    #    /O2 最大化速度  /Gy 函数级 COMDAT  /Gw 数据级 COMDAT  /GL 全程序优化
    #  链接：/OPT:REF 剔除未引用  /OPT:ICF 折叠相同函数  /LTCG 链接期代码生成
    #  原生 Win32 + WebView2，无重型依赖，瘦身后单 exe 可压到很小。
    # ========================================================
    add_compile_options(
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/Gy>
        $<$<CONFIG:Release>:/Gw>
        $<$<CONFIG:Release>:/GL>
    )
    add_link_options(
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        $<$<CONFIG:Release>:/LTCG>
    )
endif()

# ============================================================
#  核心静态库：共享模块编进一个库，GUI 与 CLI 都链接它，避免重复编译。
#  本项目目前只有平台特权层（i18n + 注册表菜单接管），后续 core/adapters 在此追加。
# ============================================================
add_library(cmm_core STATIC
    # --- 平台特权层（Windows / Win32）---
    src/platform/windows/i18n.cpp
    src/platform/windows/context_menu.cpp
    src/platform/windows/icon_extract.cpp
    src/platform/windows/menu_scanner.cpp
    src/platform/windows/com_menu_probe.cpp
)

target_include_directories(cmm_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

if(WIN32)
    # advapi32：注册表 RegCreateKeyEx/RegDeleteTree 等（菜单接管核心）。
    # gdiplus：系统图标提取后编码 PNG（icon_extract）。
    # crypt32：CryptBinaryToStringA 把 PNG 转 base64 data URI（icon_extract）。
    # 其余为 WebView2/COM/Shell 相关，对标 FairySave 的链接集。
    target_link_libraries(cmm_core PUBLIC
        advapi32 user32 shell32 gdi32 gdiplus crypt32
        ole32 oleaut32 version shlwapi
    )
    target_compile_definitions(cmm_core PUBLIC
        _WIN32_WINNT=0x0A00      # 目标 Win10/11（RegDeleteTreeW 需 Vista+，此处取更高）
        UNICODE _UNICODE
    )
endif()

# ============================================================
#  GUI 程序：ContextMenuMaster（WIN32 子系统 = 无控制台黑框窗口）
#  用 webview 单头库 + 系统自带 WebView2 承载 Vue 界面。
# ============================================================
add_executable(ContextMenuMaster WIN32
    src/app/gui_main.cpp
)
target_link_libraries(ContextMenuMaster PRIVATE cmm_core)

# webview 单头库 + WebView2 SDK 头文件路径（结构同 FairySave）。
# 代码 include 写的是 "webview/webview.h"，故 include 根指向 third_party（上一级），
# 让它解析成 third_party/webview/webview.h，而不是指到 webview 目录本身。
target_include_directories(ContextMenuMaster PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/webview2/build/native/include
)

# 把 ui 目录拷到 exe 旁，让 file:// 能加载界面。
# 注意（FairySave 踩坑第 4 条）：只改 ui/*.html 而不重新链接时 ninja 判定
# "no work to do"，POST_BUILD 不触发 → 需手动同步 ui 到 bin。
add_custom_command(TARGET ContextMenuMaster POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/ui $<TARGET_FILE_DIR:ContextMenuMaster>/ui
)

# ============================================================
#  命令行调试程序：CmmCli（仅用于无界面验证后端，对标 FairySaveCli）
# ============================================================
add_executable(CmmCli
    src/main.cpp
)
target_link_libraries(CmmCli PRIVATE cmm_core)
