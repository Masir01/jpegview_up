
REM Builds exiv2 (static, /MT) and copies outputs into src\JPEGView\exiv2
REM Requires: expat source at extras\third_party\expat (downloaded by user)
REM Reuses:   zlib source at extras\third_party\libpng-apng.src-patch\zlib
REM           zlib.lib  at src\JPEGView\libpng-apng\lib64\zlib.lib

setlocal enabledelayedexpansion

SET XSRC_DIR=%~dp0..\..\src\JPEGView
SET XEXIV2_DIR=%~dp0..\third_party\exiv2
SET XEXPAT_DIR=%~dp0..\third_party\libexpat\expat
SET XBROTLI_DIR=%~dp0..\third_party\libjxl\third_party\brotli
SET XZLIB_DIR=%~dp0..\third_party\libpng-apng.src-patch\zlib
SET XZLIB_LIB=%XSRC_DIR%\libpng-apng\lib64\zlib.lib
SET XOUT_DIR=%~dp0exiv2

REM ---- preflight: required inputs ----
IF NOT EXIST "%XEXIV2_DIR%\CMakeLists.txt" (
    echo ERROR: exiv2 source not found at extras\third_party\exiv2
    exit /b 1
)
IF NOT EXIST "%XEXPAT_DIR%\CMakeLists.txt" (
    echo ERROR: expat source not found at extras\third_party\libexpat\expat
    echo Please download expat-2.6.x source from:
    echo   https://github.com/libexpat/libexpat/releases
    echo and extract it to extras\third_party\libexpat\expat
    exit /b 1
)
IF NOT EXIST "%XBROTLI_DIR%\CMakeLists.txt" (
    echo ERROR: brotli source not found at extras\third_party\libjxl\third_party\brotli
    exit /b 1
)
IF NOT EXIST "%XZLIB_DIR%\zlib.h" (
    echo ERROR: zlib.h not found at extras\third_party\libpng-apng.src-patch\zlib
    exit /b 1
)
IF NOT EXIST "%XZLIB_LIB%" (
    echo ERROR: zlib.lib not found at src\JPEGView\libpng-apng\lib64
    echo Build libpng-apng first - build-libpng-apng.bat - to produce it.
    exit /b 1
)

REM ---- toolchain ----
call "%~dp0vs-init.bat" x64
IF ERRORLEVEL 1 exit /b 1

SET XGEN=NMake
where ninja.exe >nul 2>&1
IF NOT ERRORLEVEL 1 SET XGEN=Ninja

REM ---- 1) build brotli (static /MT, from libjxl third_party) ----
SET XBROTLI_BUILD=%XOUT_DIR%\brotli
IF EXIST "%XBROTLI_BUILD%" (
    echo ERROR: %XBROTLI_BUILD% exists, delete it before rebuilding
    exit /b 1
)
mkdir "%XBROTLI_BUILD%"
pushd "%XBROTLI_BUILD%"
cmake.exe -G "%XGEN%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
    -DBROTLI_BUILD_TOOLS=OFF ^
    -DBROTLI_DISABLE_TESTS=ON ^
    "%XBROTLI_DIR%"
IF ERRORLEVEL 1 exit /b 1
IF /I "%XGEN%" EQU "Ninja" (
    ninja.exe -C "%XBROTLI_BUILD%"
) ELSE (
    nmake.exe
)
IF ERRORLEVEL 1 exit /b 1
popd

SET XBROTLI_INC=%XBROTLI_DIR%\c\include
SET XBROTLI_COMMON_LIB=%XBROTLI_BUILD%\brotlicommon.lib
SET XBROTLI_DEC_LIB=%XBROTLI_BUILD%\brotlidec.lib
IF NOT EXIST "%XBROTLI_DEC_LIB%" (
    echo ERROR: brotlidec.lib not produced at %XBROTLI_BUILD%
    exit /b 1
)

REM ---- 2) build expat (static /MT) ----
SET XEXPAT_BUILD=%XOUT_DIR%\expat
IF EXIST "%XEXPAT_BUILD%" (
    echo ERROR: %XEXPAT_BUILD% exists, delete it before rebuilding
    exit /b 1
)
mkdir "%XEXPAT_BUILD%"
pushd "%XEXPAT_BUILD%"
cmake.exe -G "%XGEN%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
    -DEXPAT_MSVC_STATIC_CRT=ON ^
    -DEXPAT_BUILD_TOOLS=OFF ^
    -DEXPAT_BUILD_EXAMPLES=OFF ^
    -DEXPAT_BUILD_TESTS=OFF ^
    -DEXPAT_BUILD_DOCS=OFF ^
    -DEXPAT_BUILD_PKGCONFIG=OFF ^
    "%XEXPAT_DIR%"
IF ERRORLEVEL 1 exit /b 1
IF /I "%XGEN%" EQU "Ninja" (
    ninja.exe -C "%XEXPAT_BUILD%"
) ELSE (
    nmake.exe
)
IF ERRORLEVEL 1 exit /b 1
popd

