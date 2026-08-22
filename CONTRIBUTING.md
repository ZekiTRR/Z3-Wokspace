# Contributing

## Workflow

Work proceeds in phases (see `ARCHITECTURE.md`); every phase must compile
and pass tests before the next one starts:

```text
configure → build → test → review
```

## Code style

- C++20, no compiler extensions.
- Hungarian notation, applied consistently and reasonably:

```cpp
int iCount;
bool bEnabled;
std::string strName;
Variable* pVariable;
std::unique_ptr<Project> upProject;
// members:
m_strName; m_bEnabled; m_vecConstraints; m_upSolver;
```

- Classes/methods: PascalCase/camelCase (`SolverResult`, `solve()`,
  `loadProject()`).
- Constants: `k_` + type prefix (`k_strVersion`, `k_stMaxVariables`).
- Comments explain *why*, not *what*; English only; 1-2 lines preferred.
- RAII everywhere; no raw owning pointers, no manual `new/delete`.
- `std::unique_ptr` by default; `std::shared_ptr` only for real shared
  ownership.
- Strong types for IDs, scoped enums, `[[nodiscard]]`, const correctness.

## Architecture rules

- Core stays Qt-free; Domain stays Z3-free and Qt-free.
- No God objects; UI never touches `z3::*` types.
- Compiler-specific code goes to `cmake/` or a small platform layer — never
  into Domain/Application/Core.

## Warnings

The build targets zero warnings (`-Wall -Wextra -Wpedantic`). Local
suppression is allowed only with an explanatory comment. Check with:

```powershell
cmake -DZ3WORKBENCH_WARNINGS_AS_ERRORS=ON ...
```

## Tests

Significant components require unit/integration tests (doctest). Run:

```powershell
scripts/test.ps1
```

## Commits

Small, focused commits per phase step; generated artifacts (`build/`,
`ThirdParty/Z3/install`) are never committed.
