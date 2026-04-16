@echo off
setlocal enabledelayedexpansion

REM ---------- Paths ----------
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "RELEASE_ROOT=%PROJECT_ROOT%\release-package"
set "APP_DIR=%RELEASE_ROOT%\ScreenTime"
set "EXE_NAME=ScreenTime.exe"

REM Allow optional exe path argument:
set "EXE_PATH=%~1"

if "%EXE_PATH%"=="" (
    for /f "delims=" %%F in ('dir /b /s "%BUILD_DIR%\%EXE_NAME%" 2^>nul ^| findstr /i /v "\\debug\\"') do (
        set "EXE_PATH=%%F"
        goto :found_exe
    )
)

:found_exe
if "%EXE_PATH%"=="" (
    echo [ERROR] Could not find %EXE_NAME% under "%BUILD_DIR%".
    echo [TIP] Build Release first, or pass exe path:
    echo        deploy_release.bat "E:\path\to\ScreenTime.exe"
    exit /b 1
)

where windeployqt >nul 2>nul
if errorlevel 1 (
    echo [ERROR] windeployqt not found in PATH.
    echo [TIP] Open Qt MinGW prompt or add Qt bin directory to PATH.
    exit /b 1
)

echo [INFO] Project root: %PROJECT_ROOT%
echo [INFO] Source exe  : %EXE_PATH%
echo [INFO] Output dir  : %APP_DIR%

if exist "%APP_DIR%" rmdir /s /q "%APP_DIR%"
mkdir "%APP_DIR%"

copy /y "%EXE_PATH%" "%APP_DIR%\%EXE_NAME%" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy executable.
    exit /b 1
)

echo [INFO] Running windeployqt...
windeployqt --release "%APP_DIR%\%EXE_NAME%"
if errorlevel 1 (
    echo [ERROR] windeployqt failed.
    exit /b 1
)

if exist "%PROJECT_ROOT%\icons" (
    xcopy "%PROJECT_ROOT%\icons" "%APP_DIR%\icons\" /E /I /Y >nul
)

echo [OK] Release is ready:
echo      %APP_DIR%
exit /b 0
