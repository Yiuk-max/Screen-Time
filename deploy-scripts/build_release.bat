@echo off
setlocal enabledelayedexpansion

REM ---------- Paths ----------
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "BUILD_DIR=%PROJECT_ROOT%\build\mingw-release"

REM Override with: set QT_DIR=D:\Qt\6.11.1\mingw_64
if "%QT_DIR%"=="" set "QT_DIR=D:\Qt\6.11.1\mingw_64"
if "%MINGW_DIR%"=="" set "MINGW_DIR=D:\Qt\Tools\mingw1310_64"

if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [ERROR] Qt not found at "%QT_DIR%".
    echo [TIP] Set QT_DIR to your Qt MinGW kit, e.g.:
    echo        set QT_DIR=D:\Qt\6.11.1\mingw_64
    exit /b 1
)

set "PATH=%QT_DIR%\bin;%MINGW_DIR%\bin;%PATH%"

echo [INFO] Project root : %PROJECT_ROOT%
echo [INFO] Build dir    : %BUILD_DIR%
echo [INFO] Qt dir       : %QT_DIR%

cmake -S "%PROJECT_ROOT%" -B "%BUILD_DIR%" -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_C_COMPILER="%MINGW_DIR%\bin\gcc.exe" ^
    -DCMAKE_CXX_COMPILER="%MINGW_DIR%\bin\g++.exe"
if errorlevel 1 exit /b 1

cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b 1

for /f "delims=" %%F in ('dir /b /s "%BUILD_DIR%\ScreenTime.exe" 2^>nul') do (
    echo [OK] Release build ready:
    echo      %%F
    exit /b 0
)

echo [ERROR] ScreenTime.exe not found under "%BUILD_DIR%".
exit /b 1
