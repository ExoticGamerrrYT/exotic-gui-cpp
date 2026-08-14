<#
.SYNOPSIS
    Builds and runs the Exotic GUI test suite through CTest.

.EXAMPLE
    .\scripts\test.ps1 -Config debug
#>
[CmdletBinding()]
param(
    [ValidateSet('debug', 'release')]
    [string]$Config = 'debug'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

& (Join-Path $PSScriptRoot 'build.ps1') -Config $Config

Write-Host "==> ctest --preset ninja-$Config" -ForegroundColor Cyan
& ctest --test-dir (Join-Path $root "build\ninja-$Config") --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Tests failed ($LASTEXITCODE)." }

Write-Host "All tests passed" -ForegroundColor Green
