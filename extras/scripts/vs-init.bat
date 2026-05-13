@echo off
chcp 65001 >nul

REM this is a routine that would detect and initialize visual studio environment.
REM Supports VS2017, VS2019, VS2022 and later

IF /I "%~1" EQU "" (
    echo ERROR: Pass in an [arch] to be passed to a vcvarsall.bat
    exit /b 1
)

SET XVS_VCVARS_BAT=

REM Try to find vcvarsall.bat in common locations
REM 1. VS2022 on D: drive (most common for newer installations)
IF NOT DEFINED XVS_VCVARS_BAT (
    IF EXIST "D:\Program Files\Microsoft Visual Studio\18\VC\Auxiliary\Build\vcvarsall.bat" (
        SET "XVS_VCVARS_BAT=D:\Program Files\Microsoft Visual Studio\18\VC\Auxiliary\Build\vcvarsall.bat"
    )
)

REM 2. VS2022 on C: drive
IF NOT DEFINED XVS_VCVARS_BAT (
    IF EXIST "%ProgramFiles%\Microsoft Visual Studio\2022\VC\Auxiliary\Build\vcvarsall.bat" (
        SET "XVS_VCVARS_BAT=%ProgramFiles%\Microsoft Visual Studio\2022\VC\Auxiliary\Build\vcvarsall.bat"
    )
)

REM 3. VS2019 on C: drive
IF NOT DEFINED XVS_VCVARS_BAT (
    IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\VC\Auxiliary\Build\vcvarsall.bat" (
        SET "XVS_VCVARS_BAT=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\VC\Auxiliary\Build\vcvarsall.bat"
    )
)

REM 4. Search recursively in Program Files (x86)
IF NOT DEFINED XVS_VCVARS_BAT (
    FOR /F "usebackq tokens=*" %%I IN (`dir /s /b "C:\Program Files (x86)\Microsoft Visual Studio\vcvarsall.bat" 2^>nul`) DO (
        SET "XVS_VCVARS_BAT=%%I"
    )
)

REM 5. Search recursively in Program Files
IF NOT DEFINED XVS_VCVARS_BAT (
    FOR /F "usebackq tokens=*" %%I IN (`dir /s /b "D:\Program Files\Microsoft Visual Studio\vcvarsall.bat" 2^>nul`) DO (
        SET "XVS_VCVARS_BAT=%%I"
    )
)

IF NOT DEFINED XVS_VCVARS_BAT (
    echo ERROR: vcvarsall.bat not found!
    echo Please install Visual Studio or set XVS_INIT_VER to specify location.
    exit /b 1
)

echo == Using: %XVS_VCVARS_BAT% ==
call "%XVS_VCVARS_BAT%" %~1

exit /b 0
