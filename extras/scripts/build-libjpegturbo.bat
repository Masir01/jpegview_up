@echo off

REM libjpeg-turbo build script for JPEGView
REM Builds libjpeg-turbo 3.x with full optimizations using NMake Makefiles

setlocal

SET XSRC_DIR=%~dp0..\..\src
SET XLIB_DIR=%~dp0..\third_party\libjpeg-turbo
SET XOUT_DIR=%~dp0libjpeg-turbo

IF EXIST "%XOUT_DIR%" (
	echo libjpeg-turbo output exists, please delete folder before trying to build
	exit /b 1
)

echo.
echo === Building libjpeg-turbo 3.x with full optimizations ===
echo.

call :BUILD_COPY_JPEGT x64 lib64
IF ERRORLEVEL 1 exit /b 1

echo.
echo === HEADER FILES NOT MAINTAINED BY SCRIPT ===
echo NOTE: as for the header files, copy/replace files AS NEEDED
echo TO: src\JPEGView\libjpeg-turbo\include
echo FROM: extras\third_party\libjpeg-turbo\src
echo jconfig.h is in output directory


exit /b 0




:BUILD_COPY_JPEGT

setlocal

SET XBUILD_DIR=%XOUT_DIR%\%1

mkdir "%XBUILD_DIR%" 2>nul

call "%~dp0vs-init.bat" %1

pushd "%XBUILD_DIR%"

REM Configure CMake with NMake Makefiles (no VS version dependency)
REM Full optimizations:
REM   WITH_JPEG8=1     : Enable 12-bit/16-bit JPEG support
REM   REQUIRE_SIMD=1  : Force SIMD, error if NASM not found
REM   WITH_SIMD=1      : Enable SIMD instructions
REM   WITH_TURBOJPEG=1 : Build TurboJPEG API library
REM   ENABLE_SHARED=0  : Skip DLL build, static only
REM   ENABLE_STATIC=1  : Build static libraries
REM   WITH_TESTS=0     : No tests, lean build
REM   WITH_TOOLS=0     : No tools, lean build
REM   WITH_CRT_DLL=0   : Static CRT (/MT), avoid CRT conflicts
REM   /O2 /Ob2         : Maximum optimization + inline expansion
REM   /Oi              : Enable intrinsic functions (SIMD)
REM   /GL              : Whole program optimization (LTCG)
REM   /fp:fast         : Fast floating-point operations
REM   /MT              : Static CRT (redundant but explicit)
set "ASM_NASM=D:\ide\sdk\nasm\nasm.exe"
cmake.exe -G"NMake Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DWITH_JPEG8=1 ^
    -DREQUIRE_SIMD=1 ^
    -DWITH_SIMD=1 ^
    -DWITH_TURBOJPEG=1 ^
    -DENABLE_SHARED=0 ^
    -DENABLE_STATIC=1 ^
    -DWITH_TESTS=0 ^
    -DWITH_TOOLS=0 ^
    -DWITH_CRT_DLL=0 ^
    -DCMAKE_C_FLAGS="/DWIN32 /D_WINDOWS /O2 /Ob2 /Oi /GL /fp:fast /MT" ^
    "%XLIB_DIR%"
IF ERRORLEVEL 1 exit /b 1

nmake.exe
IF ERRORLEVEL 1 exit /b 1

popd

REM Copy TurboJPEG static lib (used by JPEGView)
copy /y "%XBUILD_DIR%\turbojpeg-static.lib" "%XSRC_DIR%\JPEGView\libjpeg-turbo\%~2\"
IF ERRORLEVEL 1 exit /b 1

REM Copy libjpeg static lib (for direct libjpeg API usage)
copy /y "%XBUILD_DIR%\jpeg-static.lib" "%XSRC_DIR%\JPEGView\libjpeg-turbo\%~2\"
IF ERRORLEVEL 1 exit /b 1

REM copy jconfig.h
copy /y "%XBUILD_DIR%\jconfig.h" "%XSRC_DIR%\JPEGView\libjpeg-turbo\include\"
IF ERRORLEVEL 1 exit /b 1

exit /b 0
