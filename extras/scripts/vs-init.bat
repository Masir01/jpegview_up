@echo off
chcp 65001 >nul

IF /I "%~1" EQU "" (
    echo ERROR: Pass in an [arch] to vcvarsall.bat
    exit /b 1
)

REM Save original PATH, reset to minimal before vcvarsall (avoid "input line too long")
SET "XSAVED_PATH=%PATH%"
set "PATH=%SystemRoot%\System32;%SystemRoot%"
call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" %~1

REM Append original PATH after MSVC paths
set "PATH=%PATH%;%XSAVED_PATH%"
SET "XSAVED_PATH="

exit /b 0
