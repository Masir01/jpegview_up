@echo off

setlocal
REM this builds libheif and replaces the libs in the JPEGView src folder
REM it also builds libavif as both share the same dav1d libs

SET XSRC_DIR=%~dp0..\..\src\JPEGView\libheif
SET XLIB_DIR=%~dp0..\third_party\libheif
SET XLIBAVIF_DIR=%~dp0..\third_party\libavif
SET XAVIFSRC_DIR=%~dp0..\..\src\JPEGView\libavif
SET XOUT_DIR=%~dp0libheif_libavif

SET XVENV_DIR=%~dp0venv-meson

IF EXIST "%XOUT_DIR%" (
	echo libheif output exists, please delete folder before trying to build
	exit /b 1
)

echo + Looking up NASM
where nasm.exe
IF ERRORLEVEL 1 (
	echo NASM isn't found on path!
	exit /b 1
)

echo + Looking up Meson
where meson.exe
IF ERRORLEVEL 1 (
	echo Meson not found, please run: pip install meson ninja
	exit /b 1
)

echo + Looking up Ninja
where ninja.exe
IF ERRORLEVEL 1 (
	echo Ninja not found, please run: pip install meson ninja
	exit /b 1
)


REM libavif only depends on dav1d
REM libheif depends on dav1d and libde
REM JPEGView only uses x64

call :BUILD_COPY_DAV1D x64 "64"
IF ERRORLEVEL 1 exit /b 1
call :BUILD_COPY_LIBAVIF x64 "64"
IF ERRORLEVEL 1 exit /b 1
call :BUILD_COPY_LIBDE x64 x64 "64"
IF ERRORLEVEL 1 exit /b 1
call :BUILD_COPY_LIBHEIF x64 x64 "64"
IF ERRORLEVEL 1 exit /b 1




echo === HEADER FILES NOT MAINTAINED BY SCRIPT ===
echo NOTE: as for the header files, copy/replace files AS NEEDED
echo TO: src\JPEGView\libheif\include\libheif
echo .h FROM: extras\third_party\libheif\libheif\libheif
echo FROM: %OUT_DIR%\libheif-[arch]\libheif\heif_version.h

echo;
echo TO: src\JPEGView\libheif\include\libavif
echo .h FROM: extras\third_party\libavif\include\avif

exit /b 0




REM ===============================================================================================

:BUILD_COPY_DAV1D

REM so the environments don't pollute each other
setlocal

SET XBUILD_DIR=%XOUT_DIR%\dav1d-%1\build
SET XDIST_DIR=%XOUT_DIR%\dav1d-%1\dist

mkdir "%XBUILD_DIR%" 2>nul
mkdir "%XDIST_DIR%" 2>nul

call "%~dp0vs-init.bat" %1

pushd "%XLIB_DIR%\dav1d"

REM or use --reconfigure flag
REM rd /s /q "%XLIB_DIR%\dav1d\build" 2>nul

REM part of the commands came from dav1d.cmd in libheif third-party

