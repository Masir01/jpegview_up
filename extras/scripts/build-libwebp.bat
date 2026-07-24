@echo off

setlocal
REM this builds libwebp and replaces the libs in the JPEGView src folder

SET XSRC_DIR=%~dp0..\..\src\JPEGView\libwebp
SET XLIB_DIR=%~dp0..\third_party\libwebp
SET XOUT_DIR=%~dp0libwebp

IF EXIST "%XOUT_DIR%" (
	echo libwebp output exists, please delete folder before trying to build
	exit /b 1
)

REM JPEGView only uses x64
call :BUILD_COPY_WEBP x64 lib64
IF ERRORLEVEL 1 exit /b 1


echo === HEADER FILES NOT MAINTAINED BY SCRIPT ===
echo NOTE: as for the header files, copy/replace files AS NEEDED
echo extras\third_party\libwebp\src\webp -to- src\JPEGView\libwebp\include

exit /b 0




:BUILD_COPY_WEBP

REM so the environments don't pollute each other
setlocal

mkdir "%XOUT_DIR%" 2>nul

call "%~dp0vs-init.bat" %1

pushd "%XLIB_DIR%"

REM libwebp Makefile.vc handles /O2 internally for release-static
REM /MT via RTLIBCFG=static, SIMD via internal runtime dispatch
REM Inject /GL via CL env var for LTCG (Makefile.vc doesn't add it)
REM Do NOT add CFLAGS with /arch — NMake misparses them (U1065 error)
SET CL=/GL
nmake.exe /f Makefile.vc ARCH=%1 CFG=release-static RTLIBCFG=static OBJDIR="%XOUT_DIR%"
IF ERRORLEVEL 1 exit /b 1
SET CL=

popd

REM copy the libs over
copy /y "%XOUT_DIR%\release-static\%1\lib\libwebp.lib" "%XSRC_DIR%\%2\"
IF ERRORLEVEL 1 exit /b 1
copy /y "%XOUT_DIR%\release-static\%1\lib\libwebpdemux.lib" "%XSRC_DIR%\%2\"
IF ERRORLEVEL 1 exit /b 1


exit /b 0
