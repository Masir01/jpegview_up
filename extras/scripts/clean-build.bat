@echo off
chcp 65001 >nul

REM ============================================================
REM 清理所有依赖库的编译输出目录
REM ============================================================

setlocal

SET SCRIPT_DIR=%~dp0

echo 清理编译输出目录...
echo.

REM libjpeg-turbo
IF EXIST "%SCRIPT_DIR%libjpeg-turbo" (
    echo + 清理 libjpeg-turbo
    rd /s /q "%SCRIPT_DIR%libjpeg-turbo"
)

REM libpng-apng (不需要清理，脚本会检查)
rd /s /q "%SCRIPT_DIR%..\third_party\libpng-apng\libpng\projects\vstudio\Release Library" 2>nul
rd /s /q "%SCRIPT_DIR%..\third_party\libpng-apng\libpng\projects\vstudio\x64\Release Library" 2>nul

REM libwebp
IF EXIST "%SCRIPT_DIR%libwebp" (
    echo + 清理 libwebp
    rd /s /q "%SCRIPT_DIR%libwebp"
)

REM lcms2
IF EXIST "%SCRIPT_DIR%lcms2" (
    echo + 清理 lcms2
    rd /s /q "%SCRIPT_DIR%lcms2"
)

REM libjxl
IF EXIST "%SCRIPT_DIR%libjxl" (
    echo + 清理 libjxl
    rd /s /q "%SCRIPT_DIR%libjxl"
)

REM libraw makefile
for %%F in ("Makefile.msvc-*") do (
    IF EXIST "%SCRIPT_DIR%..\third_party\LibRaw\%%~nF" (
        echo + 清理 %%~nF
        rd /s /q "%SCRIPT_DIR%..\third_party\LibRaw\%%~nF"
    )
)

REM libheif_libavif
IF EXIST "%SCRIPT_DIR%libheif_libavif" (
    echo + 清理 libheif_libavif
    rd /s /q "%SCRIPT_DIR%libheif_libavif"
)

REM venv
IF EXIST "%SCRIPT_DIR%venv-meson" (
    echo + 清理 venv-meson
    rd /s /q "%SCRIPT_DIR%venv-meson"
)

echo.
echo 清理完成!

exit /b 0
