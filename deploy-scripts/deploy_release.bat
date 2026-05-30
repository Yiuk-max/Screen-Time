@echo off
setlocal enabledelayedexpansion

REM ---------- Paths ----------
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "RELEASE_ROOT=%PROJECT_ROOT%\release-package"
set "APP_DIR=%RELEASE_ROOT%\ScreenTime"
set "EXE_NAME=ScreenTime.exe"
set "VERSION_FILE=%SCRIPT_DIR%version.txt"

for /f "usebackq delims=" %%V in ("%VERSION_FILE%") do set "APP_VERSION=%%V"
if "%APP_VERSION%"=="" set "APP_VERSION=0.1.0"

set "EXE_PATH=%~1"
set "BEST_SCORE=-1"

if "%EXE_PATH%"=="" (
    for /f "delims=" %%F in ('dir /b /s "%BUILD_DIR%\%EXE_NAME%" 2^>nul') do (
        set "CAND=%%F"
        set "SCORE=0"
        echo !CAND! | findstr /i "mingw-release" >nul && set /a SCORE+=150
        echo !CAND! | findstr /i "\\Release\\" >nul && set /a SCORE+=120
        echo !CAND! | findstr /i "\\release\\" >nul && set /a SCORE+=120
        echo !CAND! | findstr /i "-Release\\" >nul && set /a SCORE+=120
        echo !CAND! | findstr /i "\\bin\\" >nul && set /a SCORE+=20
        echo !CAND! | findstr /i "Debug" >nul && set /a SCORE-=200
        echo !CAND! | findstr /i "debug" >nul && set /a SCORE-=200
        if !SCORE! gtr !BEST_SCORE! (
            set "BEST_SCORE=!SCORE!"
            set "EXE_PATH=!CAND!"
        )
    )
)

if "%EXE_PATH%"=="" (
    echo [ERROR] Could not find %EXE_NAME% under "%BUILD_DIR%".
    echo [TIP] Build Release first:
    echo        deploy-scripts\build_release.bat
    echo [TIP] Or pass exe path:
    echo        deploy_release.bat "D:\path\to\ScreenTime.exe"
    exit /b 1
)

if /i not "%QT_DIR%"=="" (
    set "PATH=%QT_DIR%\bin;%PATH%"
)

where windeployqt >nul 2>nul
if errorlevel 1 (
    if exist "D:\Qt\6.11.1\mingw_64\bin\windeployqt.exe" (
        set "PATH=D:\Qt\6.11.1\mingw_64\bin;%PATH%"
    )
)

where windeployqt >nul 2>nul
if errorlevel 1 (
    echo [ERROR] windeployqt not found in PATH.
    echo [TIP] Open Qt MinGW prompt or set QT_DIR, e.g.:
    echo        set QT_DIR=D:\Qt\6.11.1\mingw_64
    exit /b 1
)

echo [INFO] Project root : %PROJECT_ROOT%
echo [INFO] Version      : %APP_VERSION%
echo [INFO] Source exe   : %EXE_PATH%
echo [INFO] Output dir   : %APP_DIR%

if exist "%APP_DIR%" (
    echo [INFO] Cleaning old release folder...
    rmdir /s /q "%APP_DIR%" 2>nul
    if exist "%APP_DIR%" (
        echo [ERROR] Cannot remove "%APP_DIR%".
        echo [TIP] Close Screen Time if it is running from the release folder, then retry.
        exit /b 1
    )
)
mkdir "%APP_DIR%"

copy /y "%EXE_PATH%" "%APP_DIR%\%EXE_NAME%" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy executable.
    exit /b 1
)

echo [INFO] Running windeployqt...
windeployqt --release --no-translations "%APP_DIR%\%EXE_NAME%"
if errorlevel 1 (
    echo [ERROR] windeployqt failed.
    exit /b 1
)

if exist "%PROJECT_ROOT%\icons" (
    xcopy "%PROJECT_ROOT%\icons" "%APP_DIR%\icons\" /E /I /Y >nul
)

set "ZIP_PATH=%RELEASE_ROOT%\ScreenTime_%APP_VERSION%.zip"
if exist "%ZIP_PATH%" del /f /q "%ZIP_PATH%"

echo [INFO] Creating zip for GitHub release / auto-update...
powershell -NoProfile -Command "Compress-Archive -Path '%APP_DIR%\*' -DestinationPath '%ZIP_PATH%' -Force"
if errorlevel 1 (
    echo [ERROR] Failed to create zip package.
    exit /b 1
)

echo.
echo [OK] Portable release ready:
echo      %APP_DIR%
echo [OK] Update zip ready:
echo      %ZIP_PATH%
exit /b 0
