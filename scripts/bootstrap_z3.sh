#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Bootstraps a project-local Z3 installation for Z3 Workbench (Linux/macOS).
# Mirrors scripts/bootstrap_z3.ps1; keep both in sync.
# -----------------------------------------------------------------------------
set -euo pipefail

TAG="z3-5.1.0"
COMMIT_SHA="0b6cdcdbc65da25ef0f73ac9da210574d0f66cf8"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="$REPO_ROOT/ThirdParty/Z3/source"
BUILD_DIR="$REPO_ROOT/ThirdParty/Z3/build"
INSTALL_DIR="$REPO_ROOT/ThirdParty/Z3/install"

echo "== Z3 bootstrap =="
echo "Repo root : $REPO_ROOT"
echo "Tag       : $TAG"

if [ -f "$INSTALL_DIR/lib/cmake/z3/Z3Config.cmake" ]; then
    echo "Z3 is already bootstrapped at $INSTALL_DIR — nothing to do."
    exit 0
fi

if [ ! -d "$SOURCE_DIR/.git" ]; then
    mkdir -p "$(dirname "$SOURCE_DIR")"
    git clone --depth 1 --branch "$TAG" https://github.com/Z3Prover/z3.git "$SOURCE_DIR"
fi

ACTUAL_SHA="$(git -C "$SOURCE_DIR" rev-parse HEAD)"
if [ "$ACTUAL_SHA" != "$COMMIT_SHA" ]; then
    echo "Pinned Z3 commit mismatch: expected $COMMIT_SHA, found $ACTUAL_SHA." >&2
    exit 1
fi

cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    -DZ3_BUILD_LIBZ3_SHARED=OFF \
    -DZ3_BUILD_EXECUTABLE=OFF

cmake --build "$BUILD_DIR"
cmake --install "$BUILD_DIR"

printf '%s\n%s\n' "$TAG" "$COMMIT_SHA" > "$REPO_ROOT/ThirdParty/Z3/VERSION"
echo "Done. Z3 installed into $INSTALL_DIR"
