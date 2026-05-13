@echo off
chcp 65001 >nul

REM ============================================================
REM JPEGView 依赖库编译脚本
REM 依次编译所有必需的第三方库
REM ============================================================

setlocal enabledelayedexpansion

SET SCRIPT_DIR=%~dp0
SET EXTRAS_DIR=%SCRIPT_DIR%..
SET OUTPUT_BASE=%EXTRAS_DIR%

echo ============================================================
echo  JPEGView 依赖库编译脚本
echo ============================================================
echo.

REM 设置 Visual Studio 版本 (默认2019，可通过环境变量覆盖)
IF "%XVS_INIT_VER%"=="" (
    SET XVS_INIT_VER=2019
)
echo Visual Studio 版本: %XVS_INIT_VER%
echo.

REM 定义要编译的库列表
SET BUILD_ORDER=libjpegturbo libpng-apng libwebp lcms2 libjxl libraw libheif_libavif

REM 编译每个库
for %%L in (%BUILD_ORDER%) do (
    echo ============================================================
    echo 开始编译: %%L
    echo ============================================================
    
    set "SCRIPT="
    
    IF "%%L"=="libjpegturbo" (
        set "SCRIPT=%SCRIPT_DIR%build-libjpegturbo.bat"
    ) else IF "%%L"=="libpng-apng" (
        set "SCRIPT=%SCRIPT_DIR%build-libpng-apng.bat"
    ) else IF "%%L"=="libwebp" (
        set "SCRIPT=%SCRIPT_DIR%build-libwebp.bat"
    ) else IF "%%L"=="lcms2" (
        set "SCRIPT=%SCRIPT_DIR%build-lcms2.bat"
    ) else IF "%%L"=="libjxl" (
        set "SCRIPT=%SCRIPT_DIR%build-libjxl.bat"
    ) else IF "%%L"=="libraw" (
        set "SCRIPT=%SCRIPT_DIR%build-libraw.bat"
    ) else IF "%%L"=="libheif_libavif" (
        set "SCRIPT=%SCRIPT_DIR%build-libheif_libavif.bat"
    )
    
    IF NOT "!SCRIPT!"=="" (
        IF EXIST "!SCRIPT!" (
            call "!SCRIPT!"
            IF ERRORLEVEL 1 (
                echo.
                echo [ERROR] %%L 编译失败!
                exit /b 1
            )
        ) else (
            echo [WARNING] 脚本不存在: !SCRIPT!
        )
    )
    
    echo.
)

echo ============================================================
echo  所有依赖库编译完成!
echo ============================================================
echo.
echo 请手动复制头文件到 src\JPEGView 目录:
echo   - libjpeg-turbo\include ^<- extras\third_party\libjpeg-turbo
echo   - libpng-apng\include ^<- extras\third_party\libpng-apng\libpng
echo   - libwebp\include ^<- extras\third_party\libwebp\src\webp
echo   - lcms2\include ^<- extras\third_party\Little-CMS\include
echo   - libjxl\include\jxl ^<- extras\third_party\libjxl\lib\include\jxl
echo   - libraw\include ^<- extras\third_party\LibRaw\libraw
echo   - libheif\include ^<- extras\third_party\libheif\libheif\libheif
echo   - libavif\include ^<- extras\third_party\libavif\include\avif

exit /b 0
