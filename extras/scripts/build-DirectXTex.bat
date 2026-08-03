@echo off
setlocal

REM ============================================================
REM DirectXTex build script for JPEGView (DDS texture library)
REM Builds a static library (CPU-only DDS decode) for x64
REM ============================================================

SET SCRIPT_DIR=%~dp0
SET XSRC_DIR=%SCRIPT_DIR%..\..\src\JPEGView
SET XLIB_DIR=%SCRIPT_DIR%..\third_party\DirectXTex
SET XOUT_DIR=%SCRIPT_DIR%DirectXTex

IF EXIST "%XOUT_DIR%" (
    echo DirectXTex output dir exists, please delete it before building:
    echo   %XOUT_DIR%
    exit /b 1
)

echo.
echo === Building DirectXTex (static, CPU-only DDS decode) ===
echo.

call :BUILD_COPY_DXTX x64 lib64
IF ERRORLEVEL 1 exit /b 1

echo.
echo === DirectXTex build complete ===
echo Headers : %XSRC_DIR%\DirectXTex\include
echo Lib x64 : %XSRC_DIR%\DirectXTex\lib64\DirectXTex.lib
echo.
echo Integrate in JPEGView.vcxproj (x64):
echo   IncludePath          += $(ProjectDir)DirectXTex\include
echo   LibraryPath          += $(ProjectDir)DirectXTex\lib64
echo   AdditionalDependencies += DirectXTex.lib
echo.

exit /b 0


:BUILD_COPY_DXTX
setlocal
SET XARCH=%~1
SET XLIBSUB=%~2
SET XBUILD_DIR=%XOUT_DIR%\%XARCH%

mkdir "%XBUILD_DIR%" 2>nul

call "%SCRIPT_DIR%vs-init.bat" %XARCH%
IF ERRORLEVEL 1 exit /b 1

pushd "%XBUILD_DIR%"

REM Configure CMake with NMake Makefiles (no VS version dependency)
REM Options chosen for JPEGView:
REM   BUILD_SHARED_LIBS=OFF   static lib, linked into JPEGView
REM   BUILD_TOOLS=OFF         skip texconv/texdiag/texassemble
REM   BUILD_SAMPLE=OFF        skip DDSView
REM   BUILD_DX11=OFF          no GPU BC compression / fxc.exe needed
REM   BUILD_DX12=OFF          no D3D12 runtime dependency
REM   BC_USE_OPENMP=OFF       no OpenMP dependency (decode only)
REM   CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded  match JPEGView /MT
cmake.exe -G"NMake Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DBUILD_TOOLS=OFF ^
    -DBUILD_SAMPLE=OFF ^
    -DBUILD_DX11=OFF ^
    -DBUILD_DX12=OFF ^
    -DBC_USE_OPENMP=OFF ^
    "%XLIB_DIR%"
IF ERRORLEVEL 1 exit /b 1

nmake.exe
IF ERRORLEVEL 1 exit /b 1

popd

REM Ensure destination dirs exist
if not exist "%XSRC_DIR%\DirectXTex\include" mkdir "%XSRC_DIR%\DirectXTex\include"
if not exist "%XSRC_DIR%\DirectXTex\%XLIBSUB%" mkdir "%XSRC_DIR%\DirectXTex\%XLIBSUB%"

REM Copy static lib
copy /y "%XBUILD_DIR%\lib\DirectXTex.lib" "%XSRC_DIR%\DirectXTex\%XLIBSUB%\"
IF ERRORLEVEL 1 exit /b 1

REM Copy public headers (DirectXTex.h + DirectXTex.inl)
copy /y "%XLIB_DIR%\DirectXTex\DirectXTex.h" "%XSRC_DIR%\DirectXTex\include\"
IF ERRORLEVEL 1 exit /b 1
copy /y "%XLIB_DIR%\DirectXTex\DirectXTex.inl" "%XSRC_DIR%\DirectXTex\include\"
IF ERRORLEVEL 1 exit /b 1

exit /b 0
