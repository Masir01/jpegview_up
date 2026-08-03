@echo off
chcp 65001 >nul

REM ============================================================
REM JPEGView - Build all third-party libraries
REM ============================================================

setlocal enabledelayedexpansion

SET SCRIPT_DIR=%~dp0
SET EXTRAS_DIR=%SCRIPT_DIR%..
SET SRC_DIR=%EXTRAS_DIR%\src\JPEGView

echo ============================================================
echo  JPEGView Library Build Script
echo ============================================================
echo.

REM Check for NASM (required for libjpeg-turbo and libheif/libavif)
echo Checking prerequisites...
where nasm.exe >nul 2>&1
IF ERRORLEVEL 1 (
    echo [ERROR] NASM not found on PATH. Please install NASM.
    exit /b 1
)
echo [OK] NASM found

REM Check for Python (required for libheif/libavif with Meson)
where python.exe >nul 2>&1
IF ERRORLEVEL 1 (
    echo [WARNING] Python not found, some builds may fail
)

echo.
echo ============================================================
echo  Step 1: Cleaning previous builds
echo ============================================================

call "%SCRIPT_DIR%clean-build.bat"
IF ERRORLEVEL 1 (
    echo [WARNING] Clean script returned non-zero, continuing...
)

echo.
echo ============================================================
echo  Step 2: Building libjpeg-turbo 3.1.4.1
echo ============================================================
call "%SCRIPT_DIR%build-libjpegturbo.bat"
IF ERRORLEVEL 1 (
    echo [ERROR] libjpeg-turbo build failed
    exit /b 1
)

echo.
echo ============================================================
echo  Step 3: Building libpng-apng 1.6.58
echo ============================================================
call "%SCRIPT_DIR%build-libpng-apng.bat"
IF ERRORLEVEL 1 (
    echo [ERROR] libpng-apng build failed
    exit /b 1
)

echo.
echo ============================================================
echo  Step 4: Building libwebp
echo ============================================================
call "%SCRIPT_DIR%build-libwebp.bat"
IF ERRORLEVEL 1 (
    echo [ERROR] libwebp build failed
    exit /b 1
)

echo.
echo ============================================================
echo  Step 5: Building lcms2
echo ============================================================
call "%SCRIPT_DIR%build-lcms2.bat"
IF ERRORLEVEL 1 (
    echo [ERROR] lcms2 build failed
    exit /b 1
)

echo.
echo ============================================================
echo  Step 6: Building libjxl (this takes a while...)
echo ============================================================
call "%SCRIPT_DIR%build-libjxl.bat"
IF ERRORLEVEL 1 (
    echo [ERROR] libjxl build failed
    exit /b 1
)

echo.
echo ============================================================
echo  Step 7: Building libraw
echo ============================================================
call "%SCRIPT_DIR%build-libraw.bat"
IF ERRORLEVEL 1 (
    echo [ERROR] libraw build failed
    exit /b 1
)

echo.
echo ============================================================
echo  Step 8: Building libheif and libavif
echo ============================================================
call "%SCRIPT_DIR%build-libheif_libavif.bat"
IF ERRORLEVEL 1 (
    echo [ERROR] libheif/libavif build failed
    exit /b 1
)

echo.
echo ============================================================
echo  Step 9: Building DirectXTex (DDS format support)
echo ============================================================
call "%SCRIPT_DIR%build-DirectXTex.bat"
IF ERRORLEVEL 1 (
    echo [ERROR] DirectXTex build failed
    exit /b 1
)

echo.
echo ============================================================
echo  Step 10: Copying header files
echo ============================================================

echo Copying libjpeg-turbo headers...
xcopy /y /e "%EXTRAS_DIR%\third_party\libjpeg-turbo\src" "%SRC_DIR%\libjpeg-turbo\include\" >nul 2>&1
xcopy /y "%SCRIPT_DIR%libjpeg-turbo\lib\jconfig.h" "%SRC_DIR%\libjpeg-turbo\include\" >nul 2>&1

echo Copying libpng-apng headers...
xcopy /y /e "%EXTRAS_DIR%\third_party\libpng-apng\libpng\*.h" "%SRC_DIR%\libpng-apng\include\" >nul 2>&1

echo Copying libwebp headers...
xcopy /y /e "%EXTRAS_DIR%\third_party\libwebp\src\webp" "%SRC_DIR%\libwebp\include\webp\" >nul 2>&1

echo Copying lcms2 headers...
xcopy /y /e "%EXTRAS_DIR%\third_party\Little-CMS\include" "%SRC_DIR%\lcms2\include\" >nul 2>&1

echo Copying libjxl headers...
xcopy /y /e "%EXTRAS_DIR%\third_party\libjxl\lib\include\jxl" "%SRC_DIR%\libjxl\include\jxl\" >nul 2>&1

echo Copying libraw headers...
xcopy /y /e "%EXTRAS_DIR%\third_party\LibRaw\libraw" "%SRC_DIR%\libraw\include\libraw\" >nul 2>&1

echo Copying libheif headers...
xcopy /y /e "%EXTRAS_DIR%\third_party\libheif\libheif\libheif\api\libheif" "%SRC_DIR%\libheif\include\libheif\" >nul 2>&1

echo Copying libavif headers...
xcopy /y /e "%EXTRAS_DIR%\third_party\libavif\include\avif" "%SRC_DIR%\libavif\include\avif\" >nul 2>&1

echo.
echo ============================================================
echo  ALL BUILDS COMPLETED SUCCESSFULLY!
echo ============================================================
echo.
echo Library versions:
echo   - libjpeg-turbo: 3.1.4.1
echo   - libpng-apng: 1.6.58
echo   - libwebp: latest
echo   - lcms2: latest
echo   - libjxl: latest
echo   - libraw: 0.22.1
echo   - libheif: 1.21.2
echo   - libavif: 1.4.1
echo.
echo Output directories:
echo   - 32-bit: %SRC_DIR%\lib\*\
echo   - 64-bit: %SRC_DIR%\lib64\*\
echo.
echo You can now build JPEGView in Visual Studio.
echo.

exit /b 0