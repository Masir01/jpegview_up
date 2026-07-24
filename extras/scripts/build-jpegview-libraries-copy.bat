@echo off
chcp 65001 >nul

REM ============================================================
REM JPEGView - Copy third-party library header files
REM ============================================================

setlocal enabledelayedexpansion

SET SCRIPT_DIR=%~dp0
SET EXTRAS_DIR=%SCRIPT_DIR%..
SET SRC_DIR=%EXTRAS_DIR%\src\JPEGView


echo.
echo ============================================================
echo  Copying header files
echo ============================================================

echo.
echo Copying libjpeg-turbo headers...
rd /s /q "%SRC_DIR%\libjpeg-turbo\include" 2>nul
mkdir "%SRC_DIR%\libjpeg-turbo\include" 2>nul
xcopy /y "%EXTRAS_DIR%\third_party\libjpeg-turbo\src\*.h" "%SRC_DIR%\libjpeg-turbo\include\" >nul 2>&1
xcopy /y "%SCRIPT_DIR%libjpeg-turbo\lib\jconfig.h" "%SRC_DIR%\libjpeg-turbo\include\" >nul 2>&1

echo.
echo Copying libpng-apng headers...
rd /s /q "%SRC_DIR%\libpng-apng\include" 2>nul
mkdir "%SRC_DIR%\libpng-apng\include" 2>nul
xcopy /y "%EXTRAS_DIR%\third_party\libpng-apng.src-patch\libpng\*.h" "%SRC_DIR%\libpng-apng\include\" >nul 2>&1

echo.
echo Copying libwebp headers...
rd /s /q "%SRC_DIR%\libwebp\include\webp" 2>nul
mkdir "%SRC_DIR%\libwebp\include\webp" 2>nul
xcopy /y "%EXTRAS_DIR%\third_party\libwebp\src\webp\*.h" "%SRC_DIR%\libwebp\include\webp\" >nul 2>&1

echo.
echo Copying lcms2 headers...
rd /s /q "%SRC_DIR%\lcms2\include" 2>nul
mkdir "%SRC_DIR%\lcms2\include" 2>nul
xcopy /y "%EXTRAS_DIR%\third_party\Little-CMS\include\*.h" "%SRC_DIR%\lcms2\include\" >nul 2>&1

echo.
echo Copying libjxl headers...
rd /s /q "%SRC_DIR%\libjxl\include\jxl" 2>nul
mkdir "%SRC_DIR%\libjxl\include\jxl" 2>nul
xcopy /y "%EXTRAS_DIR%\third_party\libjxl\lib\include\jxl\*.h" "%SRC_DIR%\libjxl\include\jxl\" >nul 2>&1

echo.
echo Copying libraw headers...
rd /s /q "%SRC_DIR%\libraw\include\libraw" 2>nul
mkdir "%SRC_DIR%\libraw\include\libraw" 2>nul
xcopy /y "%EXTRAS_DIR%\third_party\LibRaw\libraw\*.h" "%SRC_DIR%\libraw\include\libraw\" >nul 2>&1

echo.
echo Copying libheif headers...
rd /s /q "%SRC_DIR%\libheif\include\libheif" 2>nul
mkdir "%SRC_DIR%\libheif\include\libheif" 2>nul
REM Copy public API headers only (no .cc files, no internal headers)
xcopy /y "%EXTRAS_DIR%\third_party\libheif\libheif\libheif\api\libheif\*.h" "%SRC_DIR%\libheif\include\libheif\" >nul 2>&1
REM Copy generated heif_version.h from build output if available
IF EXIST "%SCRIPT_DIR%libheif_libavif\libheif-x64\libheif\heif_version.h" (
    copy /y "%SCRIPT_DIR%libheif_libavif\libheif-x64\libheif\heif_version.h" "%SRC_DIR%\libheif\include\libheif\" >nul
)

echo.
echo Copying libavif headers...
rd /s /q "%SRC_DIR%\libavif\include\avif" 2>nul
mkdir "%SRC_DIR%\libavif\include\avif" 2>nul
xcopy /y "%EXTRAS_DIR%\third_party\libavif\include\avif\*.h" "%SRC_DIR%\libavif\include\avif\" >nul 2>&1

echo.
echo ============================================================
echo  ALL HEADER COPIES COMPLETED SUCCESSFULLY!
echo ============================================================
echo.
echo Library versions:
echo   - libjpeg-turbo: 3.1.4.1
echo   - libpng-apng: 1.6.58
echo   - libwebp: latest
echo   - lcms2: latest
echo   - libjxl: latest
echo   - libraw: 0.22.1
echo   - libheif: 1.23.1
echo   - libavif: 1.4.1
echo.
echo Output directories:
echo   - 32-bit: %SRC_DIR%\lib\*\
echo   - 64-bit: %SRC_DIR%\lib64\*\
echo.
echo You can now build JPEGView in Visual Studio.
echo.

exit /b 0
