# Screen Time - MSIX packaging helper
#
# Produces a Store-ready unsigned package (Partner Center re-signs it) plus a
# .msixupload bundle. Local signing is only needed for sideload testing.
#
# Usage (or use the pack_msix.bat wrapper):
#   pack_msix.bat                                   Build unsigned Store package
#   pack_msix.bat -IdentityName 12345Yiuk.ScreenTime -Publisher "CN=..."
#   pack_msix.bat -Sign -DevSign                    Sign with a generated dev cert
#   pack_msix.bat -Sign -PfxPath cert.pfx -PfxPassword secret
#   pack_msix.bat -InstallCertificate -Sign -DevSign
#
# Store identity values come from Partner Center -> Product identity.
# Keep them in installer\msix\store-identity.json; command-line values win.

[CmdletBinding()]
param(
    [string]$Version,
    [string]$SourceDir,
    [string]$Manifest,
    [string]$OutputDir,
    [string]$IdentityName,
    [string]$Publisher,
    [string]$DisplayName,
    [string]$PublisherDisplayName,
    [string]$StoreId,
    [switch]$Sign,
    [string]$PfxPath,
    [string]$PfxPassword,
    [string]$TimestampUrl,
    [switch]$DevSign,
    [switch]$InstallCertificate,
    [switch]$NoUpload
)

$ErrorActionPreference = 'Stop'

$scriptDir   = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = (Resolve-Path (Join-Path $scriptDir '..')).Path

if (-not $SourceDir) { $SourceDir = Join-Path $projectRoot 'release-package\ScreenTime' }
if (-not $Manifest)  { $Manifest  = Join-Path $projectRoot 'installer\msix\AppxManifest.xml' }
if (-not $OutputDir) { $OutputDir = Join-Path $projectRoot 'release-package\msix' }

if ($PfxPath -or $PfxPassword -or $DevSign) { $Sign = $true }
if (-not $PfxPassword -and $env:MSIX_PFX_PASSWORD) { $PfxPassword = $env:MSIX_PFX_PASSWORD }

function Write-Step([string]$Message) {
    Write-Host ""
    Write-Host "==== $Message" -ForegroundColor Cyan
}

