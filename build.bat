@echo off
setlocal EnableExtensions
cd /d "%~dp0"
title Boomify - Build automatique

echo ========================================
echo        BOOMIFY - BUILD
echo ========================================
echo.

call :find_tools
if defined CMAKE_EXE if defined VS_GENERATOR goto :build

echo [Boomify] Outils de compilation manquants.
echo [Boomify] Installation automatique des composants Microsoft necessaires...
echo.

where winget >nul 2>nul
if errorlevel 1 goto :no_winget

winget install --id Microsoft.VisualStudio.2022.BuildTools -e --source winget --accept-source-agreements --accept-package-agreements --override "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
if errorlevel 1 goto :install_error

call :find_tools
if not defined CMAKE_EXE goto :restart_needed
if not defined VS_GENERATOR goto :restart_needed
goto :build

:find_tools
set "CMAKE_EXE="
set "VS_GENERATOR="
set "VS_VERSION="
for /f "delims=" %%I in ('where cmake.exe 2^>nul') do if not defined CMAKE_EXE set "CMAKE_EXE=%%I"
if not defined CMAKE_EXE if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
if not defined CMAKE_EXE if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
  for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.CMake.Project -find Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe 2^>nul`) do if not defined CMAKE_EXE set "CMAKE_EXE=%%I"
)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
  for /f "usebackq delims=" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion 2^>nul`) do set "VS_VERSION=%%I"
)
if defined VS_VERSION (
  echo %VS_VERSION% | findstr /b "17\." >nul && set "VS_GENERATOR=Visual Studio 17 2022"
)
exit /b 0

:build
echo [Boomify] Environnement pret.
echo [Boomify] Compilation...
if not exist build mkdir build
"%CMAKE_EXE%" -S . -B build -G "%VS_GENERATOR%" -A x64
if errorlevel 1 goto :build_error
"%CMAKE_EXE%" --build build --config Release
if errorlevel 1 goto :build_error

echo.
echo ========================================
echo  BUILD OK - Lancement de Boomify !
echo ========================================
start "" "%~dp0build\Release\Boomify.exe"
exit /b 0

:no_winget
echo [ERREUR] WinGet n'est pas disponible.
pause
exit /b 1

:install_error
echo [ERREUR] Installation automatique des Build Tools impossible.
pause
exit /b 1

:restart_needed
echo [Boomify] Installation terminee. Ferme puis relance build.bat.
pause
exit /b 0

:build_error
echo.
echo [ERREUR] La compilation de Boomify a echoue.
echo Copie les dernieres lignes ici si besoin.
pause
exit /b 1
