<#
.SYNOPSIS
    Loads the MSVC developer environment into the current PowerShell session.

.DESCRIPTION
    Locates the newest Visual Studio installation that ships the C++ toolset
    and enters its developer shell. Safe to dot-source repeatedly: it returns
    immediately when the requested architecture is already active.

.EXAMPLE
    . .\scripts\vsdev.ps1
#>
[CmdletBinding()]
param(
    [ValidateSet('x64', 'x86', 'arm64')]
    [string]$Arch = 'x64'
)

$ErrorActionPreference = 'Stop'

if ($env:VSCMD_ARG_TGT_ARCH -eq $Arch) {
    Write-Verbose "MSVC environment for $Arch is already active."
    return
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Install Visual Studio 2022+ with the 'Desktop development with C++' workload."
}

$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath

if (-not $vsPath) {
    throw "No Visual Studio installation with the MSVC C++ toolset was found."
}

Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=$Arch" | Out-Null

Write-Host "MSVC environment ready ($Arch)" -ForegroundColor Green
