@echo off
chcp 65001 >nul

call "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

msbuild.exe /property:Platform=x64 /property:configuration=Release /v:m "%~dp0..\..\src\JPEGView.sln"

exit /b %ERRORLEVEL%
