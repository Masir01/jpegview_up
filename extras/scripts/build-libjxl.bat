@echo off

setlocal enabledelayedexpansion

REM this builds libjxl and replaces the libs in the JPEGView src folder

REM for me it takes like 15min to build
echo NOTE: this takes a LONG time to build, don't be alarmed...
echo       if it looks like the build process hung... it didn't, it's just WAY SLOW!

SET XSRC_DIR=%~dp0..\..\src
SET XLIB_DIR=%~dp0..\third_party\libjxl
SET XOUT_DIR=%~dp0libjxl

IF EXIST "%XOUT_DIR%" (
    echo libjxl output exists, please delete folder before trying to build
    exit /b 1
)

REM JPEGView only uses x64
call :BUILD_COPY_JXL x64 x64 "64"
IF ERRORLEVEL 1 exit /b 1

echo === HEADER FILES NOT MAINTAINED BY SCRIPT ===
echo NOTE: as for the header files, copy/replace files AS NEEDED
echo TO: src\JPEGView\libjxl\include\jxl
echo FROM: extras\third_party\libjxl\lib\include\jxl
echo FROM: %XOUT_DIR%\[arch]\lib\include\jxl

exit /b 0

:BUILD_COPY_JXL

SET XBUILD_ARCH=%~1
SET XPLATFORM=%~2
SET XJPV_ARCH_PATH=%~3

SET XBUILD_DIR=%XOUT_DIR%\%XBUILD_ARCH%

mkdir "%XBUILD_DIR%" 2>nul

call "%~dp0vs-init.bat" %XBUILD_ARCH%

REM Clean MSYS paths from INCLUDE and LIB using PowerShell
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('INCLUDE','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "INCLUDE=%%p"
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('LIB','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "LIB=%%p"

pushd "%XBUILD_DIR%"

REM libjxl uses HWY for full runtime SIMD dispatch — NO /arch flags (would break dispatch)
REM cmake Release mode sets /O2; HWY handles SIMD math internally, no /fp needed
REM JPEGXL_ENABLE_LTO enables LTCG (whole-program optimization + linker code gen)
REM Disable everything JPEGView doesn't need for lean build
cmake.exe -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DJPEGXL_ENABLE_LTO=ON ^
    -DBUILD_TESTING=OFF ^
    -DJPEGXL_ENABLE_BENCHMARK=OFF ^
    -DJPEGXL_ENABLE_DOXYGEN=OFF ^
    -DJPEGXL_ENABLE_JNI=OFF ^
    -DJPEGXL_ENABLE_MANPAGES=OFF ^
    -DJPEGXL_ENABLE_OPENEXR=OFF ^
    -DJPEGXL_ENABLE_PLUGINS=OFF ^
    -DJPEGXL_ENABLE_SJPEG=OFF ^
    -DJPEGXL_ENABLE_TCMALLOC=OFF ^
    -DJPEGXL_FORCE_SYSTEM_BROTLI=OFF ^
    -DJPEGXL_ENABLE_BROTLI=ON ^
    "%XLIB_DIR%"
IF ERRORLEVEL 1 exit /b 1
ninja.exe -C "%XBUILD_DIR%"
IF ERRORLEVEL 1 exit /b 1

popd

REM create destination directories if they don't exist
if not exist "%XSRC_DIR%\JPEGView\libjxl\bin%XJPV_ARCH_PATH%" mkdir "%XSRC_DIR%\JPEGView\libjxl\bin%XJPV_ARCH_PATH%"
if not exist "%XSRC_DIR%\JPEGView\libjxl\lib%XJPV_ARCH_PATH%" mkdir "%XSRC_DIR%\JPEGView\libjxl\lib%XJPV_ARCH_PATH%"

REM copy the libs over
copy /y "%XBUILD_DIR%\lib\jxl.dll" "%XSRC_DIR%\JPEGView\libjxl\bin%XJPV_ARCH_PATH%\"
copy /y "%XBUILD_DIR%\lib\jxl_threads.dll" "%XSRC_DIR%\JPEGView\libjxl\bin%XJPV_ARCH_PATH%\"
copy /y "%XBUILD_DIR%\lib\jxl_cms.dll" "%XSRC_DIR%\JPEGView\libjxl\bin%XJPV_ARCH_PATH%\"
copy /y "%XBUILD_DIR%\lib\jxl_dec.dll" "%XSRC_DIR%\JPEGView\libjxl\bin%XJPV_ARCH_PATH%\"
REM brotli DLLs are built in third_party\brotli\ (not in enc\ subfolder)
REM all three brotli DLLs (brotlicommon, brotlidec, brotlienc) are needed at runtime
copy /y "%XBUILD_DIR%\third_party\brotli\brotlicommon.dll" "%XSRC_DIR%\JPEGView\libjxl\bin%XJPV_ARCH_PATH%\"
copy /y "%XBUILD_DIR%\third_party\brotli\brotlidec.dll" "%XSRC_DIR%\JPEGView\libjxl\bin%XJPV_ARCH_PATH%\"
copy /y "%XBUILD_DIR%\third_party\brotli\brotlienc.dll" "%XSRC_DIR%\JPEGView\libjxl\bin%XJPV_ARCH_PATH%\"
copy /y "%XBUILD_DIR%\lib\jxl.lib" "%XSRC_DIR%\JPEGView\libjxl\lib%XJPV_ARCH_PATH%\"
copy /y "%XBUILD_DIR%\lib\jxl_dec.lib" "%XSRC_DIR%\JPEGView\libjxl\lib%XJPV_ARCH_PATH%\"
copy /y "%XBUILD_DIR%\lib\jxl_threads.lib" "%XSRC_DIR%\JPEGView\libjxl\lib%XJPV_ARCH_PATH%\"
copy /y "%XBUILD_DIR%\lib\jxl_cms.lib" "%XSRC_DIR%\JPEGView\libjxl\lib%XJPV_ARCH_PATH%\"

exit /b 0
