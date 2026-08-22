# -----------------------------------------------------------------------------
# Launches the application from a configured build directory with the Qt
# runtime DLLs on PATH (development runs; deployment uses windeployqt later).
#
# Usage:
#   scripts/run.ps1                         # build/mingw-debug
#   scripts/run.ps1 -BuildType Release
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

function Find-FirstExisting([string[]]$arrPaths)
{
    foreach ($strPath in $arrPaths)
    {
        if ($strPath -and (Test-Path -LiteralPath $strPath)) { return $strPath }
    }
    return $null
}

$strRepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$strBuildDir = Join-Path $strRepoRoot "build/$Toolchain-$($BuildType.ToLowerInvariant())"
$strExe      = Join-Path $strBuildDir 'Z3Workbench.exe'

if (-not (Test-Path -LiteralPath $strExe))
{
    throw "Executable not found: $strExe. Build the project first."
}

$strQtBin   = Find-FirstExisting @('E:/Qt/6.11.0/mingw_64/bin')
$strMingwBin = Find-FirstExisting @('E:/Qt/Tools/mingw1310_64/bin', 'E:/Qt/Tools/mingw900_64/bin')
if ($strQtBin)    { $env:PATH = "$strQtBin;$env:PATH" }
if ($strMingwBin) { $env:PATH = "$strMingwBin;$env:PATH" }

& $strExe @args
