# -----------------------------------------------------------------------------
# Configures the project into a per-toolchain build directory.
#
# Usage:
#   scripts/configure.ps1                          # Debug, mingw
#   scripts/configure.ps1 -BuildType Release
#   scripts/configure.ps1 -Toolchain clang
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

# --- toolchain ---------------------------------------------------------------
# CMake fixes the compiler at configure time; each toolchain gets its own
# build directory so configurations never clash.
$strMingwBin = $null
if ($Toolchain -eq 'mingw')
{
    $strMingwBin = Find-FirstExisting @('E:/Qt/Tools/mingw1310_64/bin', 'E:/Qt/Tools/mingw900_64/bin')
    if (-not $strMingwBin)
    {
        $oCmd = Get-Command g++.exe -ErrorAction SilentlyContinue
        if ($oCmd) { $strMingwBin = Split-Path -Parent $oCmd.Source }
    }
    if (-not $strMingwBin) { throw "MinGW g++.exe not found" }
    $env:PATH = "$strMingwBin;$env:PATH"
}

$strNinjaDir = Find-FirstExisting @('E:/Qt/Tools/Ninja')
if ($strNinjaDir) { $env:PATH = "$strNinjaDir;$env:PATH" }

Write-Host "== Configure: $strBuildDir =="
& cmake.exe -S $strRepoRoot -B $strBuildDir -G Ninja "-DCMAKE_BUILD_TYPE=$BuildType"
if ($LASTEXITCODE -ne 0) { throw "Configure failed" }

# Keep compile_commands.json at the repo root for clangd/IDE tooling.
$strCcSource = Join-Path $strBuildDir 'compile_commands.json'
if (Test-Path -LiteralPath $strCcSource)
{
    Copy-Item -LiteralPath $strCcSource -Destination (Join-Path $strRepoRoot 'compile_commands.json') -Force
    Write-Host "compile_commands.json copied to repo root"
}
