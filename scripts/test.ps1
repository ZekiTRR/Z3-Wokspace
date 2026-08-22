# -----------------------------------------------------------------------------
# Runs the test suite for a configured build directory.
#
# Usage:
#   scripts/test.ps1                         # default: build/mingw-debug
#   scripts/test.ps1 -BuildType Release      # build/mingw-release
# -----------------------------------------------------------------------------
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$BuildType = 'Debug',
    [ValidateSet('mingw', 'clang')]
    [string]$Toolchain = 'mingw'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$strRepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$strBuildDir = Join-Path $strRepoRoot "build/$Toolchain-$($BuildType.ToLowerInvariant())"

function Find-FirstExisting([string[]]$arrPaths)
{
    foreach ($strPath in $arrPaths)
    {
        if ($strPath -and (Test-Path -LiteralPath $strPath)) { return $strPath }
    }
    return $null
}

# Runtime DLL discovery for tests linked against shared Qt (none today, but
# keep the environment consistent with run-time expectations).
$strQtBin = Find-FirstExisting @('E:/Qt/6.11.0/mingw_64/bin')
if ($strQtBin) { $env:PATH = "$strQtBin;$env:PATH" }

Write-Host "== Test ($Toolchain / $BuildType) =="
& ctest.exe --test-dir $strBuildDir --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
