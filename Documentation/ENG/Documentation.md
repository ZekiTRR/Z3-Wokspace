# Z3 Workbench — User Documentation

This document is the complete user manual for **Z3 Workbench**: a desktop
workbench for the [Z3](https://github.com/Z3Prover/z3) SMT solver. It covers
the DSL syntax in full detail, explains how to create and solve problems,
catalogs every diagnostic message, and documents all file formats.

A Russian version of this document is available: [Documentation.ru.md](../RUS/Documentation.ru.md).

---

## Table of contents

1. [Introduction](#1-introduction)
2. [Quick start](#2-quick-start)
3. [User interface](#3-user-interface)
4. [DSL syntax reference](#4-dsl-syntax-reference)
5. [Creating and solving a problem](#5-creating-and-solving-a-problem)
6. [Worked examples](#6-worked-examples)
7. [Reading results](#7-reading-results)
8. [Diagnostics catalog](#8-diagnostics-catalog)
9. [Files: projects, export, import](#9-files-projects-export-import)
10. [Settings and stored state](#10-settings-and-stored-state)
11. [Keyboard shortcuts](#11-keyboard-shortcuts)
12. [Known limitations](#12-known-limitations)

---

## 1. Introduction

### 1.1 What is Z3 Workbench?

Z3 Workbench is a graphical IDE-style tool built around the Z3 theorem
prover. You describe a **problem** — a set of *variables* and *constraints*
over those variables — press `Solve`, and receive one of three outcomes:

| Outcome | Meaning |
|---|---|
| **SAT** | SAT (from *satisfiable*) — at least one solution exists. The workbench shows a concrete solution (a *model*). |
| **UNSAT** | UNSAT (from *unsatisfiable*) — no solution exists; the constraints contradict each other. |
| **UNKNOWN** | The solver could not decide within its budget (for example, the timeout expired on a very hard problem). |

### 1.2 Key terms

| Term | Definition |
|---|---|
| **SMT** | SMT (*Satisfiability Modulo Theories*) — the task of deciding whether a logical formula is satisfiable over background theories such as integers, bit-vectors or strings. |
| **Solver** | The engine that decides satisfiability. Z3 Workbench uses Microsoft's Z3. |
| **Problem** | A named set of variables plus constraints. Stored inside a project. |
| **Variable** | A named value with a type (`Int`, `Bool`, …). The solver assigns values to variables. |
| **Constraint** | A Boolean expression over variables that must hold in every solution. |
| **Model** | A model (also called an assignment) — a concrete table `variable → value` that satisfies all constraints. Shown after SAT. |
| **Sort** | Sort — the formal name for a value domain/type (`Int`, `BitVec(32)`, …). |
| **Literal** | Literal — a written constant: `42`, `0x1337`, `true`, `"text"`. |
| **DSL** | DSL (*Domain-Specific Language*) — the small language this application defines for writing problems by hand. |

### 1.3 Design principles

- **The DSL source text is authoritative.** Every problem keeps its editable
  DSL text; parsed entities are rebuilt from it. Project files store exactly
  this text.
- **Nothing reaches Z3 unvalidated.** Text → lexer → parser → semantic
  analysis (types, widths, unknown names) → internal expression tree → Z3.
- **The UI never blocks.** Solving runs on a worker thread; `Stop` cancels
  cooperatively.

---

## 2. Quick start

From launch to your first solved problem:

1. **Start the application.** One starter problem (`problem_1`) already
   exists and contains:
   ```dsl
   var x: Int

   constraint x > 0
   ```
2. **Replace the editor contents** with the crackme example below. This is
   the classic reverse-engineering shape "find x such that…":
   ```dsl
   var x: BitVec(32)

   constraint ((x ^ 0x1337) + 10) == 0x4242
   ```
3. While typing, watch the **DIAGNOSTICS** area under the editor: it lists
   syntax and type errors live, with line numbers. When it is empty, the
   problem parses cleanly.
4. Press **`F5`** (or click **Solve**, or press `Ctrl+Enter`).
5. Read the result:
   - Status bar turns green: `Status: SAT`, with solve time next to it.
   - The right panel fills in: `x = 0x0000510f (20751)`.
   - The console at the bottom logs each step.

You have just solved `(x ^ 0x1337) + 10 == 0x4242`: the answer is
`x = 0x510F`. Click the value to copy it to the clipboard.

---

## 3. User interface

```text
┌ MenuBar ── File / Solver / Help ────────────────────────────────────────────┐
│ Toolbar   [New] [Open…] [Save] │ [Solve (F5)] [Stop] │ [SMT-LIB2] [Export] │
├──────────────┬──────────────────────────────────────┬───────────────────────┤
│ PROJECT      │ PROBLEM EDITOR                       │ VARIABLES / MODEL     │
│ EXPLORER     │                                      │                       │
│              │   var x: BitVec(32)                  │ Name    Type       Value
│ ▸ problem_1  │                                      │ x       BitVec(32) 0x0000510f (20751)
│ ▸ crackme_01 │   constraint ((x ^ 0x1337) + 10)...  │                       │
│              ├──────────────────────────────────────┤                       │
│ [New][Dup]   │ DIAGNOSTICS                          │                       │
│ [Rename][Del]│ Line 2: Unknown variable "y"         │                       │
├──────────────┴──────────────────────────────────────┴───────────────────────┤
│ CONSOLE                                                                     │
│ [20:41:02] INFO Starting Z3...                                              │
│ [20:41:02] INFO Result: SAT                                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│ Status: SAT        Time: 8 ms                                    Z3 Workbench │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Panels

| Panel | Purpose |
|---|---|
| **Project Explorer** | Lists all problems of the current project. Click selects; buttons create, duplicate, rename, delete. Rename validates the identifier rules (§4.2) and uniqueness. |
| **Problem Editor** | The DSL text of the selected problem. Monospaced font, DSL syntax highlighting (keywords, types, literals, comments). Every edit re-parses immediately. |
| **Diagnostics** | Under the editor. Live list of errors/warnings with line/column positions. While diagnostics are non-empty, solving is refused. Importantly: while the text has errors, the problem internally keeps its **last valid state**, so you can keep typing freely. |
| **Variables / Model** | Declared variables with types. After a SAT run, a `Value` column appears; clicking a value copies it to the clipboard. BitVec values show hex padded to full width plus decimal in parentheses. |
| **Console** | Timestamped log of everything: parsing results, solver start, outcome, reasons, export/import operations. Levels: `INFO`, `WARNING`, `ERROR`. Auto-scrolls. |
| **Status bar** | Current status with color coding: green `SAT`, yellow `UNSAT`, blue `UNKNOWN`, red `ERROR`/`INVALID`, blue accent `Solving…`/`Cancelling…`. Solve time when available. |

---

## 4. DSL syntax reference

The DSL (domain-specific language) has exactly **two kinds of statements**:

```ebnf
program     = { statement } ;
statement   = var-decl | constraint ;
var-decl    = "var" identifier ":" type ;
constraint  = "constraint" expression ;
type        = "Bool" | "Int" | "Real" | "String" | "BitVec" "(" number ")" ;
expression  = ... infix operators over literals and identifiers ...
```

Everything else — comments, whitespace layout, statement order — is free.

### 4.1 Program structure

- Statements are **not terminated** by `;` or newlines. A new statement is
  recognized by its keyword (`var` / `constraint`), so you may format freely.
- Statement order does not matter: constraints may be written before the
  variables they use. Internally declarations are collected first, then all
  constraints are resolved against them.
- Keywords are case-sensitive: `var` works, `Var` does not.
- Type names are also keywords: they cannot be used as variable names.

Valid program with unusual formatting:

```dsl
constraint y > x var x: Int var y: Int
```

Recommended style (used throughout this manual):

```dsl
var x: Int
var y: Int

constraint x >= 10
constraint y == x + 20
```

One blank line between the declaration block and constraints is convention,
not syntax.

### 4.2 Identifiers

An identifier (variable name):

```ebnf
identifier = ( letter | "_" ) , { letter | digit | "_" } ;
letter     = "A".."Z" | "a".."z" ;   digit = "0".."9" ;
```

- ASCII letters only; Unicode is not accepted.
- Must not start with a digit.
- Violations produce: ``Invalid variable name '...'``.
- Names must be unique within a problem:
  ``Variable "x" is already defined``.
- Reserved words (`var`, `constraint`, `true`, `false`, `Bool`, `Int`,
  `Real`, `String`, `BitVec`) are lexed as keywords and cannot be used as
  variable names.

### 4.3 Variables — the `var` command

```ebnf
var-decl = "var" identifier ":" type ;
```

Declares a variable and fixes its sort. Five sorts exist:

#### `Bool`

Two-valued logic domain: `true` / `false`.

#### `Int`

Mathematical integers — **arbitrary precision, no overflow**. There is no
32-bit wrapping here; if you need machine arithmetic, use `BitVec`.
Literals are decimal, hexadecimal (`0x…`) or binary (`0b…`).

> Integer division `/` and remainder `%` on `Int` are **not part of the
> DSL** (see §4.7): `/` is reserved for `Real` and `BitVec`.

#### `Real`

Exact rational numbers (not binary floating point!). Values are stored and
computed as rationals, so `1 / 3 * 3 == 1` holds exactly. Literals use
decimal notation with mandatory fractional digits: `3.14`, `0.5`.
Models display reals as decimal strings.

#### `String`

UTF-8 strings. Only equality comparisons are available (§4.7); there are no
string functions (length, concatenation) yet.

#### `BitVec(N)`

Fixed-width bit-vector — machine word semantics. `N` is the width in bits,
from 1 to 64 (`BitVec width must be between 1 and 64`). Typical choices:

```dsl
var flags: BitVec(8)
var port : BitVec(16)
var eax  : BitVec(32)
var ptr  : BitVec(64)
```

Semantics:

- Arithmetic is **modulo 2^N** (wraps around, like C unsigned types):
  `0xFFFFFFFF + 1 == 0` on `BitVec(32)`.
- Comparisons `< <= > >=` are **unsigned** (as in RE tooling conventions).
  Signed variants do not exist in the DSL.
- Division `/` and remainder `%` are **unsigned** (`bvudiv`/`bvurem`).
- Shifts `<< >>` are **logical** (`bvlshr` for `>>`; there is no
  arithmetic/signed shift).
- Unary minus `-` is **not applicable** to BitVec in the DSL (§4.7).

### 4.4 Constraints — the `constraint` command

```ebnf
constraint = "constraint" expression ;
```

Adds an assertion: a Boolean expression that must hold in every solution.
The root of the expression must be Boolean:

```dsl
constraint x >= 10          // OK: comparison yields Bool
constraint x + 1            // ERROR: Constraint must be a Boolean expression, got Int
```

Constraints can be temporarily disabled — currently by deleting/commenting
them out (`//`).

Multiple constraints are combined implicitly with logical AND.

### 4.5 Literals

| Kind | Forms | Examples |
|---|---|---|
| Integer | decimal | `0`, `42`, `-7`* |
| Integer | hexadecimal `0x`/`0X` | `0x1337`, `0Xff` |
| Integer | binary `0b`/`0B` | `0b1010`, `0B11` |
| Real | decimal with fraction digits | `3.14`, `0.5`, `-2.75`* |
| Bool | keywords | `true`, `false` |
| String | double quotes | `"hello"`, `"a\"b"` |

\* negative values are produced by the unary minus operator applied to a
positive literal (§4.7); the literal itself is non-negative.

Rules and traps:

- Hex/binary require at least one digit: `0x` alone →
  ``Hexadecimal literal has no digits``.
- Real literals require digits after the dot: `1.` →
  ``Malformed real literal (missing digits after '.')``.
- A number glued to letters is rejected without cascading:
  `123abc` → ``Malformed number literal``.
- Integers beyond the 64-bit signed range →
  ``Integer literal '...' is out of range``. (For huge constants use
  `BitVec` contexts or split the constant.)
- String escapes: `\\`, `\"`, `\n`, `\t`. Any other escape →
  ``Unknown escape sequence '\X'``. An unterminated string (newline or EOF
  before closing quote) is reported as an error.
- In expressions, plain integer literals automatically adapt to a
  surrounding BitVec context (§4.9).

### 4.6 Comments

```dsl
// line comment — until end of line

/* block comment;
   may span several lines */
var x: Int /* inline */ // trailing comment
```

Block comments do **not nest**. An unterminated `/*` without `*/` is an error.

Comments are allowed anywhere whitespace is allowed.

### 4.7 Operators reference

Unary operators:

| Operator | Operand sorts | Result | Semantics |
|---|---|---|---|
| `!` | `Bool` | `Bool` | Logical negation (`not`) |
| `-` | `Int`, `Real` | same | Arithmetic negation. **Not applicable to `BitVec`** — negation of bit-vectors is not expressible in the DSL yet. |
| `~` | `BitVec(N)` | `BitVec(N)` | Bitwise NOT (`bvnot`), flips all N bits |

Binary operators, grouped by family:

| Operator | Operand sorts | Result | Semantics / backend |
|---|---|---|---|
| `\|\|` | `Bool` × `Bool` | `Bool` | Logical OR |
| `&&` | `Bool` × `Bool` | `Bool` | Logical AND |
| `==` | same sort¹ | `Bool` | Equality (`=`) |
| `!=` | same sort¹ | `Bool` | Inequality (`distinct`) |
| `<` `<=` `>` `>=` | `Int`×`Int`, `Real`×`Real`, `BitVec(W)`×`BitVec(W)` | `Bool` | Ordering. For BitVec: **unsigned** (`bvult`, `bvule`, `bvugt`, `bvuge`). Widths must match. |
| `+` `-` `*` | `Int`, `Real`, `BitVec(W)` (same sort both sides) | same | Arithmetic. On BitVec: wraps modulo 2^W. |
| `/` | `Real`×`Real`, `BitVec(W)`×`BitVec(W)` | same | Exact rational division; **unsigned** division (`bvudiv`). Integer `/` is rejected by the type checker. |
| `%` | `BitVec(W)`×`BitVec(W)` | `BitVec(W)` | Unsigned remainder (`bvurem`). Integer `%` is rejected by the type checker. |
| `&` \| `^` | `BitVec(W)`×`BitVec(W)` | `BitVec(W)` | Bitwise AND/OR/XOR (`bvand`, `bvor`, `bvxor`) |
| `<<` `>>` | `BitVec(W)`×`BitVec(W)` | `BitVec(W)` | Shift left / **logical** shift right (`bvshl`, `bvlshr`) |

¹ Allowed equality sorts: `Bool`, `Int`, `Real`, `String`,
`BitVec(W)` (same width on both sides). Both operands must have identical
sorts; mixing never coerces except the literal rule of §4.9.

All binary operators are **left-associative**.

Notes:

- `&&`/`||` short-circuit evaluation is irrelevant here: the whole formula
  is handed to the solver.
- There are no bitwise operators for `Int` — switch the variable to
  `BitVec`.
- There is no exponentiation operator.

### 4.8 Precedence and associativity

From lowest binding to highest:

| Level | Operators | Comment |
|---|---|---|
| 1 | `\|\|` | lowest |
| 2 | `&&` | |
| 3 | `==` `!=` | |
| 4 | `<` `<=` `>` `>=` | |
| 5 | `&` `\|` `^` | bitwise binds tighter than comparison — unlike C! |
| 6 | `<<` `>>` | |
| 7 | `+` `-` | |
| 8 | `*` `/` `%` | |
| 9 | unary `!` `-` `~` | highest |

Parentheses always override precedence.

Worked disambiguations (canonical fully-parenthesized forms):

| Source | Parses as |
|---|---|
| `x ^ 0x1337 == 0x4242` | `(x ^ 0x1337) == 0x4242` |
| `1 + 2 * 3 == 7` | `(1 + (2 * 3)) == 7` |
| `true \|\| false && true` | `true \|\| (false && true)` |
| `x & 1 << 2` | `x & (1 << 2)` |
| `-x + y < 0` | `((-x) + y) < 0` |

The first row is the deliberate design choice: bitwise operations bind
tighter than comparisons, so RE-style equations read naturally. In C the
same expression would parse as `x ^ (0x1337 == 0x4242)`.

### 4.9 Automatic BitVec literal adaptation

Inside any operation whose other operand is a `BitVec`, a plain integer
literal is automatically widened to that operand's width:

```dsl
var x: BitVec(32)

constraint (x ^ 0x1337) + 10 == 0x4242   // 0x1337, 10, 0x4242 become BitVec(32)
constraint x < 100                        // 100 becomes BitVec(32)
constraint (x << 2) > 0                   // 2 becomes BitVec(32)
```

Rules:

- Adaptation applies to a **plain integer literal directly used as an
  operand** — not to computed subexpressions.
- Value range check against the width: the literal must satisfy
  `0 ≤ v < 2^W`.
  - Negative literal → ``Negative integer literal cannot be used as BitVec(W)``
  - Too large → ``Integer literal N does not fit into BitVec(W)``
- Without a BitVec context the literal stays `Int`.

### 4.10 Typing rules summary

1. Operands of every binary operator must have identical sorts (the only
   exception being §4.9 literal adaptation).
2. `Int` and `Real` never mix: `r == i + 1.5` where `i: Int`, `r: Real`
   → ``Type mismatch: operator '+' requires numeric operands of the same sort, got Int and Real``.
   Convert explicitly by renaming the variable's declared sort.
3. BitVec operands must share the exact width: `a & b` with
   `a: BitVec(8)`, `b: BitVec(16)` → mismatch naming `BitVec(8)`.
4. Strings support only `==` / `!=`; ordering comparisons on strings are
   rejected.
5. The root of every `constraint` must be `Bool`.

---

## 5. Creating and solving a problem

Typical workflow:

1. **Create a problem** — `Ctrl+N` or Explorer ▸ `New`. Problems get
   auto-generated names (`problem_1`, `problem_2`, …) so you can start
   typing instantly. A starter template is inserted.
2. **Rename** (optional) — select in Explorer, `Rename`, enter a valid
   identifier.
3. **Write variables and constraints** — see §4. Fix diagnostics until the
   list under the editor is empty.
4. **Solve** — `F5`, or toolbar `Solve`, or `Ctrl+Enter` inside the editor.
   - The job runs on a worker thread; the UI stays responsive.
   - While running, the status bar shows `Solving…`, `Solve` is disabled
     and `Stop` enabled.
5. **Inspect the model** (SAT) — right panel; click values to copy.
6. **Iterate** — add/remove constraints, re-solve. To enumerate several
   solutions manually, exclude previous ones, e.g. after learning
   `x = 11`:
   ```dsl
   constraint x != 11
   ```
7. **Persist** — `Ctrl+S` saves the project (`.z3w`). First save asks for a
   location; afterwards the same path is reused (`Ctrl+Shift+S` saves-as).
   Unsaved changes are marked `*` in the title bar and on the Save button.
8. **Export/share** — File ▸ Export Problem ▸ SMT-LIB2 / JSON / TXT (§9).

Problem management summary (Explorer buttons):

| Action | Behavior |
|---|---|
| New | Appends `problem_N`, selects it, inserts the starter template |
| Duplicate | Full copy with fresh internal ids and rewritten variable references; name gets `_copy` suffix |
| Rename | Checks identifier validity and uniqueness |
| Delete | Removes the problem; selection clears if needed |

---

## 6. Worked examples

All examples below parse and solve in the current build.

### 6.1 Simple bounds

```dsl
var x: Int

constraint x >= 10
constraint x <= 100
```

Result: `SAT`, some `x` in `[10, 100]` (e.g. `10`).

### 6.2 System of equations

```dsl
var x: Int
var y: Int
var z: Int

constraint x + y + z == 30
constraint x - y == 2
constraint z == 2 * x
```

Result: `SAT`, e.g. `x = 8, y = 6, z = 16`.

### 6.3 Contradiction (UNSAT)

```dsl
var x: Int

constraint x > 10
constraint x < 5
```

Result: `UNSAT`. Useful to verify the pipeline: no model appears, the
console states that no solution exists.

### 6.4 Boolean puzzle

```dsl
var p: Bool
var q: Bool
var r: Bool

constraint p || q || r
constraint !(p && q)
constraint q == r
```

Result: `SAT`; inspect which assignments satisfy all three clauses.

### 6.5 Classic crackme (BitVec)

Find `x` such that XOR-ing it with `0x1337` and adding 10 gives `0x4242`:

```dsl
var x: BitVec(32)

constraint ((x ^ 0x1337) + 10) == 0x4242
```

Manual derivation: `x = (0x4242 − 10) ^ 0x1337 = 0x4238 ^ 0x1337 = 0x510F`.
The model shows `x = 0x0000510f (20751)` — matching.

Note how integer literals `10` adapted to `BitVec(32)` (§4.9).

### 6.6 Byte extraction and masks (RE style)

```dsl
var v: BitVec(32)

constraint (v & 0xFF) == 0x41                 // low byte is 'A'
constraint ((v >> 8) & 0xFF) == 0x42          // next byte is 'B'
constraint ((v >> 16) & 0xFF) == 0            // third byte zero
constraint (v >> 24) == 0                     // high byte zero
```

Result: `SAT`, `v = 0x00004241 (16961)` — the four constraints pin every
byte, so the answer is unique.

### 6.7 Wrapping arithmetic

```dsl
var a: BitVec(8)

constraint a == 255
constraint a + 1 == 0
```

Result: `SAT` — demonstrates modulo-256 wrap-around. On `Int` the second
line would make the problem UNSAT.

### 6.8 Strings

```dsl
var password: String

constraint password != ""
constraint password == "s3cr3t"
```

Result: `SAT`, `password = s3cr3t`. Only `==`/`!=` are available; there is
no length/substring machinery yet (§12).

### 6.9 Rational arithmetic

```dsl
var total: Real
var part : Real

constraint total == 10
constraint part == total / 4
constraint part * 4 == total
```

Result: `SAT`, `part = 2.5`. Reals are exact rationals; `0.1 + 0.2 == 0.3`
would be SAT (unlike binary floating point).

### 6.10 Hard problem → UNKNOWN via timeout

```dsl
var f1: Int
var f2: Int

constraint f1 > 1
constraint f2 > 1
constraint f1 * f2 == 999999670000016821
```

Factoring a large semiprime exceeds the default budget (5000 ms). Expected
result: `UNKNOWN` with a console diagnostic like
`Reason: timeout` (the exact reason string comes from Z3). This is honest
output — the solver neither proved unsatisfiability nor found a model.

---

## 7. Reading results

### 7.1 Statuses

| Status | Color | Meaning | Model panel |
|---|---|---|---|
| `SAT` | green | Solution found | Values filled |
| `UNSAT` | yellow | No solution exists | Cleared |
| `UNKNOWN` | blue | Undecided (e.g. timeout) | Cleared; `Reason:` logged |
| `ERROR` | red | Backend failure (e.g. unsupported construct) | Cleared |
| `INVALID` | red | Editor text currently has diagnostics | Previous values cleared |
| `Solving…` / `Cancelling…` | accent blue | Job in flight | — |

### 7.2 Model values

- Each declared variable gets a value; unconstrained variables still receive
  an arbitrary satisfying assignment.
- Display formats: `Int` decimal; `Bool` `true/false`; `Real` decimal
  string; `String` raw text; `BitVec(W)` as `0x` + hex padded to W/4 digits
  + decimal in parentheses, e.g. `0x0000510f (20751)`.
- **Click a value to copy it** to the clipboard.
- Only ONE model is returned per run. To search for alternatives, exclude
  known values (§5 step 6).

### 7.3 Console

Every run appends lines similar to:

```text
[20:41:02] INFO    Parsing problem... done
[20:41:02] INFO    Variables: 1, Constraints: 1 (of 1 enabled)
[20:41:02] INFO    Starting Z3...
[20:41:02] INFO    Result: SAT
[20:41:02] INFO    Solver time: 8 ms
```

Warnings/errors appear with their levels; UNKNOWN adds a `Reason:` line
(e.g. timeout). If you switched problems before a job finished, its result
is discarded with a note instead of corrupting the view.

### 7.4 Timing

The status bar shows wall-clock milliseconds of the whole solve request
(parse-to-model). Small problems typically report single-digit
milliseconds.

---

## 8. Diagnostics catalog

Messages appear in the DIAGNOSTICS list with `Line N:` prefixes where
positions are known. Exact texts:

### Lexer

| Message | Cause / fix |
|---|---|
| `Unexpected character '=' (did you mean '=='?)` | Single `=` typed; equality is `==`. |
| `Unexpected character 'X'` | Character outside the DSL alphabet (e.g. `@`, `;`). |
| ``Hexadecimal literal has no digits`` | Bare `0x`. |
| `Binary literal has no digits` | Bare `0b`. |
| `Malformed hexadecimal literal` | Trailing garbage glued to a hex number (`0xFFG`). |
| `Malformed binary literal` | e.g. `0b102`. |
| `Malformed number literal` | Digits followed by letters (`123abc`). The numeric prefix is kept for recovery. |
| ``Malformed real literal (missing digits after '.')`` | `1.` style. |
| `Unknown escape sequence '\X'` | Only `\\ \" \n \t` exist in strings. |
| `Unterminated string literal` | Missing closing quote (strings cannot span lines). |
| `Unterminated block comment` | `/*` without `*/`. |

### Parser

| Message | Cause / fix |
|---|---|
| ``Expected 'var' or 'constraint', found …`` | Statement outside the two forms. Recovery skips to the next keyword. |
| `Expected variable name` / ``Invalid variable name '...'`` | §4.2 rules. |
| ``Expected ':' after variable name`` | `var x Int` → insert `:`. |
| ``Expected '(' after 'BitVec'`` / `Expected bit width` / ``Expected ')' after bit width`` | Malformed `BitVec(...)`. |
| ``BitVec width must be between 1 and 64`` | Width range. |
| ``Expected a type (Bool, Int, Real, String, BitVec), found …`` | Unknown type name. |
| ``Expected ')', found …`` / `Unexpected token …` | Parenthesis/operator misuse. Multiple errors are reported in one pass thanks to per-statement recovery. |
| ``Integer literal '...' is out of range`` | Beyond 64-bit signed. |

### Semantic analysis

| Message | Cause / fix |
|---|---|
| ``Variable "x" is already defined`` | Duplicate `var`. |
| ``Unknown variable "foo"`` | Use before declaration or typo — remember order does not matter, so this means the name truly does not exist. |
| ``Type mismatch: operator 'OP' requires …, got A and B`` | Family violation: e.g. `&&` on Int, `&` on Int, `+` mixing sorts. |
| ``Type mismatch: cannot compare A with B`` | `==`/`!=`/ordering across different sorts or widths. |
| ``Operator 'OP' cannot be applied to TYPE`` | Unary misuse: `-x` on BitVec, `~flag` on Bool, `!x` on Int. |
| ``Constraint must be a Boolean expression, got TYPE`` | Root of `constraint` is not Bool. |
| ``Negative integer literal cannot be used as BitVec(W)`` | §4.9. |
| ``Integer literal N does not fit into BitVec(W)`` | §4.9, v ≥ 2^W. |
| `Function calls are not supported yet: "..."` | Call syntax is reserved. |

### Solver / import

| Message | Cause / fix |
|---|---|
| `Reason: …` after UNKNOWN | Z3's own reason (commonly `timeout`). |
| `Solving was cancelled before it started` | Stop pressed before dispatch. |
| ``Sort 'Array' is not supported yet (variable "…")`` | Array sort is reserved. |
| `Import failed: Line N: …` | SMT-LIB2 reader errors (§9.4). |
| `Open failed: …` / `Save failed: …` | Project file I/O problems (missing file, unwritable path, invalid JSON, wrong schema version). |

---

## 9. Files: projects, export, import

### 9.1 Project files (`.z3w`)

A project bundles all problems. Format: JSON, schema version 1:

```json
{
    "version": 1,
    "name": "Example",
    "problems": [
        {
            "name": "crackme_01",
            "source": "var x: BitVec(32)\n\nconstraint ((x ^ 0x1337) + 10) == 0x4242\n"
        }
    ]
}
```

Design facts worth knowing:

- Only **names + DSL sources** are stored. Variables and constraints are
  rebuilt through the real parser on load, so a project file can never drift
  from the language implementation.
- Loading validates every source; a problem whose text no longer parses is
  reported as `Format` error naming the problem.
- Files written by a newer schema version are rejected with a clear message
  (migration support is built into the loader chain).
- Operations are blocked while a solve is in flight.

### 9.2 Export

File ▸ Export Problem ▸ … exports the **currently selected problem**
(reflecting the newest editor text when it parses).

**SMT-LIB2** (`.smt2`) — portable document, independent of Z3 internals;
usable with any SMT-LIB2 solver:

```lisp
(declare-const x (_ BitVec 32))
(assert (= (+ (bvxor x (_ bv4919 32)) (_ bv10 32)) (_ bv16962 32)))
(check-sat)
(get-model)
```

Notes: BitVec operations map to unsigned functions (`bvudiv`, `bvurem`,
`bvult`…); negation picks `bvneg` vs `-` by inferred sort.

**JSON** (`.json`) — structured dump: name, source, variables (with
widths), constraints with canonical display form, enabled flag, comment,
source line.

**TXT** (`.txt`) — human-readable listing:

```text
Problem: crackme_01

Variables:
  x : BitVec(32)

Constraints:
  [x] ((x ^ 0x1337:32) == 0x4242:32)    (line 3)
```

### 9.3 Two SMT-LIB2 views

| View | Trigger | Content |
|---|---|---|
| **Backend view** | `Ctrl+M` viewer | Rendered by Z3 itself from what the adapter actually passes — best for debugging converter issues. |
| **Portable export** | File ▸ Export ▸ SMT-LIB2 | Produced by the workbench's own serializer from the domain tree — backend-independent, stable formatting. |

### 9.4 Import SMT-LIB2

File ▸ Import SMT-LIB2 reads the subset produced by the exporter:
`declare-const` / `assert` commands over the workbench sorts and operators.

- Unknown commands (`check-sat`, `set-info`, `set-logic`, …) are skipped.
- Unsupported constructs (quantifiers, uninterpreted functions, arrays) are
  reported as `Import failed: Line N: Unsupported operator "…"`.
- The imported problem becomes a **first-class DSL problem**: the reader
  reconstructs expressions, a DSL printer generates editable source, and the
  normal validation/solve/persist pipeline takes over. The problem name is
  derived from the file name (uniquified with a suffix).

### 9.5 Recent projects

The last 10 opened/saved project paths persist across sessions
(File ▸ Open Recent). Entries open with one click.

---

## 10. Settings and stored state

There is no settings dialog yet; persistent state lives in QSettings
(organization `Z3Workbench`, application `Z3 Workbench`):

| Data | Keys | Notes |
|---|---|---|
| Recent projects | `recentProjects` | list, max 10 |
| Window geometry | `window/x`, `window/y`, `window/width`, `window/height` | restored on start; centered fallback when absent |

Storage locations: Windows registry (`HKEY_CURRENT_USER\Software\Z3Workbench`),
Linux `~/.config/Z3Workbench/Z3 Workbench.conf`.

Solver options (timeout 5000 ms, random seed, produce-model) currently use
built-in defaults; a configuration UI is planned.

---

## 11. Keyboard shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+N` | New problem |
| `Ctrl+O` | Open project (.z3w) |
| `Ctrl+S` | Save project (asks path on first save) |
| `Ctrl+Shift+S` | Save project as… |
| `F5` | Solve |
| `Shift+F5` | Stop (enabled only while solving) |
| `Ctrl+Enter` | Solve (works directly from the editor) |
| `Ctrl+M` | View generated SMT-LIB2 (backend view) |
| `Ctrl+Q` | Quit |

---

## 12. Known limitations

Current, deliberate MVP boundaries (all are extension points, not defects):

1. **No BitVec negation/comparison signs in DSL**: unary `-` applies to
   Int/Real only. Emulate subtraction via `+` with adapted literals.
2. **No signed BitVec semantics**: comparisons, `/`, `%`, `>>` are
   unsigned. Signed variants are future ops.
3. **Integer `/` and `%` are not in the DSL**: `/` is Real-only, `%` is
   BitVec-only. Use multiplication-based formulations for integer
   divisibility tasks.
4. **Array sort reserved but unsupported**; likewise floats, enums.
5. **Strings: equality only** — no length/concat/regex yet.
6. **One model per solve**; enumerate alternatives by excluding previous
   values manually.
7. **No function calls** in expressions (syntax reserved).
8. **Solver options UI absent** (timeout fixed at 5000 ms in this build);
   statistics are captured in the result but no advanced panel yet.
9. **Unsat cores / optimization / push-pop** are architecture-ready but not
   exposed yet.

---

*Z3 Workbench · MIT license · [ARCHITECTURE.md](ARCHITECTURE.md) describes
the internal design.*
