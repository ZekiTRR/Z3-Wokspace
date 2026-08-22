# Dependencies

All external versions are pinned. Build-time pins live in
`cmake/Dependencies.cmake` (FetchContent) and `scripts/bootstrap_z3.*`;
update both places together.

| Dependency | Version | Source | Integration |
|------------|---------|--------|-------------|
| Qt         | 6.11.0  | preinstalled (`E:/Qt/6.11.0/mingw_64` on Windows) | `find_package(Qt6)`; prefix overridable via `-DCMAKE_PREFIX_PATH` |
| Z3         | 5.1.0   | https://github.com/Z3Prover/z3, tag `z3-5.1.0`, commit `0b6cdcdbc65da25ef0f73ac9da210574d0f66cf8` | project-local build into `ThirdParty/Z3/install`; consumed as `z3::libz3` |
| doctest    | 2.4.12  | https://github.com/doctest/doctest, tag `v2.4.12` | FetchContent at configure time |
| Ninja      | 1.12.1  | `E:/Qt/Tools/Ninja` | build generator |
| CMake      | >= 3.24 | system / `E:/Qt/Tools/CMake_64` | build system |

## Toolchains

Currently supported and tested:

```text
Windows: MinGW-w64 GCC 13.1.0 (shipped with Qt, E:/Qt/Tools/mingw1310_64)
Linux:   GCC, Clang (supported by the build design; CI matrix comes later)
```

**MSVC is not a supported compiler.** The code base and CMake layout are kept
portable so MSVC support can be added later through
`cmake/CompilerWarnings.cmake` / `cmake/CompilerOptions.cmake` and a
separate MSVC-compatible Qt installation — without redesigning the
application.

## Notes

- Z3 is built as a **static** library (`Z3_BUILD_LIBZ3_SHARED=OFF`), so no
  `z3.dll` deployment is required.
- Official Z3 Windows release binaries are MSVC-built and intentionally not
  used.
- Qt is never downloaded or installed by the project; the user-provided
  installation is used as-is.
- Generated binaries (`ThirdParty/Z3/install`, `build/`) are not committed.

## C++ standard

C++20, strict (`CMAKE_CXX_STANDARD_REQUIRED ON`, no compiler extensions).
C++23+ features are avoided to keep GCC and Clang compatibility.