REM locate expat headers (lib\expat.h in modern expat sources)
IF EXIST "%XEXPAT_DIR%\lib\expat.h" (
    SET XEXPAT_INC=%XEXPAT_DIR%\lib
) ELSE (
    SET XEXPAT_INC=%XEXPAT_DIR%
)
SET XEXPAT_LIB=%XEXPAT_BUILD%\libexpatMT.lib
IF NOT EXIST "%XEXPAT_LIB%" (
    echo ERROR: libexpatMT.lib not produced at %XEXPAT_BUILD%
    exit /b 1
)

REM ---- 3) build exiv2 (static /MT, reuse project zlib + expat + brotli) ----
SET XEXIV2_BUILD=%XOUT_DIR%\exiv2
IF EXIST "%XEXIV2_BUILD%" (
    echo ERROR: %XEXIV2_BUILD% exists, delete it before rebuilding
    exit /b 1
)
mkdir "%XEXIV2_BUILD%"
pushd "%XEXIV2_BUILD%"
cmake.exe -G "%XGEN%" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DEXIV2_ENABLE_DYNAMIC_RUNTIME=OFF ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded ^
    -DEXIV2_ENABLE_XMP=ON ^
    -DEXIV2_ENABLE_EXTERNAL_XMP=OFF ^
    -DEXIV2_ENABLE_PNG=ON ^
    -DEXIV2_ENABLE_NLS=OFF ^
    -DEXIV2_ENABLE_LENSDATA=ON ^
    -DEXIV2_ENABLE_BMFF=ON ^
    -DEXIV2_ENABLE_BROTLI=ON ^
    -DEXIV2_ENABLE_VIDEO=OFF ^
    -DEXIV2_ENABLE_WEBREADY=OFF ^
    -DEXIV2_ENABLE_CURL=OFF ^
    -DEXIV2_ENABLE_INIH=OFF ^
    -DEXIV2_BUILD_SAMPLES=OFF ^
    -DEXIV2_BUILD_EXIV2_COMMAND=OFF ^
    -DEXIV2_BUILD_UNIT_TESTS=OFF ^
    -DBUILD_TESTING=OFF ^
    -DZLIB_INCLUDE_DIR="%XZLIB_DIR%" ^
    -DZLIB_LIBRARY="%XZLIB_LIB%" ^
    -DEXPAT_INCLUDE_DIR="%XEXPAT_INC%" ^
    -DEXPAT_LIBRARY="%XEXPAT_LIB%" ^
    -DBROTLI_INCLUDE_DIR="%XBROTLI_INC%" ^
    -DBROTLIDEC_LIBRARY="%XBROTLI_DEC_LIB%" ^
    -DBROTLICOMMON_LIBRARY="%XBROTLI_COMMON_LIB%" ^
    "%XEXIV2_DIR%"
IF ERRORLEVEL 1 exit /b 1
IF /I "%XGEN%" EQU "Ninja" (
    ninja.exe -C "%XEXIV2_BUILD%"
) ELSE (
    nmake.exe
)
IF ERRORLEVEL 1 exit /b 1
popd

REM ---- 4) copy outputs ----
IF NOT EXIST "%XSRC_DIR%\exiv2\lib64" mkdir "%XSRC_DIR%\exiv2\lib64"
IF NOT EXIST "%XSRC_DIR%\exiv2\include\exiv2" mkdir "%XSRC_DIR%\exiv2\include\exiv2"
IF NOT EXIST "%XSRC_DIR%\exiv2\include\brotli" mkdir "%XSRC_DIR%\exiv2\include\brotli"

copy /y "%XEXIV2_BUILD%\lib\exiv2.lib" "%XSRC_DIR%\exiv2\lib64\" || exit /b 1
copy /y "%XBROTLI_COMMON_LIB%" "%XSRC_DIR%\exiv2\lib64\" || exit /b 1
copy /y "%XBROTLI_DEC_LIB%" "%XSRC_DIR%\exiv2\lib64\" || exit /b 1
copy /y "%XBROTLI_BUILD%\brotlienc.lib" "%XSRC_DIR%\exiv2\lib64\" || exit /b 1
xcopy /s /e /y "%XEXIV2_DIR%\include\exiv2\*.hpp" "%XSRC_DIR%\exiv2\include\exiv2\" >nul || exit /b 1
xcopy /s /e /y "%XEXIV2_DIR%\include\exiv2\*.h" "%XSRC_DIR%\exiv2\include\exiv2\" >nul 2>nul
copy /y "%XEXIV2_BUILD%\exv_conf.h" "%XSRC_DIR%\exiv2\include\exiv2\" || exit /b 1
IF EXIST "%XEXIV2_BUILD%\exiv2lib_export.h" copy /y "%XEXIV2_BUILD%\exiv2lib_export.h" "%XSRC_DIR%\exiv2\include\exiv2\" || exit /b 1
copy /y "%XEXPAT_LIB%" "%XSRC_DIR%\exiv2\lib64\" || exit /b 1
xcopy /s /e /y "%XBROTLI_INC%\brotli\*.h" "%XSRC_DIR%\exiv2\include\brotli\" >nul || exit /b 1

echo === BUILD OK ===
echo lib  : %XSRC_DIR%\exiv2\lib64\exiv2.lib
echo hdrs : %XSRC_DIR%\exiv2\include\exiv2

exit /b 0
