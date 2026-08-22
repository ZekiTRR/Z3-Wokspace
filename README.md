# Z3 Workbench

A desktop workbench for the [Z3 SMT solver](https://github.com/Z3Prover/z3):
create constraint problems, edit variables and constraints, solve, and
inspect models — a small specialized IDE for SMT/constraint solving, built
for reverse engineering, CTF, and symbolic reasoning workflows.

```text
var x: BitVec(32)
constraint ((x ^ 0x1337) + 10) == 0x4242
→ Solve → SAT, x = 0x510F
```

## Status

MVP complete (phases 1–9):

- IDE layout, dark theme, DSL editor with live diagnostics + highlighting
- Int / Bool / Real / String / BitVec(8..64), unsigned BV semantics
- Async solving with timeout + Stop (never blocks the UI)
- SAT/UNSAT/UNKNOWN with models, statistics, console log
- `.z3w` project files (JSON, versioned schema), recent projects
- Export: SMT-LIB2 (portable, Z3-free serializer), JSON, TXT; Import: SMT-LIB2

See **[Documentation.md](Documentation.md)** (English) or
**[Documentation.ru.md](Documentation.ru.md)** (Russian) for the complete
user manual: full DSL syntax reference, worked examples, diagnostics catalog,
and file formats. See `ARCHITECTURE.md` for the internal design and
`BUILD.md` for build instructions.

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
GCC / G++    (Windows: MinGW-w64; Linux: system GCC)   — CI: ✓
Clang        (Linux; CI job enabled)                   — CI: ✓
MSVC         not supported yet — future compatibility target only
```

## License

MIT — see `LICENSE`.
