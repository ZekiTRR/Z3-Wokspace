# Z3 Workbench

![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6.11-41CD52?logo=qt&logoColor=white)
![Z3](https://img.shields.io/badge/Z3-5.1.0-C8A048)
![CMake](https://img.shields.io/badge/CMake-%E2%89%A53.24-064F8C?logo=cmake&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-97CA00)

**English** | [Русский](README.ru.md)

---

A desktop IDE-style **workbench for the [Z3](https://github.com/Z3Prover/z3)
SMT solver**: describe variables and constraints in a small purpose-builtDSL.
Press `Solve`, and inspect the resulting model — built for reverse engineering, CTF, symbolic reasoning and everyday constraint experiments.

```dsl
var x: BitVec(32)

constraint ((x ^ 0x1337) + 10) == 0x4242
```

Press `F5` → **SAT**, `x = 0x0000510f (20751)`.

## Capabilities

**Problem editing**

- Custom DSL: `Int`, `Real`, `Bool`, `String`, `BitVec(1..64)`
- Live diagnostics on every keystroke — syntax *and* type errors with
  line/column positions; the problem keeps its last valid state while you type
- DSL syntax highlighting, monospaced editor, RE-oriented precedence
  (`x ^ 0x1337 == 0x4242` parses the way you expect)

**Solving**

- One-click `Solve` (`F5` / `Ctrl+Enter`) → `SAT` / `UNSAT` / `UNKNOWN`
- Asynchronous execution: the UI never freezes; cooperative `Stop` +
  per-run timeout (5 s default)
- Model viewer for every declared variable; BitVec values shown as
  hex + decimal; click-to-copy
- Console log of every step, solve timing, backend reason on `UNKNOWN`

**Projects & files**

- `.z3w` project format (JSON, versioned schema, migration-ready)
- Recent projects, unsaved-changes indicator, window geometry persistence
- Export a single problem to portable **SMT-LIB2**, structured **JSON**, or a
  human-readable **TXT** listing
- Import SMT-LIB2 back into an editable problem (exporter subset)

**Engineering quality**

- Qt-free core library; Z3 strictly behind an adapter boundary
- Unit + integration tests over parser, type checker, solver, serialization
  (doctest + CTest); zero-warning build (`-Wall -Wextra -Wpedantic`)
- CI matrix: Linux GCC, Linux Clang, Windows MinGW GCC

## Quick start (Windows, MinGW)

```powershell
scripts/bootstrap_z3.ps1   # one-time: build pinned Z3 5.1.0 locally
scripts/configure.ps1      # cmake configure into build/mingw-debug
scripts/build.ps1          # compile
scripts/test.ps1           # run tests
scripts/run.ps1            # launch the application
```

Linux uses `scripts/bootstrap_z3.sh` plus plain CMake. Full instructions,
toolchain details and troubleshooting: [BUILD.md](Documentation/ENG/BUILD.md).

## Documentation

| Document | Contents |
|---|---|
| [User manual](Documentation/ENG/Documentation.md) | Complete guide: DSL syntax in depth, worked examples, diagnostics catalog, file formats |
| [Syntax reference](Documentation/ENG/SYNTAX.md) | Compact one-page language reference: every statement, operator, command |
| [Architecture](Documentation/ENG/ARCHITECTURE.md) | Layers, dependency rules, parser/solver/GUI design, threading model |
| [Build instructions](Documentation/ENG/BUILD.md) | Prerequisites, Windows/Linux commands, Z3 bootstrap, troubleshooting |
| [Dependencies](Documentation/ENG/DEPENDENCIES.md) | Pinned versions, toolchain policy, integration notes |

Russian versions live in [`Documentation/RUS/`](Documentation/RUS/) — see
the [Russian README](README.ru.md).

## Repository map

```text
src/core/    domain, parser, solver (+ Z3 adapter), serialization — no Qt
src/gui/     viewmodels, models, worker thread, highlighting
src/app/     entry point + QML module
qml/         dark IDE shell, panels
tests/       doctest suites mirroring core/parser/solver/serialization
scripts/     bootstrap_z3 · configure · build · test · run
cmake/       warning policy, dependencies, Qt/Z3 discovery
```

## Roadmap

The MVP described above is complete (9 development phases). Next steps:

1. **Advanced solving** — unsat cores, optimization objectives
   (maximize/minimize), push/pop, incremental solving, assumptions
2. **More backends** — CVC5, Boolector behind the existing `ISolver`
   interface; solver options UI (timeout, random seed)
3. **Structured problem editing** — visual variable/constraint mode next to
   the text editor, expression tree inspection
4. **Editor productivity** — search/replace, go-to-definition for variables,
   rename refactoring across constraints
5. **Deeper diagnostics** — statistics panel (conflicts, decisions),
   SMT-LIB2 diffing between backend and portable views
6. **Broader platform story** — signed installers, deployment packaging,
   MSVC toolchain support behind the existing compiler-policy layer

Long-term vision: a full IDE for constraint solving and symbolic execution —
where crackmes, CTF tasks and register-allocation problems get solved in
seconds, not evenings.

## Contributing

Code style, phase workflow and quality gates are described in
[ARCHITECTURE.md](Documentation/ENG/ARCHITECTURE.md) and the documentation
set above. Every change must build warning-free and pass the test suite.

## License

[MIT](LICENSE)
