<#
.SYNOPSIS
    Runs clang-format over the source tree using the repository .clang-format.

.EXAMPLE
    .\scripts\format.ps1          # rewrite files in place
    .\scripts\format.ps1 -Check   # fail if anything is unformatted
#>
[CmdletBinding()]
param([switch]$Check)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent

$clangFormat = (Get-Command clang-format -ErrorAction SilentlyContinue).Source
if (-not $clangFormat) {
    # Visual Studio ships clang-format with the LLVM toolset component.
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * -property installationPath
        $candidate = Join-Path $vsPath 'VC\Tools\Llvm\bin\clang-format.exe'
        if (Test-Path $candidate) { $clangFormat = $candidate }
    }
}
if (-not $clangFormat) {
    throw "clang-format not found. Install the 'C++ Clang tools for Windows' Visual Studio component."
}

$files = Get-ChildItem -Path (Join-Path $root 'include'), (Join-Path $root 'src'),
    (Join-Path $root 'examples'), (Join-Path $root 'tests') -Recurse -Include *.hpp, *.cpp -File

$formatArgs = if ($Check) { @('--dry-run', '--Werror') } else { @('-i') }
& $clangFormat @formatArgs $files.FullName
if ($LASTEXITCODE -ne 0) { throw "clang-format reported problems ($LASTEXITCODE)." }

Write-Host "$($files.Count) files formatted ($clangFormat)" -ForegroundColor Green
