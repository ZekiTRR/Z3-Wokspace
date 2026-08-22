# -----------------------------------------------------------------------------
# Bootstraps a project-local Z3 installation for Z3 Workbench.
#
# Steps: check install -> clone pinned tag -> verify commit -> configure ->
# build -> install into ThirdParty/Z3/install.
#
# The pinned version must be kept in sync with cmake/Z3.cmake docs and
# DEPENDENCIES.md. Official Z3 Windows binaries are MSVC-built, so on Windows
# we always build from source with the same MinGW toolchain as the project.
# -----------------------------------------------------------------------------
[CmdletBinding()]
param(
    [string]$Tag = 'z3-5.1.0',
    [string]$CommitSha = '0b6cdcdbc65da25ef0f73ac9da210574d0f66cf8'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$strRepoRoot    = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$strSourceDir   = Join-Path $strRepoRoot 'ThirdParty/Z3/source'
$strBuildDir    = Join-Path $strRepoRoot 'ThirdParty/Z3/build'
$strInstallDir  = Join-Path $strRepoRoot 'ThirdParty/Z3/install'

function Find-FirstExisting([string[]]$arrPaths)
{
    foreach ($strPath in $arrPaths)
    {
        if ($strPath -and (Test-Path -LiteralPath $strPath)) { return $strPath }
    }
    return $null
}

function Resolve-Tool([string]$strName, [string[]]$arrCandidates)
{
    $strFound = Find-FirstExisting $arrCandidates
    if (-not $strFound)
    {
        $oCmd = Get-Command $strName -ErrorAction SilentlyContinue
        if ($oCmd) { $strFound = Split-Path -Parent $oCmd.Source }
    }
    if (-not $strFound) { throw "Required tool not found: $strName" }
    return $strFound
}

# --- toolchain discovery -----------------------------------------------------
# Prefer the MinGW toolchain shipped with Qt so Z3 and the application share
# the same compiler ABI; fall back to whatever is on PATH.
$strMingwBin = Resolve-Tool 'g++.exe' @(
    'E:/Qt/Tools/mingw1310_64/bin',
    'E:/Qt/Tools/mingw900_64/bin'
)
$strNinjaBin = Resolve-Tool 'ninja.exe' @(
    'E:/Qt/Tools/Ninja'
)
$strCMakeBin = Resolve-Tool 'cmake.exe' @(
    'E:/Qt/Tools/CMake_64/bin'
)

$env:PATH = "$strMingwBin;$strNinjaBin;$strCMakeBin;$env:PATH"

Write-Host "== Z3 bootstrap =="
Write-Host "Repo root : $strRepoRoot"
Write-Host "Compiler  : $(& (Join-Path $strMingwBin 'g++.exe') --version | Select-Object -First 1)"
Write-Host "Tag       : $Tag"
Write-Host "Commit    : $CommitSha"

# --- already installed? ------------------------------------------------------
$strConfigProbe = Join-Path $strInstallDir 'lib/cmake/z3/Z3Config.cmake'
if (Test-Path -LiteralPath $strConfigProbe)
{
    Write-Host "Z3 is already bootstrapped at $strInstallDir — nothing to do."
    exit 0
}

# --- clone -------------------------------------------------------------------
if (-not (Test-Path -LiteralPath (Join-Path $strSourceDir '.git')))
{
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $strSourceDir) | Out-Null
    Write-Host "Cloning Z3 ($Tag)..."
    & git clone --depth 1 --branch $Tag https://github.com/Z3Prover/z3.git $strSourceDir
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
}

$strActualSha = (& git -C $strSourceDir rev-parse HEAD).Trim()
if ($strActualSha -ne $CommitSha)
{
    throw ("Pinned Z3 commit mismatch: expected {0}, found {1}. " +
           "Refuse to build an unverified version." -f $CommitSha, $strActualSha)
}

# --- configure / build / install ---------------------------------------------
Write-Host "Configuring Z3..."
& cmake.exe -S $strSourceDir -B $strBuildDir -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_INSTALL_PREFIX="$strInstallDir" `
    -DZ3_BUILD_LIBZ3_SHARED=OFF `
    -DZ3_BUILD_EXECUTABLE=OFF
if ($LASTEXITCODE -ne 0) { throw "Z3 configure failed" }

Write-Host "Building Z3 (this can take several minutes)..."
& cmake.exe --build $strBuildDir
if ($LASTEXITCODE -ne 0) { throw "Z3 build failed" }

Write-Host "Installing Z3..."
& cmake.exe --install $strBuildDir
if ($LASTEXITCODE -ne 0) { throw "Z3 install failed" }

Set-Content -LiteralPath (Join-Path $strRepoRoot 'ThirdParty/Z3/VERSION') `
    -Value "$Tag`n$CommitSha"

Write-Host "Done. Z3 installed into $strInstallDir"
