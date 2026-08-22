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

## Persistence (Phase 7, done)

`.z3w` project files are JSON with a `version` field:

```json
{
    "version": 1,
    "name": "Example",
    "problems": [
        { "name": "crackme_01", "source": "var x: BitVec(32)\nconstraint ..." }
    ]
}
```

- `JsonProjectStorage` lives in core (`serialization/`, Qt-free) and returns
  typed outcomes (`StorageOutcome` + `StorageError{Io, Format, Version}`);
  no exceptions cross the boundary.
- **The DSL source is the single source of truth**: loading rebuilds each
  problem through the real parser, so the format cannot drift from the
  language. A stored source that no longer parses is reported as a format
  error naming the problem.
- Migration chain hook: `migrateDocument()` upgrades documents stepwise;
  files written by newer schema versions are rejected with a clear message.
- GUI wiring: Ctrl+O / Ctrl+S / Ctrl+Shift+S, native file dialogs, dirty
  tracking (`*` in title/toolbar), busy-guarded operations.
  `Project::adoptProblem` moves loaded problems into the project.

## Export / Import (Phase 8, done)

```text
Domain Problem → SmtLib2Serializer → .smt2   (Z3-free, portable output)
.smt2 → SmtLib2Reader → expressions → DslPrinter → DSL source → normal pipeline
Domain Problem → ProblemExporter {SmtLib2 | Json | Txt} → files
```

- The serializer infers types from the declared variables, so BitVec
  operations map to unsigned functions (bvult, bvudiv, bvneg, ...) and
  negation picks the right form per type.
- The reader accepts the exporter's subset (declare-const/assert over the
  workbench operator set); unknown commands are skipped, unknown operators
  are reported with line numbers.
- Imported problems become first-class: DslPrinter generates editable DSL
  source, then everything (validation, persistence, solving) works as usual.
- GUI: File ▸ Export Problem ▸ SMT-LIB2/JSON/TXT, File ▸ Import SMT-LIB2,
  toolbar "Export SMT2"; the Ctrl+M viewer keeps showing the backend view
  (`ISolver::toSmtLib2`, what Z3 actually receives).

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

## CI (Phase 9)

GitHub Actions, GCC/Clang only (`.github/workflows/ci.yml`):

```text
linux-gcc / linux-clang: apt Qt6 + libz3-dev → configure → build → ctest
windows-mingw:           MSYS2 MINGW64 pacman (gcc, ninja, qt6, z3) → same steps
[Future] MSVC job — documented extension point; must reuse the same steps
```

CI uses system/package-manager Z3; the project-local bootstrap remains the
local-development path. Both flows converge on the same CMake targets.

## Polish delivered in Phase 9

- DSL syntax highlighting (`DslHighlighter`, block-comment states,
  palette mirrors Theme.qml).
- Recent projects (QSettings, File ▸ Open Recent) and persistent window
  geometry.
- Remaining future work: editor search, settings dialog, unsat cores,
  additional solver backends.

## Roadmap

```text
Phase 1  build skeleton, Z3 bootstrap            ✓ done
Phase 2  domain model                            ✓ done
Phase 3  parser + diagnostics                    ✓ done
Phase 4  Z3 solver adapter                       ✓ done
Phase 5  GUI panels, dark theme                  ✓ done
Phase 6  async solving, cancellation, timeout    ✓ done
Phase 7  JSON persistence (.z3w)                 ✓ done
Phase 8  SMT-LIB2 / JSON / TXT export, import    ✓ done
Phase 9  polish, syntax highlighting, CI         ✓ done (GCC/Clang matrix)
```