REM dav1d uses meson with --buildtype release for /O2
REM dav1d has its own runtime CPU detection — NO /arch flags
REM Add /GL for LTCG, /Oi for intrinsics, /Ob2 for inline expansion
REM Use space-separated string format for meson c_args on Windows
REM Clean MSYS paths from PATH (pkg-config from msys64 confuses meson)
REM Also remove sccache from PATH (sandbox blocks its cache dir)
set "PATH=%PATH:D:\tools\wbin\msys64;=%"
set "PATH=%PATH:C:\Users\Trace\.Cargo\bin;=%"
REM also filter INCLUDE/LIB for MSVC safety
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('INCLUDE','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "INCLUDE=%%p"
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('LIB','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "LIB=%%p"
SET XARCH_AVX2=-Dc_args="/GL /Oi /Ob2"

REM meson.exe from pip install venv\scripts
REM if you want to build static DLL do this:  meson setup --default-library=static --buildtype release --prefix "%XDIST_DIR%" "%XBUILD_DIR%"
meson setup --buildtype release --prefix "%XDIST_DIR%" %XARCH_AVX2% "%XBUILD_DIR%"
IF ERRORLEVEL 1 exit /b 1

REM ninja.exe from pip install venv\scripts
ninja -C "%XBUILD_DIR%"
IF ERRORLEVEL 1 exit /b 1
REM libheif building requires things to be "installed" to work
ninja -C "%XBUILD_DIR%" install
IF ERRORLEVEL 1 exit /b 1

REM when dynamic, will copy this out
REM copy /y "%XBUILD_DIR%\src\dav1d.dll" "%XSRC_DIR%\bin%~2\"
copy /y "%XDIST_DIR%\bin\dav1d.dll" "%XAVIFSRC_DIR%\bin%~2\"
IF ERRORLEVEL 1 exit /b 1
::copy /y "%XDIST_DIR%\lib\dav1d.lib" "%XAVIFSRC_DIR%\lib%~2\"
::IF ERRORLEVEL 1 exit /b 1

popd

exit /b 0




REM ===============================================================================================
:BUILD_COPY_LIBDE

REM so the environments don't pollute each other
setlocal

SET XLIB_DIR=%XLIB_DIR%\libde265

SET XBUILD_DIR=%XOUT_DIR%\libde-%1

mkdir "%XBUILD_DIR%" 2>nul

call "%~dp0vs-init.bat" %1

pushd "%XBUILD_DIR%"
REM libde265 has runtime SIMD detection — NO /arch flags
REM NMake Makefiles generator avoids VS version dependency
REM Add /GL for LTCG, /Oi for intrinsics
REM Remove all MSYS/MinGW/msvcrt paths from PATH, INCLUDE, LIB
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('PATH','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git|sccache|cargo'}) -join ';'}"') do set "PATH=%%p"
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('INCLUDE','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "INCLUDE=%%p"
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('LIB','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "LIB=%%p"
cmake.exe -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="/GL /Oi" "%XLIB_DIR%"
IF ERRORLEVEL 1 exit /b 1
nmake.exe
IF ERRORLEVEL 1 exit /b 1


REM NMake Makefiles single-config gen — no Release\ subdir
copy /y "%XBUILD_DIR%\libde265\libde265.dll" "%XSRC_DIR%\bin%~3\"
IF ERRORLEVEL 1 exit /b 1


popd

exit /b 0








REM ===============================================================================================

:BUILD_COPY_LIBHEIF

REM so the environments don't pollute each other
setlocal

SET XARCH=%1

SET XBUILD_DIR=%XOUT_DIR%\libheif-%XARCH%

SET XDAV1D_DIST=%XOUT_DIR%\dav1d-%XARCH%\dist
SET XDE265_DIR=%XLIB_DIR%\libde265

SET XDE265_BUILD=%XOUT_DIR%\libde-%1

mkdir "%XBUILD_DIR%" 2>nul

call "%~dp0vs-init.bat" %XARCH%

copy /y "%XDE265_BUILD%\libde265\de265-version.h" "%XDE265_DIR%\libde265"
IF ERRORLEVEL 1 exit /b 1

pushd "%XBUILD_DIR%"

REM Remove all MSYS/MinGW/msvcrt paths from PATH, INCLUDE, LIB
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('PATH','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git|sccache|cargo'}) -join ';'}"') do set "PATH=%%p"
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('INCLUDE','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "INCLUDE=%%p"
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('LIB','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "LIB=%%p"

REM Use NMake Makefiles to avoid VS version dependency
REM Disable examples/tests — only the library is needed
cmake.exe -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="/O2 /Ob2 /GL" ^
    -DCMAKE_CXX_FLAGS="/O2 /Ob2 /GL /EHsc" ^
    -DWITH_EXAMPLES=OFF ^
    -DBUILD_TESTING=OFF ^
    -DDAV1D_LIBRARY="%XDAV1D_DIST%\lib\dav1d.lib" ^
    -DDAV1D_INCLUDE_DIR="%XDAV1D_DIST%\include" ^
    -DLIBDE265_LIBRARY="%XDE265_BUILD%\libde265\de265.lib" ^
    -DLIBDE265_INCLUDE_DIR="%XDE265_DIR%" ^
    "%XLIB_DIR%\libheif"
IF ERRORLEVEL 1 exit /b 1
nmake.exe
IF ERRORLEVEL 1 exit /b 1

popd


REM NMake Makefiles single-config gen — no Release\ subdir
copy /y "%XBUILD_DIR%\libheif\heif.dll" "%XSRC_DIR%\bin%~3\"
IF ERRORLEVEL 1 exit /b 1
copy /y "%XBUILD_DIR%\libheif\heif.lib" "%XSRC_DIR%\lib%~3\"
IF ERRORLEVEL 1 exit /b 1


exit /b 0






REM ===============================================================================================

:BUILD_COPY_LIBAVIF


REM so the environments don't pollute each other
setlocal

SET XARCH=%1

SET XBUILD_DIR=%XOUT_DIR%\libavif-%XARCH%

SET XDAV1D_DIST=%XOUT_DIR%\dav1d-%XARCH%\dist


mkdir "%XBUILD_DIR%" 2>nul

call "%~dp0vs-init.bat" %XARCH%


pushd "%XBUILD_DIR%"

REM libavif doesn't add /arch flags, relies on dav1d's runtime dispatch
REM Add /GL for LTCG
REM Remove all MSYS/MinGW paths from PATH, INCLUDE, LIB
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('PATH','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git|sccache|cargo'}) -join ';'}"') do set "PATH=%%p"
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('INCLUDE','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "INCLUDE=%%p"
for /f "tokens=*" %%p in ('powershell -NoProfile -Command "$i=[Environment]::GetEnvironmentVariable('LIB','Process'); if($i){($i -split ';' | ?{$_ -notmatch 'msys|mingw|ucrt64|git'}) -join ';'}"') do set "LIB=%%p"
cmake.exe -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="/GL" ^
    -DCMAKE_CXX_FLAGS="/GL" ^
    -DAVIF_CODEC_DAV1D=ON ^
    -DAVIF_LIBYUV=OFF ^
    -DDAV1D_LIBRARY="%XDAV1D_DIST%\lib\dav1d.lib" ^
    -DDAV1D_INCLUDE_DIR="%XDAV1D_DIST%\include" ^
    "%XLIBAVIF_DIR%"
IF ERRORLEVEL 1 exit /b 1
ninja.exe
IF ERRORLEVEL 1 exit /b 1

popd


copy /y "%XBUILD_DIR%\avif.dll" "%XAVIFSRC_DIR%\bin%~2\"
IF ERRORLEVEL 1 exit /b 1
copy /y "%XBUILD_DIR%\avif.lib" "%XAVIFSRC_DIR%\lib%~2\"




exit /b 0
