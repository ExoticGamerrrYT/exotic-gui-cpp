<#
.SYNOPSIS
    Configures and builds Exotic GUI with CMake + Ninja + MSVC.

.EXAMPLE
    .\scripts\build.ps1                       # release build
    .\scripts\build.ps1 -Config debug         # debug build, warnings are errors
    .\scripts\build.ps1 -Target exotic_demo   # build a single target
    .\scripts\build.ps1 -Clean                # wipe the build tree first
#>
[CmdletBinding()]
param(
    [ValidateSet('debug', 'release')]
    [string]$Config = 'release',
    [string]$Target,
    [switch]$Clean,
    [switch]$Fresh
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'vsdev.ps1')

$preset = "ninja-$Config"
$buildDir = Join-Path $root "build\$preset"

if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Removing $buildDir" -ForegroundColor Yellow
    Remove-Item -Recurse -Force $buildDir
}

$configureArgs = @('-S', $root, '--preset', $preset)
if ($Fresh) { $configureArgs += '--fresh' }

Write-Host "==> cmake $($configureArgs -join ' ')" -ForegroundColor Cyan
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed ($LASTEXITCODE)." }

$buildArgs = @('--build', '--preset', $preset)
if ($Target) { $buildArgs += @('--target', $Target) }

Write-Host "==> cmake $($buildArgs -join ' ')" -ForegroundColor Cyan
& cmake @buildArgs
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)." }

Write-Host "Build succeeded -> $buildDir\bin" -ForegroundColor Green