function Find-SdkTool([string]$Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }

    $roots = @()
    if (${env:ProgramFiles(x86)}) { $roots += (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin') }
    if ($env:ProgramFiles)        { $roots += (Join-Path $env:ProgramFiles 'Windows Kits\10\bin') }

    foreach ($root in $roots) {
        if (-not (Test-Path $root)) { continue }
        $versionDirs = Get-ChildItem -Path $root -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
            Sort-Object { [version]$_.Name } -Descending
        foreach ($dir in $versionDirs) {
            $candidate = Join-Path $dir.FullName "x64\$Name"
            if (Test-Path $candidate) { return $candidate }
        }
    }
    return $null
}

function Convert-ToPackageVersion([string]$Value) {
    $parts = @($Value.Trim().Split('.'))
    while ($parts.Count -lt 4) { $parts += '0' }
    if ($parts.Count -gt 4) { $parts = $parts[0..3] }
    foreach ($part in $parts) {
        if ($part -notmatch '^\d+$') { throw "Invalid version: $Value" }
        if ([int]$part -gt 65535)    { throw "Version component out of range (0-65535): $Value" }
    }
    return ($parts -join '.')
}

# A certificate usable for MSIX signing must be an end-entity certificate
# (a CA certificate such as the legacy installer\YiukLabs.pfx is rejected by
# Windows with 0x800B0109 / 0x80096019).
function Test-CodeSigningCertificate($cert) {
    if (-not $cert -or -not $cert.HasPrivateKey) { return $false }
    $bcExt = $cert.Extensions | Where-Object { $_.Oid.Value -eq '2.5.29.19' }
    if ($bcExt) {
        $bc = New-Object System.Security.Cryptography.X509Certificates.X509BasicConstraintsExtension($bcExt, $false)
        if ($bc.CertificateAuthority) { return $false }
    }
    return $true
}

# --- Store identity (optional config file, command line wins) --------------
$identityFile = Join-Path (Split-Path -Parent $Manifest) 'store-identity.json'
if (Test-Path $identityFile) {
    try {
        $identity = Get-Content -Path $identityFile -Raw | ConvertFrom-Json
        if (-not $IdentityName -and $identity.identityName) { $IdentityName = $identity.identityName }
        if (-not $Publisher -and $identity.publisher) { $Publisher = $identity.publisher }
        if (-not $DisplayName -and $identity.displayName) { $DisplayName = $identity.displayName }
        if (-not $PublisherDisplayName -and $identity.publisherDisplayName) {
            $PublisherDisplayName = $identity.publisherDisplayName
        }
        if (-not $StoreId -and $identity.storeId) { $StoreId = $identity.storeId }
    } catch {
        Write-Warning "Could not parse $identityFile : $($_.Exception.Message)"
    }
}
if (-not $IdentityName)        { $IdentityName = 'Yiuk.TheScreenTime' }
if (-not $Publisher)           { $Publisher = 'CN=6C42CCA0-F9A8-4179-A164-0152ECF29CAD' }
if (-not $DisplayName)         { $DisplayName = 'The Screen Time' }
if (-not $PublisherDisplayName) { $PublisherDisplayName = 'Yiuk' }
if (-not $StoreId)             { $StoreId = '9N99N8P4VR3H' }

# --- Resolve tools ---------------------------------------------------------
$makeAppx = Find-SdkTool 'makeappx.exe'
if (-not $makeAppx) {
    throw "makeappx.exe not found. Install the Windows 10/11 SDK (App Certification Kit)."
}
$signTool = Find-SdkTool 'signtool.exe'
if ($Sign -and -not $signTool) {
    throw "signtool.exe not found but signing was requested."
}

# --- Resolve version -------------------------------------------------------
if (-not $Version) {
    $versionFile = Join-Path $scriptDir 'version.txt'
    if (Test-Path $versionFile) {
        $Version = (Get-Content -Path $versionFile -TotalCount 1).Trim()
    }
}
if (-not $Version) { $Version = '2.0' }
$packageVersion = Convert-ToPackageVersion $Version

# --- Validate inputs -------------------------------------------------------
if (-not (Test-Path $SourceDir)) {
    throw "Deployed app folder not found: $SourceDir`nRun deploy-scripts\deploy_release.bat first."
}
if (-not (Test-Path $Manifest)) { throw "AppxManifest.xml not found: $Manifest" }

$layoutDir = Join-Path $OutputDir 'layout'
$msixPath  = Join-Path $OutputDir ("ScreenTime_{0}.msix" -f $Version)

Write-Host "========================================"
Write-Host " Screen Time MSIX packager"
Write-Host " Version : $Version ($packageVersion)"
Write-Host " Identity : $IdentityName"
Write-Host " Publisher: $Publisher"
Write-Host " Name     : $DisplayName"
Write-Host " Publisher display name: $PublisherDisplayName"
Write-Host " Store ID : $StoreId"
Write-Host " Source   : $SourceDir"
Write-Host " Output  : $msixPath"
Write-Host "========================================"

# --- Stage layout ----------------------------------------------------------
Write-Step "Staging package layout"
if (Test-Path $layoutDir) { Remove-Item -Path $layoutDir -Recurse -Force }
New-Item -ItemType Directory -Path $layoutDir -Force | Out-Null
if (-not (Test-Path $OutputDir)) { New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null }

Copy-Item -Path (Join-Path $SourceDir '*') -Destination $layoutDir -Recurse -Force

# Generate every logo referenced by the manifest from icons\app.png.
$assetsDir = Join-Path $layoutDir 'Assets'
New-Item -ItemType Directory -Path $assetsDir -Force | Out-Null
$assetGenerator = Join-Path $scriptDir 'generate_msix_assets.ps1'
if (-not (Test-Path $assetGenerator)) {
    throw "MSIX asset generator not found: $assetGenerator"
}
& powershell -NoProfile -ExecutionPolicy Bypass -File $assetGenerator -OutputDir $assetsDir
if ($LASTEXITCODE -ne 0 -or -not (Test-Path (Join-Path $assetsDir 'StoreLogo.png'))) {
    throw "MSIX asset generation failed."
}

# --- Resolve signing certificate (optional) --------------------------------
$pfxToUse         = $null
$pfxPasswordToUse = $null
$certificatePath  = $null
$signerSubject    = $null

if ($Sign) {
    Write-Step "Resolving signing certificate"

    if ($PfxPath -and (Test-Path $PfxPath)) {
        $candidates = @()
        if ($PfxPassword) { $candidates += $PfxPassword } else { $candidates += @('123456', '') }
        foreach ($candidate in $candidates) {
            try {
                $probe = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($PfxPath, $candidate)
            } catch {
                $probe = $null
            }
            if (-not $probe -or -not $probe.HasPrivateKey) { continue }
            if (Test-CodeSigningCertificate $probe) {
                $pfxToUse = $PfxPath
                $pfxPasswordToUse = $candidate
                $signerSubject = $probe.Subject
                Write-Host "Using certificate: $($probe.Subject) [$($probe.Thumbprint)]"
            } else {
                Write-Warning "'$PfxPath' is a CA certificate (BasicConstraints CA=TRUE) and cannot sign MSIX."
            }
            break
        }
    }

    if (-not $pfxToUse) {
        Write-Step "Generating a development code-signing certificate"
        $devPfx = Join-Path $OutputDir 'ScreenTimeDev.pfx'
        $devCer = Join-Path $OutputDir 'ScreenTimeDev.cer'
        if (Test-Path $devPfx) { Remove-Item $devPfx -Force }
        if (Test-Path $devCer) { Remove-Item $devCer -Force }

        # The signing certificate subject must exactly match Package/Identity/Publisher.
        $devCert = New-SelfSignedCertificate `
            -Type CodeSigningCert `
            -Subject $Publisher `
            -FriendlyName 'Screen Time MSIX Development' `
            -KeyExportPolicy Exportable `
            -CertStoreLocation 'Cert:\CurrentUser\My' `
            -NotAfter (Get-Date).AddYears(5)
        $emptyPassword = New-Object System.Security.SecureString
        Export-PfxCertificate -Cert $devCert -FilePath $devPfx -Password $emptyPassword | Out-Null
        Export-Certificate -Cert $devCert -FilePath $devCer | Out-Null
        Get-ChildItem 'Cert:\CurrentUser\My' | Where-Object { $_.Thumbprint -eq $devCert.Thumbprint } | Remove-Item -Force

        $pfxToUse = $devPfx
        $pfxPasswordToUse = ''
        $signerSubject = $devCert.Subject
        $certificatePath = $devCer
        Write-Host "Generated development certificate: $devPfx"
        Write-Host "Subject: $($devCert.Subject) [$($devCert.Thumbprint)]"
    }

    if ($pfxToUse -and -not $certificatePath) {
        $cerCandidate = [System.IO.Path]::ChangeExtension($pfxToUse, '.cer')
        if (Test-Path $cerCandidate) { $certificatePath = $cerCandidate }
    }

    if ($signerSubject -and $Publisher -ne $signerSubject) {
        throw "Signing certificate subject '$signerSubject' does not match Store publisher '$Publisher'."
    }
}

# --- Manifest --------------------------------------------------------------
Write-Step "Writing AppxManifest.xml"
# Read/write as explicit UTF-8 (PowerShell 5.1 would otherwise use the ANSI
# code page and corrupt non-ASCII characters in the manifest).
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$manifestText = [System.IO.File]::ReadAllText($Manifest, [System.Text.Encoding]::UTF8)
$manifestText = $manifestText.Replace('__MSIX_VERSION__', $packageVersion)
$manifestText = $manifestText.Replace('__MSIX_IDENTITY_NAME__', $IdentityName)
$manifestText = $manifestText.Replace('__MSIX_PUBLISHER__', $Publisher)
$manifestText = $manifestText.Replace('__MSIX_DISPLAY_NAME__', $DisplayName)
$manifestText = $manifestText.Replace('__MSIX_PUBLISHER_DISPLAY_NAME__', $PublisherDisplayName)
if ($manifestText -match '__MSIX_[A-Z_]+__') {
    throw "AppxManifest.xml contains an unresolved MSIX placeholder."
}

# Validate Store-reserved identity values before creating the package.
[xml]$manifestXml = $manifestText
$namespace = New-Object System.Xml.XmlNamespaceManager($manifestXml.NameTable)
$namespace.AddNamespace('f', 'http://schemas.microsoft.com/appx/manifest/foundation/windows10')
$identityNode = $manifestXml.SelectSingleNode('/f:Package/f:Identity', $namespace)
$propertiesNode = $manifestXml.SelectSingleNode('/f:Package/f:Properties', $namespace)
if (-not $identityNode -or $identityNode.GetAttribute('Name') -ne $IdentityName) {
    throw "Manifest Identity/Name does not match '$IdentityName'."
}
if ($identityNode.GetAttribute('Publisher') -ne $Publisher) {
    throw "Manifest Identity/Publisher does not match '$Publisher'."
}
if (-not $propertiesNode) {
    throw "Manifest Properties element is missing."
}
$manifestDisplayName = $propertiesNode.SelectSingleNode('f:DisplayName', $namespace).InnerText
$manifestPublisherName = $propertiesNode.SelectSingleNode('f:PublisherDisplayName', $namespace).InnerText
if ($manifestDisplayName -cne $DisplayName) {
    throw "Manifest Properties/DisplayName must exactly match reserved name '$DisplayName'."
}
if ($manifestPublisherName -cne $PublisherDisplayName) {
    throw "Manifest Properties/PublisherDisplayName must exactly match '$PublisherDisplayName'."
}
if ($manifestText -notmatch 'windows\.startupTask' -or
    $manifestText -notmatch 'ScreenTimeStartupTask') {
    throw "AppxManifest.xml is missing the ScreenTime startup task."
}
if ($manifestText -match 'ImmediateRegistration' -or $manifestText -match 'rescap5') {
    throw "AppxManifest.xml uses restricted immediateRegistration capability."
}

$manifestOut = Join-Path $layoutDir 'AppxManifest.xml'
[System.IO.File]::WriteAllText($manifestOut, $manifestText, $utf8NoBom)

# --- Pack ------------------------------------------------------------------
Write-Step "Packing MSIX"
if (Test-Path $msixPath) { Remove-Item -Path $msixPath -Force }
& $makeAppx pack /d $layoutDir /p $msixPath /o
if ($LASTEXITCODE -ne 0) { throw "makeappx failed with exit code $LASTEXITCODE" }

# --- Sign (optional) -------------------------------------------------------
if ($Sign) {
    Write-Step "Signing MSIX"
    $signArgs = @('sign', '/fd', 'SHA256', '/f', $pfxToUse)
    if ($pfxPasswordToUse) { $signArgs += @('/p', $pfxPasswordToUse) }
    if ($TimestampUrl) { $signArgs += @('/tr', $TimestampUrl, '/td', 'SHA256') }
    $signArgs += $msixPath

    & $signTool @signArgs
    if ($LASTEXITCODE -ne 0) { throw "signtool failed with exit code $LASTEXITCODE" }
}

# --- Store upload bundle ---------------------------------------------------
$uploadPath = $null
if (-not $NoUpload) {
    Write-Step "Creating Store upload bundle (.msixupload)"
    $tempUpload = Join-Path $OutputDir '_upload_tmp'
    if (Test-Path $tempUpload) { Remove-Item $tempUpload -Recurse -Force }
    New-Item -ItemType Directory -Path $tempUpload -Force | Out-Null
    Copy-Item -Path $msixPath -Destination $tempUpload -Force
    $uploadPath = Join-Path $OutputDir ("ScreenTime_{0}.msixupload" -f $Version)
    if (Test-Path $uploadPath) { Remove-Item $uploadPath -Force }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::CreateFromDirectory($tempUpload, $uploadPath)
    Remove-Item $tempUpload -Recurse -Force
}

# --- Optional: trust the signing certificate (sideload testing only) -------
if ($InstallCertificate -and $certificatePath -and (Test-Path $certificatePath)) {
    Write-Step "Installing certificate into LocalMachine trust stores"
    try {
        Import-Certificate -FilePath $certificatePath -CertStoreLocation 'Cert:\LocalMachine\TrustedPeople' | Out-Null
        Import-Certificate -FilePath $certificatePath -CertStoreLocation 'Cert:\LocalMachine\Root' | Out-Null
        Write-Host "Certificate trusted for this machine."
    } catch {
        Write-Warning "Could not install certificate (run as administrator): $($_.Exception.Message)"
    }
}

Write-Step "Done"
Write-Host "MSIX package  : $msixPath"
if ($uploadPath) { Write-Host "Store upload  : $uploadPath" }
if (-not $Sign) {
    Write-Host ""
    Write-Host "The package is unsigned. That is what the Microsoft Store expects:"
    Write-Host "upload the .msixupload in Partner Center and it will be signed there."
    Write-Host "For local sideload testing use: pack_msix.bat -Sign -DevSign -InstallCertificate"
} elseif ($certificatePath) {
    Write-Host "Certificate   : $certificatePath"
    Write-Host ""
    Write-Host "First local install (as administrator, once):"
    Write-Host "  Import-Certificate -FilePath `"$certificatePath`" -CertStoreLocation Cert:\LocalMachine\TrustedPeople"
    Write-Host "  Import-Certificate -FilePath `"$certificatePath`" -CertStoreLocation Cert:\LocalMachine\Root"
    Write-Host "Then install:"
    Write-Host "  Add-AppxPackage -Path `"$msixPath`""
}
