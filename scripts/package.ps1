<#
.SYNOPSIS
    Builds, installs and zips a release of Exotic GUI.

.DESCRIPTION
    Produces dist\exotic-gui-<version>-windows-x64.zip containing the static
    library, the public headers, the CMake package config and the examples.

.EXAMPLE
    .\scripts\package.ps1
#>
[CmdletBinding()]
param(
    [ValidateSet('debug', 'release')]
    [string]$Config = 'release'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

# The version lives in exactly one place: the top-level project() call.
$cmakeText = Get-Content (Join-Path $root 'CMakeLists.txt') -Raw
if ($cmakeText -notmatch 'VERSION\s+(\d+\.\d+\.\d+)') { throw "Could not read the project version from CMakeLists.txt." }
$version = $Matches[1]

& (Join-Path $PSScriptRoot 'build.ps1') -Config $Config

$buildDir = Join-Path $root "build\ninja-$Config"
$stageDir = Join-Path $root "dist\exotic-gui-$version-windows-x64"

if (Test-Path $stageDir) { Remove-Item -Recurse -Force $stageDir }

Write-Host "==> cmake --install" -ForegroundColor Cyan
& cmake --install $buildDir --prefix $stageDir
if ($LASTEXITCODE -ne 0) { throw "Install failed ($LASTEXITCODE)." }

# Ship the example binaries alongside the library so the zip is runnable.
$examples = Join-Path $stageDir 'bin'
New-Item -ItemType Directory -Force $examples | Out-Null
Get-ChildItem (Join-Path $buildDir 'bin') -Filter 'exotic_*.exe' |
    Where-Object { $_.Name -ne 'exotic_tests.exe' } |
    Copy-Item -Destination $examples

$zip = "$stageDir.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path $stageDir -DestinationPath $zip

$size = [math]::Round((Get-Item $zip).Length / 1KB)
Write-Host "Packaged exotic-gui $version -> $zip ($size KB)" -ForegroundColor Green
