<#
.SYNOPSIS
    Removes build and packaging output.

.DESCRIPTION
    Deletes build/ and dist/. Fetched dependencies live inside build/, so a
    clean also forces GLFW and stb to be downloaded again on the next configure.
#>
[CmdletBinding(SupportsShouldProcess)]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

foreach ($dir in @('build', 'dist')) {
    $path = Join-Path $root $dir
    if (Test-Path $path) {
        if ($PSCmdlet.ShouldProcess($path, 'Remove')) {
            Remove-Item -Recurse -Force $path
            Write-Host "Removed $path" -ForegroundColor Yellow
        }
    }
}

Write-Host "Clean" -ForegroundColor Green
