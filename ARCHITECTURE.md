# Architecture

## Overview

Layered architecture with one dependency direction:

```text
QML Views  →  ViewModels/Controllers  →  Application Services  →  Domain
                                                                        ↑
                                                    Infrastructure ─────┘
                                                    (Z3 adapter, serialization)
```

Hard rules:

- `Z3WorkbenchCore` does not depend on Qt.
- Domain does not depend on Z3, Qt, or GUI.
- Z3 is visible only inside `src/core/solver/z3/`.
- QML contains no solver logic; the GUI never touches raw Z3 types.
- Solving never blocks the GUI thread.

## CMake targets

```text
Z3WorkbenchApp (exe)   → Z3WorkbenchGui
Z3WorkbenchGui (lib)   → Qt6 (Core, Gui, Qml, Quick, QuickControls2)
Z3WorkbenchTests (exe) → Z3WorkbenchCore, doctest, z3::libz3 (smoke test)
Z3WorkbenchCore (lib)  → z3::libz3 (PRIVATE)
```

The QML module (`URI Z3Workbench`) is attached to the executable: resource
initializers of modules declared in static libraries get dropped by the
linker otherwise.

## Directory layout

```text
src/core/    domain/, application/, solver/, parser/, serialization/, utils/
src/gui/     viewmodels/, controllers/, models/ (added in later phases)
src/app/     thin main() + QML module
qml/         Main.qml, views/, panels/, components/, dialogs/
tests/       core/, parser/, solver/, serialization/, integration/
scripts/     bootstrap_z3, configure, build, test, run
cmake/       CompilerWarnings, CompilerOptions, Dependencies, Qt, Z3
ThirdParty/  Z3/{source,build,install} (generated, not committed)
```

## Domain model (Phase 2)

Value types with strong-typed IDs:

```cpp
enum class VariableType { Bool, Int, Real, BitVec, String, Array };
struct Variable    { VariableId id; std::string name; VariableType type; TypeParams params; };
struct Constraint  { ConstraintId id; Expression expr; bool enabled; std::string comment; SourceLocation loc; };
struct Problem     { ProblemId id; std::string name; std::vector<Variable>; std::vector<Constraint>; };
struct Project     { std::string name; std::vector<Problem> problems; };
```

`Expression` is an immutable tree (`std::variant<VariableRef, Constant,
Unary, Binary, Call>`), independent of `z3::expr`.

## Solver abstraction (Phase 4)

```cpp
class ISolver {
public:
    virtual SolverResult solve(const Problem&, const SolverConfig&,
                               std::shared_ptr<ICancellation>) = 0;
    virtual std::string toSmtLib2(const Problem&) const = 0;
    virtual ~ISolver() = default;
};
```

`Z3Solver` creates an isolated `z3::context` per solve request. Conversion
is split into `Z3ExpressionConverter` (Domain → Z3 AST) and
`Z3ModelConverter` (Z3 model → `ModelValue`, with dec/hex/bin for BitVec).
`SolverResult` carries status, model, diagnostics, statistics, timing.

## Parser (Phase 3, done)

```text
Source → Lexer → Parser (recursive descent, Pratt priorities) → statements
       → SemanticAnalyzer (unknown variables, sort/width checks)
       → resolved Domain Expression → Z3 adapter
```

Design decisions:

- The statement layer has its own AST (`ast::VarDecl`, `ast::ConstraintDecl`);
  expressions reuse the domain `Expression` tree directly (unresolved
  `VariableRef`s carry invalid ids) — a parallel AST expression hierarchy
  would duplicate every node type without adding information.
- Operators are stored resolved; BitVec comparisons/division/shifts are
  unsigned.
- Precedence is RE-oriented: bitwise binds tighter than comparison, so
  `x ^ 0x1337 == 0x4242` parses as `(x ^ 0x1337) == 0x4242`.
- Plain integer literals coerce to the surrounding BitVec width
  (`x << 2` with `x: BitVec(8)`), with range checking.
- Int and Real never mix implicitly; constraint roots must be Bool.
- Errors carry line/column; statement-level recovery reports several errors
  per pass. Canonical display: Int constants in decimal, BitVec values as
  `0x…:width`.

DSL: `var <name>: <Type>` and `constraint <expr>` statements; `//` and
`/* */` comments; dec/hex/bin/real/string literals.

## Threading model (Phase 6, done)

```text
GUI thread: solve() → snapshot Problem into SolveJob → queued emit → worker
Worker thread (solver-worker): Z3Solver::solve(job.problem, config, cancel)
Stop: AtomicCancellation.cancel() — cooperative; Z3 :timeout bounds the wait
Result: SolveJobResult via queued signal → GUI thread → panels updated
```

- `SolveJob`/`SolveJobResult` are Qt-facing value structs in the GUI layer;
  domain types stay metatype-free.
- A long-lived `SolverWorker` lives in a dedicated QThread; each request gets
  fresh backend state, so no worker churn is needed.
- Results for a problem that is no longer selected are discarded with a log
  note. Stop while busy sets status CANCELLING until the backend returns.

## Persistence and export (Phases 7-8)

`.z3w` project files are JSON with a schema `version` field (migration
layer). Serialization lives in core (Qt-free) using nlohmann/json (pinned).
Export targets: SMT-LIB2, JSON, TXT; SMT-LIB2 is produced by a dedicated
serializer from the Domain representation and is also viewable in the UI.

## GUI (Phase 5, done)

IDE-style dark layout: menu bar, toolbar, project explorer (left), problem
editor with live diagnostics (center), variables/model panel (right),
console (bottom), status bar. `Theme.qml` is a QML singleton holding the
palette.

- QML handles layout/binding only; all state lives in
  `gui::WorkspaceViewModel`, exposed as the `workspace` context property.
  QML never touches core types beyond what the viewmodel exposes.
- List views are backed by dedicated `QAbstractListModel`s
  (`ProblemsModel`, `VariablesModel`, `DiagnosticsModel`,
  `ConsoleLogModel`); the viewmodel pushes snapshots, models stay dumb.
- Editing re-parses on every change: errors show live in diagnostics while
  the problem keeps its last valid contents (`rebuildProblemFromSource`).
  Solve always runs on the newest text.
- The solver is injected through `ISolver` (`makeDefaultSolver()`), so GUI
  and app never see Z3 headers.
- Shortcuts: Ctrl+N new problem, F5 solve, Shift+F5 stop (Phase 6),
  Ctrl+M SMT-LIB2 viewer, Ctrl+Q quit.

## Testing

doctest + CTest: parser (lexer/parser/positions), core (type checker),
solver (SAT/UNSAT/UNKNOWN, model conversion), serialization (round-trip),
integration (source → solve → model). Phase 1 already covers the full Z3
chain with smoke tests.

## Roadmap

```text
Phase 1  build skeleton, Z3 bootstrap            ✓ done
Phase 2  domain model                            ✓ done
Phase 3  parser + diagnostics                    ✓ done
Phase 4  Z3 solver adapter                       ✓ done
Phase 5  GUI panels, dark theme                  ✓ done
Phase 6  async solving, cancellation, timeout    ✓ done
Phase 7  JSON persistence (.z3w)
Phase 8  SMT-LIB2 / JSON / TXT export, import
Phase 9  polish, syntax highlighting, CI (GCC + Clang matrix)
```
