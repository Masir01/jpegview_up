@echo off
chcp 65001 >nul

setlocal
REM this builds LibRaw and replaces the dlls/libs in the JPEGView src folder

SET XSRC_DIR=%~dp0..\..\src\JPEGView\libraw
SET XLIB_DIR=%~dp0..\third_party\LibRaw

REM random name of a makefile that we're going to create / patch with options
SET XMAKEFILE=Makefile.msvc-%RANDOM%
SET XMF_PATH=%XLIB_DIR%\%XMAKEFILE%

IF EXIST "%XMF_PATH%" (
    echo ERROR: Random Makefile exists?  Shouldn't happen... reset submodule please!
    exit /b 1
)

call :PATCH_LIBRAW_MAKEFILE
IF ERRORLEVEL 1 exit /b 1

REM JPEGView only uses x64
call :BUILD_COPY_LIBRAW x64 "64"
IF ERRORLEVEL 1 exit /b 1

REM cleanup
del "%XMAKEFILE%" 2>nul

echo === HEADER FILES NOT MAINTAINED BY SCRIPT ===
echo NOTE: as for the header files, copy/replace files AS NEEDED
echo.
echo TO:   src\JPEGView\libraw\include
echo FROM: extras\third_party\LibRaw\libraw

exit /b 0

:PATCH_LIBRAW_MAKEFILE
REM patch the Makefile so that it has the options we need.  This only has to be done once

echo + Patching Makefile with extra options ...

pushd "%XLIB_DIR%"

SET XTMP_FILE=%TEMP%\libraw-makefile-%RANDOM%-%RANDOM%.tmp

IF EXIST "%XTMP_FILE%" (
    echo ERROR: random temp file exists?!  impossible...
    popd
    exit /b 1
)

REM add in the extra features compile flags + LTCG + fp:fast + openmp
REM LibRaw has its own runtime SIMD dispatch, do NOT add /arch flags
REM /fp:fast speeds up demosaic / white balance / color matrix math
REM /GL enables LTCG (link-time code generation)
REM /openmp enables LibRaw internal parallelism (#pragma omp in demosaic etc.)
REM   LibRaw detects _OPENMP and auto-enables LIBRAW_USE_OPENMP (MSVC>=2010)
REM   DLL boundary contains OpenMP runtime, no impact on JPEGView EXE
REM COPT already has /O2 /EHsc /MP
>> "%XTMP_FILE%" echo COPT_OPT=/DUSE_X3FTOOLS /DUSE_6BY9RPI /DUSE_OLD_VIDEOCAMS /openmp /fp:fast /GL

REM concat the file
copy "%XTMP_FILE%" /A + Makefile.msvc /A "%XMAKEFILE%" /A
IF ERRORLEVEL 1 (
    del "%XTMP_FILE%" 2>nul
    popd
    exit /b 1
)

del "%XTMP_FILE%" 2>nul
popd
exit /b 0

:BUILD_COPY_LIBRAW
call "%~dp0vs-init.bat" %1

pushd "%XLIB_DIR%"

nmake.exe -f "%XMAKEFILE%" clean
IF ERRORLEVEL 1 (
    popd
    exit /b 1
)

nmake.exe -f "%XMAKEFILE%"
IF ERRORLEVEL 1 (
    popd
    exit /b 1
)

copy /y "lib\libraw.lib" "%XSRC_DIR%\lib%~2"
IF ERRORLEVEL 1 (
    popd
    exit /b 1
)
copy /y "bin\libraw.dll" "%XSRC_DIR%\bin%~2"
IF ERRORLEVEL 1 (
    popd
    exit /b 1
)

popd
exit /b 0
