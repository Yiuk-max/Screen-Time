@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "SKIP_BUILD=0"
set "SKIP_INSTALLER=0"
set "EXE_PATH="

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="--skip-build" set "SKIP_BUILD=1"
if /i "%~1"=="--skip-installer" set "SKIP_INSTALLER=1"
if /i "%~1"=="--exe" (
    set "EXE_PATH=%~2"
    shift
)
shift
goto parse_args

:args_done

for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "VERSION_FILE=%SCRIPT_DIR%version.txt"
for /f "usebackq delims=" %%V in ("%VERSION_FILE%") do set "APP_VERSION=%%V"
if "%APP_VERSION%"=="" set "APP_VERSION=0.1.0"

echo ========================================
echo  Screen Time Release Packager
echo  Version: %APP_VERSION%
echo ========================================
echo.

if "%SKIP_BUILD%"=="0" (
    if "%EXE_PATH%"=="" (
        echo [STEP 1/3] Build Release...
        call "%SCRIPT_DIR%build_release.bat"
        if errorlevel 1 exit /b 1
    ) else (
        echo [STEP 1/3] Build skipped, using provided exe.
    )
) else (
    echo [STEP 1/3] Build skipped.
)

echo.
echo [STEP 2/3] Deploy Qt dependencies...
if "%EXE_PATH%"=="" (
    call "%SCRIPT_DIR%deploy_release.bat"
) else (
    call "%SCRIPT_DIR%deploy_release.bat" "%EXE_PATH%"
)
if errorlevel 1 exit /b 1

if "%SKIP_INSTALLER%"=="1" (
    echo.
    echo [STEP 3/3] Installer skipped.
    goto done
)

echo.
echo [STEP 3/3] Build installer...

set "ISCC="
if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if exist "C:\Program Files\Inno Setup 6\ISCC.exe" set "ISCC=C:\Program Files\Inno Setup 6\ISCC.exe"

if "%ISCC%"=="" (
    where ISCC >nul 2>nul
    if not errorlevel 1 for /f "delims=" %%I in ('where ISCC') do set "ISCC=%%I"
)

if "%ISCC%"=="" (
    echo [WARN] Inno Setup not found. Portable package and zip are still ready.
    echo [TIP] Install Inno Setup 6, then rerun:
    echo        "%ISCC%" "%PROJECT_ROOT%\installer\ScreenTime.iss"
    goto done
)

"%ISCC%" /DMyAppVersion="%APP_VERSION%" "%PROJECT_ROOT%\installer\ScreenTime.iss"
if errorlevel 1 exit /b 1

:done
echo.
echo ========================================
echo  Packaging finished
echo ========================================
echo  Portable : %PROJECT_ROOT%\release-package\ScreenTime
echo  Zip      : %PROJECT_ROOT%\release-package\ScreenTime_%APP_VERSION%.zip
echo  Installer: %PROJECT_ROOT%\release-package\ScreenTime_Setup_%APP_VERSION%.exe
echo ========================================
exit /b 0
