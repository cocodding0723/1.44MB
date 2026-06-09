@echo off
REM build.bat — no-CRT MSVC x64 빌드 + 1.44MB 용량 측정 (rules/10, rules/20)
setlocal enabledelayedexpansion

where cl >nul 2>nul || call :findvc || exit /b 1

if not exist build mkdir build

set OBJS=
set FOUND=0
for %%f in (src\*.c) do (
  set FOUND=1
  cl /nologo /c /O1 /Os /GS- /Gy /utf-8 /TC "%%f" /Fobuild\%%~nf.obj || exit /b 1
  set OBJS=!OBJS! build\%%~nf.obj
)
if "!FOUND!"=="0" (
  echo [build] src\*.c not found - create M1 engine skeleton first.
  exit /b 0
)

link /nologo /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup /NODEFAULTLIB /OPT:REF /OPT:ICF ^
     /OUT:build\game.exe !OBJS! kernel32.lib user32.lib gdi32.lib opengl32.lib winmm.lib || exit /b 1

for %%F in (build\game.exe) do set SIZE=%%~zF
set /a CAP=1474560
set /a TGT=65536
set /a PCAP=SIZE*100/CAP
set /a PTGT=SIZE*100/TGT
echo ==================================================
echo   game.exe = !SIZE! bytes
echo   cap 1,474,560 : !PCAP!%%    target 65,536 : !PTGT!%%
if !SIZE! GTR !CAP! (
  echo   [X] FAIL - over 1.44MB CAP - reduce now ^(rules/10^)
) else (
  if !SIZE! GTR 1179648 ( echo   [!] RED - freeze new features, reduce
  ) else ( echo   [OK] within budget )
)
echo ==================================================
exit /b 0

:findvc
set "VSW=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSW%" goto :novs
"%VSW%" -latest -property installationPath > "%TEMP%\nd_vsp.txt" 2>nul
set /p VSP=<"%TEMP%\nd_vsp.txt"
if not defined VSP goto :novs
call "%VSP%\VC\Auxiliary\Build\vcvars64.bat" >nul
exit /b 0
:novs
echo [build] VS/BuildTools not found. Run from "x64 Native Tools Command Prompt for VS".
exit /b 1
