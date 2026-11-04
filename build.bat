@echo off
setlocal EnableExtensions
cd /d "%~dp0"
title Boomify - Build automatique

echo ========================================
echo       BOOMIFY - BUILD WINDOWS
echo ========================================
echo.

call :find_tools
if not defined CMAKE_EXE goto :install_tools
if not defined VS_GENERATOR goto :install_tools
goto :ensure_git

:install_tools
echo [Boomify] Installation des outils de compilation C++...
where winget >nul 2>nul
if errorlevel 1 goto :no_winget
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --source winget --accept-source-agreements --accept-package-agreements --override "--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended
if errorlevel 1 goto :install_error
call :find_tools
if not defined CMAKE_EXE goto :restart_needed
if not defined VS_GENERATOR goto :restart_needed
goto :ensure_git

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
if defined VS_VERSION echo %VS_VERSION% | findstr /b "17\." >nul && set "VS_GENERATOR=Visual Studio 17 2022"
exit /b 0

:ensure_git
where git.exe >nul 2>nul
if not errorlevel 1 goto :build
echo [Boomify] Git est requis pour recuperer le SDK VST3.
where winget >nul 2>nul
if errorlevel 1 goto :no_git
winget install --id Git.Git -e --source winget --accept-source-agreements --accept-package-agreements
if errorlevel 1 goto :no_git
goto :restart_needed

:build
echo [Boomify] Nettoyage de l'ancienne build...
if exist build rmdir /s /q build
mkdir build
if exist boomify_crash.log del /q boomify_crash.log

echo [Boomify] Configuration (SDK VST3 conserve en cache)...
"%CMAKE_EXE%" -S . -B build -G "%VS_GENERATOR%" -A x64
if errorlevel 1 goto :build_error

echo [Boomify] Compilation Release...
"%CMAKE_EXE%" --build build --config Release
if errorlevel 1 goto :build_error

if not exist "%~dp0build\Release\Boomify.exe" goto :exe_missing
if not exist "%~dp0build\Release\BoomifyVST3Host.exe" goto :host_missing

echo.
echo ========================================
echo    BUILD OK - Lancement de Boomify
echo ========================================
echo.
pushd "%~dp0build\Release"
Boomify.exe
set "APP_EXIT=%ERRORLEVEL%"
popd

echo.
echo [Boomify] Application fermee. Code de sortie : %APP_EXIT%
if exist "%~dp0build\Release\boomify_crash.log" (
  echo.
  echo ===== CRASH LOG =====
  type "%~dp0build\Release\boomify_crash.log"
  echo =====================
)
echo.
pause
exit /b %APP_EXIT%

:exe_missing
echo [ERREUR] Boomify.exe n'a pas ete genere.
pause
exit /b 1

:host_missing
echo [ERREUR] BoomifyVST3Host.exe n'a pas ete genere.
pause
exit /b 1

:no_git
echo [ERREUR] Installez Git for Windows, puis relancez build.bat.
pause
exit /b 1

:no_winget
echo [ERREUR] WinGet n'est pas disponible.
pause
exit /b 1

:install_error
echo [ERREUR] Installation automatique des Build Tools impossible.
pause
exit /b 1

:restart_needed
echo [Boomify] Installation terminee. Fermez puis relancez build.bat.
pause
exit /b 0

:build_error
echo.
echo [ERREUR] La compilation de Boomify a echoue.
pause
exit /b 1
