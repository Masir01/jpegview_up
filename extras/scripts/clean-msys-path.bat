@echo off
REM This script removes MSYS/MSVC paths from PATH that conflict with VS build tools
for /f "tokens=*" %%a in ('powershell -NoProfile -Command "$p=[Environment]::GetEnvironmentVariable('Path','Process') -split ';';$p | Where-Object { $_ -notmatch 'msys|mingw|ucrt64|git' -and $_ -ne '' } | Select-Object -First 200; ($p | Select-Object -First 200) -join ';'"') do set "PATH=%%a"
