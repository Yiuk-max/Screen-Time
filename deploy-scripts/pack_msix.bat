@echo off
setlocal enabledelayedexpansion

REM ---------- Pack Screen Time as MSIX (unsigned, sideload) ----------
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "RELEASE_ROOT=%PROJECT_ROOT%\release-package"
set "APP_DIR=%RELEASE_ROOT%\ScreenTime"
set "MSIX_OUT_DIR=%RELEASE_ROOT%\msix"
set "STAGING_DIR=%RELEASE_ROOT%\_msix_staging"
set "VERSION_FILE=%SCRIPT_DIR%version.txt"
set "MANIFEST_TEMPLATE=%PROJECT_ROOT%\installer\msix\AppxManifest.xml"

for /f "usebackq delims=" %%V in ("%VERSION_FILE%") do set "APP_VERSION=%%V"
if "%APP_VERSION%"=="" set "APP_VERSION=0.1.0"

if not exist "%APP_DIR%\ScreenTime.exe" (
    echo [ERROR] Portable release not found: "%APP_DIR%"
    echo [TIP] Run deploy-scripts\deploy_release.bat first.
    exit /b 1
)

REM MSIX version must be 4-part: major.minor.build.revision
set "MSIX_VERSION=%APP_VERSION%.0"

echo [INFO] App version  : %APP_VERSION%
echo [INFO] MSIX version  : %MSIX_VERSION%
echo [INFO] Source dir    : %APP_DIR%
echo [INFO] Output dir    : %MSIX_OUT_DIR%

rmdir /s /q "%STAGING_DIR%" 2>nul
mkdir "%STAGING_DIR%"
mkdir "%STAGING_DIR%\Assets"
if not exist "%MSIX_OUT_DIR%" mkdir "%MSIX_OUT_DIR%"

echo [INFO] Copying application files...
robocopy "%APP_DIR%" "%STAGING_DIR%" /MIR /NFL /NDL /NJH /NJS
if errorlevel 1 (
    echo [ERROR] Failed to copy files to staging directory.
    exit /b 1
)

echo [INFO] Generating MSIX assets...
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%generate_msix_assets.ps1" -OutputDir "%STAGING_DIR%\Assets"
if errorlevel 1 exit /b 1

echo [INFO] Writing AppxManifest.xml...
powershell -NoProfile -Command ^
  "$v='%MSIX_VERSION%'; $t=Get-Content -Raw -Encoding UTF8 '%MANIFEST_TEMPLATE%'; $t=$t -replace '__MSIX_VERSION__',$v; Set-Content -Path '%STAGING_DIR%\AppxManifest.xml' -Value $t -Encoding UTF8"
if errorlevel 1 exit /b 1

set "MAKEAPPX="
if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.22621.0\x64\makeappx.exe" (
    set "MAKEAPPX=%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.22621.0\x64\makeappx.exe"
)
if "%MAKEAPPX%"=="" (
    for /f "delims=" %%K in ('dir /b /ad /o-n "%ProgramFiles(x86)%\Windows Kits\10\bin\10.*" 2^>nul') do (
        if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%K\x64\makeappx.exe" (
            set "MAKEAPPX=%ProgramFiles(x86)%\Windows Kits\10\bin\%%K\x64\makeappx.exe"
            goto found_makeappx
        )
    )
)
:found_makeappx
if "%MAKEAPPX%"=="" (
    where makeappx >nul 2>nul
    if not errorlevel 1 for /f "delims=" %%I in ('where makeappx') do set "MAKEAPPX=%%I"
)

if "%MAKEAPPX%"=="" (
    echo [ERROR] makeappx.exe not found.
    echo [TIP] Install Windows 10/11 SDK and enable "MSIX Packaging Tools".
    rmdir /s /q "%STAGING_DIR%" 2>nul
    exit /b 1
)

set "MSIX_PATH=%MSIX_OUT_DIR%\ScreenTime_%APP_VERSION%.msix"
if exist "%MSIX_PATH%" del /f /q "%MSIX_PATH%"

echo [INFO] Running makeappx pack...
echo [DEBUG] START MAKEAPPX
"%MAKEAPPX%" pack /d "%STAGING_DIR%" /p "%MSIX_PATH%" /o
echo [DEBUG] MAKEAPPX EXIT CODE: %errorlevel%
pause
if errorlevel 1 (
    echo [ERROR] makeappx pack failed.
    exit /b 1
)
set "SIGNTOOL="

if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe" (
    set "SIGNTOOL=%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"
)

if "%SIGNTOOL%"=="" (
    for /f "delims=" %%K in ('dir /b /ad /o-n "%ProgramFiles(x86)%\Windows Kits\10\bin\10.*" 2^>nul') do (
        if exist "%ProgramFiles(x86)%\Windows Kits\10\bin\%%K\x64\signtool.exe" (
            set "SIGNTOOL=%ProgramFiles(x86)%\Windows Kits\10\bin\%%K\x64\signtool.exe"
            goto found_signtool
        )
    )
)

:found_signtool

:: 自动寻找证书
set "PFX_PATH=%PROJECT_ROOT%\installer\YiukLabs.pfx"

if exist "%PFX_PATH%" (
    echo [INFO] Signing package...

    "%SIGNTOOL%" sign ^
    /fd SHA256 ^
    /f "%PFX_PATH%" ^
    /p 123456 ^
    "%MSIX_PATH%"

    if errorlevel 1 (
        echo [ERROR] Package signing failed.
        exit /b 1
    )
) else (
    echo [WARN] Certificate not found:
    echo        %PFX_PATH%
    echo [WARN] Package will remain unsigned.
)
rmdir /s /q "%STAGING_DIR%" 2>nul

echo.
echo [OK] MSIX package ready:
echo      %MSIX_PATH%
echo [TIP] Install unsigned package (Developer Mode or sideload):
echo      Add-AppxPackage -Path "%MSIX_PATH%"
exit /b 0
