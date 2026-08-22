# Build instructions

## Prerequisites

| Tool      | Version  | Notes                                              |
|-----------|----------|----------------------------------------------------|
| CMake     | >= 3.24  | system CMake or `E:/Qt/Tools/CMake_64`             |
| Ninja     | any      | `E:/Qt/Tools/Ninja` on Windows                     |
| GCC (MinGW-w64) | 13.1.0 | `E:/Qt/Tools/mingw1310_64` on Windows          |
| Qt        | 6.11.0   | `E:/Qt/6.11.0/mingw_64` on Windows (see below)     |
| Z3        | 5.1.0    | bootstrapped into `ThirdParty/Z3` automatically    |
| git       | any      | required by the Z3 bootstrap                       |

The Qt location is a CMake default in the root `CMakeLists.txt` and can be
overridden without editing files:

```bash
cmake -S . -B build/custom -DCMAKE_PREFIX_PATH="<other Qt>/lib/cmake"
```

## Windows (MinGW)

```powershell
scripts/bootstrap_z3.ps1                 # one-time Z3 build (~5-10 min)
scripts/configure.ps1                    # Debug; add -BuildType Release
scripts/configure.ps1 -BuildType Release
scripts/build.ps1                        # add -BuildType Release
scripts/test.ps1
scripts/run.ps1
```

Raw CMake equivalent:

```powershell
$env:PATH = "E:\Qt\Tools\mingw1310_64\bin;E:\Qt\Tools\Ninja;$env:PATH"
cmake -S . -B build/mingw-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/mingw-debug
ctest --test-dir build/mingw-debug --output-on-failure
```

Running the app from a plain shell needs Qt DLLs on PATH:

```powershell
$env:PATH = "E:\Qt\6.11.0\mingw_64\bin;E:\Qt\Tools\mingw1310_64\bin;$env:PATH"
build\mingw-debug\Z3Workbench.exe
```

## Linux

```bash
scripts/bootstrap_z3.sh
cmake -S . -B build/gcc-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/gcc-debug
ctest --test-dir build/gcc-debug --output-on-failure
```

Qt 6 (Core, Gui, Qml, Quick, QuickControls2) must be installed through the
system package manager; adjust `CMAKE_PREFIX_PATH` if Qt is in a
non-standard prefix.

## Build directories

CMake fixes the toolchain at configure time, so each toolchain/configuration
gets its own build directory:

```text
build/mingw-debug    build/mingw-release
build/clang-debug    build/clang-release
```

`compile_commands.json` is exported and copied to the repository root by
`scripts/configure.ps1` for clangd/IDE tooling.

## Z3 bootstrap details

`scripts/bootstrap_z3.ps1` / `.sh`:

1. skip if `ThirdParty/Z3/install/lib/cmake/z3/Z3Config.cmake` exists;
2. clone the pinned tag into `ThirdParty/Z3/source` (shallow);
3. verify the pinned commit SHA (refuse to build unverified sources);
4. configure + build + install a static `libz3` into `ThirdParty/Z3/install`.

On Windows, Z3 is always built from source with the same MinGW toolchain as
the project: the official Z3 Windows binaries are MSVC-built and cannot be
linked by GCC/Clang.

## Troubleshooting

- **"Z3 was not found" at configure time** — run the bootstrap script, or
  point to an existing installation:
  `cmake -DZ3WORKBENCH_Z3_ROOT=<install prefix>`.
- **App fails to start with missing DLL errors** — Qt/MinGW bin directories
  are not on PATH (see the Windows section above).
- **Wrong compiler picked up** — make sure `E:/Qt/Tools/mingw1310_64/bin`
  precedes other toolchains (MSYS2 etc.) in PATH for this project.
- **Warnings as errors** — off by default; enable with
  `-DZ3WORKBENCH_WARNINGS_AS_ERRORS=ON`.
