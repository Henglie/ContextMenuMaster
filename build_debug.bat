@echo off
chcp 65001 >nul
rem ============================================================
rem  ContextMenuMaster - debug build bat (mirrors FairySave's convention)
rem  ASCII-only comments on purpose: cmd caches the file read offset in
rem  bytes, and a mid-file `chcp 65001` makes it mis-seek across any
rem  multibyte (Chinese) comment line, turning a fragment into a command.
rem  Keeping comments ASCII removes that glitch entirely.
rem  - VS-bundled cmake + ninja, vcvars64 first to set up MSVC env.
rem  - call from bash as: cmd.exe //c build_debug.bat  (double slash vs MSYS)
rem  - must stay CRLF; otherwise cmd treats "D:\Program" as a command.
rem ============================================================

set "VSROOT=D:\Program Files\VS"
set "CMAKEBIN=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
set "NINJABIN=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "PATH=%CMAKEBIN%;%NINJABIN%;%PATH%"

call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul || goto :err

cd /d "%~dp0" || goto :err

rem Kill any running instance first; a locked exe causes LNK1168 on relink.
taskkill /F /IM ContextMenuMaster.exe >nul 2>&1

cmake --preset windows-debug || goto :err
cmake --build build/debug || goto :err

echo.
echo ===== BUILD OK =====
goto :eof

:err
echo.
echo ===== BUILD FAILED (errorlevel %errorlevel%) =====
exit /b 1
