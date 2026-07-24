@echo off
chcp 65001 >nul

REM ============================================================
REM Clean all third-party library build output directories
REM ============================================================

setlocal

SET SCRIPT_DIR=%~dp0

echo Cleaning build output directories...
echo.

REM libjpeg-turbo
IF EXIST "%SCRIPT_DIR%libjpeg-turbo" (
    echo + Cleaning libjpeg-turbo
    rd /s /q "%SCRIPT_DIR%libjpeg-turbo"
)

REM libpng-apng
rd /s /q "%SCRIPT_DIR%..\third_party\libpng-apng\libpng\projects\vstudio\Release Library" 2>nul
rd /s /q "%SCRIPT_DIR%..\third_party\libpng-apng\libpng\projects\vstudio\x64\Release Library" 2>nul

REM libwebp
IF EXIST "%SCRIPT_DIR%libwebp" (
    echo + Cleaning libwebp
    rd /s /q "%SCRIPT_DIR%libwebp"
)

REM lcms2
IF EXIST "%SCRIPT_DIR%lcms2" (
    echo + Cleaning lcms2
    rd /s /q "%SCRIPT_DIR%lcms2"
)

REM libjxl
IF EXIST "%SCRIPT_DIR%libjxl" (
    echo + Cleaning libjxl
    rd /s /q "%SCRIPT_DIR%libjxl"
)

REM libraw makefile - search for Makefile.msvc-* in the parent script dir
pushd "%SCRIPT_DIR%..\third_party\LibRaw"
for %%F in (Makefile.msvc-*) do (
    IF EXIST "%SCRIPT_DIR%..\third_party\LibRaw\%%~nF" (
        echo + Cleaning %%~nF
        rd /s /q "%SCRIPT_DIR%..\third_party\LibRaw\%%~nF"
    )
)
popd

REM libheif_libavif
IF EXIST "%SCRIPT_DIR%libheif_libavif" (
    echo + Cleaning libheif_libavif
    rd /s /q "%SCRIPT_DIR%libheif_libavif"
)

REM venv
IF EXIST "%SCRIPT_DIR%venv-meson" (
    echo + Cleaning venv-meson
    rd /s /q "%SCRIPT_DIR%venv-meson"
)

echo.
echo Clean completed!

exit /b 0
