<#
.SYNOPSIS
    Builds and runs one of the Exotic GUI examples.

.EXAMPLE
    .\scripts\run.ps1                              # runs exotic_demo if present
    .\scripts\run.ps1 -Example exotic_hello_window
    .\scripts\run.ps1 -Config debug
#>
[CmdletBinding()]
param(
    [string]$Example,
    [ValidateSet('debug', 'release')]
    [string]$Config = 'release'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$binDir = Join-Path $root "build\ninja-$Config\bin"

if (-not $Example) {
    $Example = if (Test-Path (Join-Path $binDir 'exotic_demo.exe')) { 'exotic_demo' } else { 'exotic_hello_window' }
}

& (Join-Path $PSScriptRoot 'build.ps1') -Config $Config -Target $Example

$exe = Join-Path $binDir "$Example.exe"
if (-not (Test-Path $exe)) { throw "Example '$Example' was not built ($exe)." }

Write-Host "==> $exe" -ForegroundColor Cyan
& $exe
if ($LASTEXITCODE -ne 0) { throw "$Example exited with $LASTEXITCODE." }
