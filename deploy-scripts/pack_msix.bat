@echo off
setlocal

REM Screen Time MSIX packager (wrapper around pack_msix.ps1).
REM
REM By default it produces an UNSIGNED Store-ready package:
REM   release-package\msix\ScreenTime_<version>.msix          (unsigned)
REM   release-package\msix\ScreenTime_<version>.msixupload    (upload to Partner Center)
REM
REM Arguments are forwarded to PowerShell:
REM   pack_msix.bat -IdentityName 12345Yiuk-max.ScreenTime -Publisher "CN=..."
REM   pack_msix.bat -Sign -DevSign -InstallCertificate     (local sideload test)
REM   pack_msix.bat -Sign -PfxPath cert.pfx -PfxPassword secret
REM   pack_msix.bat -NoUpload                               (skip the .msixupload)
REM
REM For the Store, copy Name/Publisher from Partner Center -> App identity.
REM They can also be stored in installer\msix\store-identity.json.

set "SCRIPT_DIR=%~dp0"

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%pack_msix.ps1" %*
exit /b %errorlevel%
