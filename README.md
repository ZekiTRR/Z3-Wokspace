# Z3 Workbench

A desktop workbench for the [Z3 SMT solver](https://github.com/Z3Prover/z3):
create constraint problems, edit variables and constraints, solve, and
inspect models — a small specialized IDE for SMT/constraint solving, built
for reverse engineering, CTF, and symbolic reasoning workflows.

```text
var x: BitVec(32)
constraint ((x ^ 0x1337) + 10) == 0x4242
→ Solve → SAT, x = 0x4238 ^ 0x1337
```

## Status

Phase 1 (architecture skeleton) — the project builds and runs:

- CMake project with per-toolchain build trees
- Qt-free core library (`Z3WorkbenchCore`) linked against a project-local Z3
- Qt 6 / QML application with a dark IDE-style shell window
- Unit + solver smoke tests (SAT / UNSAT / UNKNOWN / BitVec / model eval)

See `ARCHITECTURE.md` for the design and `BUILD.md` for build instructions.

## Quick start (Windows, MinGW)

```powershell
scripts/bootstrap_z3.ps1     # one-time: build Z3 5.1.0 into ThirdParty/Z3
scripts/configure.ps1        # cmake configure into build/mingw-debug
scripts/build.ps1            # build
scripts/test.ps1             # run tests
scripts/run.ps1              # launch the application
```

Linux: use `scripts/bootstrap_z3.sh` and plain CMake — see `BUILD.md`.

## Supported toolchains

```text
GCC / G++    (Windows: MinGW-w64 13.1.0 shipped with Qt; Linux: system GCC)
Clang        (supported by design; CI matrix in a later phase)
MSVC         not supported yet — future compatibility target only
```

## License

MIT — see `LICENSE`.
