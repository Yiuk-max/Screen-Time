param(
    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    [string]$SourceIcon
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Resolve-Path (Join-Path $scriptDir "..")
if ([string]::IsNullOrWhiteSpace($SourceIcon)) {
    $SourceIcon = Join-Path $projectRoot "icons\app.png"
}
if (-not (Test-Path -LiteralPath $SourceIcon)) {
    throw "Source icon not found: $SourceIcon"
}

function New-IconPng {
    param(
        [string]$Source,
        [int]$Width,
        [int]$Height,
        [string]$Path
    )

    $sourceImage = [System.Drawing.Image]::FromFile($Source)
    try {
        $bitmap = New-Object System.Drawing.Bitmap $Width, $Height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.Clear([System.Drawing.Color]::Transparent)
            $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality

            $scale = [Math]::Min($Width / $sourceImage.Width, $Height / $sourceImage.Height)
            $drawWidth = [int][Math]::Round($sourceImage.Width * $scale)
            $drawHeight = [int][Math]::Round($sourceImage.Height * $scale)
            $x = [int][Math]::Floor(($Width - $drawWidth) / 2)
            $y = [int][Math]::Floor(($Height - $drawHeight) / 2)

            $graphics.DrawImage($sourceImage, $x, $y, $drawWidth, $drawHeight)
            $ms = New-Object System.IO.MemoryStream
$bitmap.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
[System.IO.File]::WriteAllBytes($Path, $ms.ToArray())
$ms.Dispose()
        } finally {
            $graphics.Dispose()
            $bitmap.Dispose()
        }
    } finally {
        $sourceImage.Dispose()
    }
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
New-IconPng -Source $SourceIcon -Width 16 -Height 16 -Path (Join-Path $OutputDir "AppIcon16x16.png")
New-IconPng -Source $SourceIcon -Width 32 -Height 32 -Path (Join-Path $OutputDir "AppIcon32x32.png")
New-IconPng -Source $SourceIcon -Width 44 -Height 44 -Path (Join-Path $OutputDir "Square44x44Logo.png")
New-IconPng -Source $SourceIcon -Width 150 -Height 150 -Path (Join-Path $OutputDir "Square150x150Logo.png")
New-IconPng -Source $SourceIcon -Width 310 -Height 150 -Path (Join-Path $OutputDir "Wide310x150Logo.png")
New-IconPng -Source $SourceIcon -Width 512 -Height 512 -Path (Join-Path $OutputDir "StoreLogo.png")
New-IconPng -Source $SourceIcon -Width 512 -Height 512 -Path (Join-Path $OutputDir "StoreLogo512x512.png")

Write-Host "[OK] MSIX assets generated from $SourceIcon in $OutputDir"
