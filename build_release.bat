@echo off
chcp 65001 >nul
rem ============================================================
rem  ContextMenuMaster - release build bat (parameterized by arch)
rem  ASCII-only comments on purpose: cmd caches the file read offset in
rem  bytes, and a mid-file `chcp 65001` makes it mis-seek across any
rem  multibyte (Chinese) comment line, turning a fragment into a command.
rem
rem  Usage (from bash):  cmd.exe //c build_release.bat x64
rem                      cmd.exe //c build_release.bat x86
rem                      cmd.exe //c build_release.bat arm64
rem  Default arch when omitted: x64.
rem
rem  Host is x64, so x86 and arm64 use the x64-hosted cross toolchains
rem  (vcvarsamd64_x86 / vcvarsamd64_arm64). Must stay CRLF.
rem ============================================================

set "ARCH=%~1"
if "%ARCH%"=="" set "ARCH=x64"

set "VSROOT=D:\Program Files\VS"
set "CMAKEBIN=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
set "NINJABIN=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
set "PATH=%CMAKEBIN%;%NINJABIN%;%PATH%"

rem Pick vcvars script + CMake preset by arch.
if /i "%ARCH%"=="x64"   ( set "VCVARS=vcvars64.bat"          & set "PRESET=windows-release"       & set "BUILDDIR=build/release"       & goto :archok )
if /i "%ARCH%"=="x86"   ( set "VCVARS=vcvarsamd64_x86.bat"   & set "PRESET=windows-release-x86"   & set "BUILDDIR=build/release-x86"   & goto :archok )
if /i "%ARCH%"=="arm64" ( set "VCVARS=vcvarsamd64_arm64.bat" & set "PRESET=windows-release-arm64" & set "BUILDDIR=build/release-arm64" & goto :archok )
echo Unknown arch "%ARCH%" (use x64 / x86 / arm64).
exit /b 2

:archok
echo === Building Release %ARCH% (preset %PRESET%, vcvars %VCVARS%) ===

call "%VSROOT%\VC\Auxiliary\Build\%VCVARS%" >nul || goto :err

cd /d "%~dp0" || goto :err

rem Kill any running instance first; a locked exe causes LNK1168 on relink.
taskkill /F /IM ContextMenuMaster.exe >nul 2>&1

cmake --preset %PRESET% || goto :err
cmake --build %BUILDDIR% || goto :err

echo.
echo ===== BUILD OK (%ARCH%) =====
goto :eof

:err
echo.
echo ===== BUILD FAILED (%ARCH%, errorlevel %errorlevel%) =====
exit /b 1
