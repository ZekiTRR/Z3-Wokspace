# -----------------------------------------------------------------------------
# Builds a configured build directory.
#
# Usage:
#   scripts/build.ps1                       # build/mingw-debug
#   scripts/build.ps1 -BuildType Release
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

if (-not (Test-Path -LiteralPath (Join-Path $strBuildDir 'CMakeCache.txt')))
{
    throw "Build directory is not configured: $strBuildDir. Run scripts/configure.ps1 first."
}

$strMingwBin = Find-FirstExisting @('E:/Qt/Tools/mingw1310_64/bin', 'E:/Qt/Tools/mingw900_64/bin')
if ($strMingwBin) { $env:PATH = "$strMingwBin;$env:PATH" }
$strNinjaDir = Find-FirstExisting @('E:/Qt/Tools/Ninja')
if ($strNinjaDir) { $env:PATH = "$strNinjaDir;$env:PATH" }

Write-Host "== Build: $strBuildDir =="
& cmake.exe --build $strBuildDir
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
