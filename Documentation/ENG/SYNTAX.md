# Z3 Workbench — Syntax Reference

Complete documentation of all constructs, operators, and commands supported
by the DSL and the Z3 Workbench shell.

For tutorials and worked tasks see
[Documentation.md](Documentation.md);
[ARCHITECTURE.md](ARCHITECTURE.md).

---

## 1. Statements

No terminators; the order of declarations relative to constraints does not
matter.

| Syntax | Example |
|---|---|
| `var NAME : TYPE ;`* | `var x: Int` |
| `constraint EXPR` | `constraint x >= 10` |

\* there is no actual `;` — statements are separated implicitly by keywords.

Minimal program:

```dsl
var x: Int
constraint x > 0
```

---

## 2. Data types

| Syntax | Example |
|---|---|
| `Bool` | `var flag: Bool` |
| `Int` | `var n: Int` |
| `Real` | `var ratio: Real` |
| `String` | `var name: String` |
| `BitVec(N)` | `var eax: BitVec(32)` |

Width rules for `BitVec(N)`:

| Rule | Example |
|---|---|
| N must be 1..64 | `BitVec(32)` — ok · `BitVec(0)`, `BitVec(65)` — rejected |

Reserved, not yet supported types: `Array`, float, enum.

---

## 3. Identifiers

| Rule | Example |
|---|---|
| `[A-Za-z_][A-Za-z0-9_]*`, ASCII only, case-sensitive | `x`, `_tmp`, `eax_01` |
| Unique within a problem | a second `var x` → error |
| Keywords cannot be used as names | `var`, `constraint`, `true`, `false`, `Bool`, `Int`, `Real`, `String`, `BitVec` |

---

## 4. Literals

| Kind | Syntax | Examples |
|---|---|---|
| Integer (dec) | `DIGITS` | `42`, `7` |
| Integer (hex) | `0xHEX` / `0XHEX` | `0x1337`, `0XFF` |
| Integer (bin) | `0bBITS` / `0bBITS` | `0b1010`, `0B11` |
| Real | `D.DD` | `3.14`, `0.5` |
| Boolean | keyword | `true`, `false` |
| String | `"TEXT"` | `"hello"` |

Negative values are written with a minus before a positive
literal: `-7`.

### String escape sequences

| Escape | Meaning | Example |
|---|---|---|
| `\\` | backslash | `"a\\b"` |
| `\"` | double quote | `"say \"hi\""` |
| `\n` | newline | `"line1\nline2"` |
| `\t` | tab | `"a\tb"` |

Any other escape is an error; strings do not span lines.

---

## 5. Comments

| Syntax | Purpose | Example |
|---|---|---|
| `// TEXT` | Line comment, to end of line | `// init` |
| `/* TEXT */` | Block comment, may span several lines, no nesting | `/* bounds */` |

```dsl
/* problem setup */
var x: Int   // start value
```

---

## 6. Unary operators

| Operator | Operand type | Result | Backend | Example |
|---|---|---|---|---|
| `!E` | `Bool` | `Bool` | `not` | `!flag` |
| `-E` | `Int`, `Real` | same | `-` | `-x` |
| `~E` | `BitVec(N)` | `BitVec(N)` | `bvnot` | `~mask` |

Note: minus does **not** apply to `BitVec` in this DSL.

---

## 7. Binary operators (by precedence, low to high)

All binary operators are left-associative. Operands must have identical
types (exception — literal adaptation from §9).

### Level 1 — logical OR

| Operator | Type | Result | Backend | Example |
|---|---|---|---|---|
| `A \|\| B` | `Bool` × `Bool` | `Bool` | `or` | `x < 0 \|\| x > 100` |

### Level 2 — logical AND

| Operator | Type | Result | Backend | Example |
|---|---|---|---|---|
| `A && B` | `Bool` × `Bool` | `Bool` | `and` | `x >= 10 && x <= 20` |

### Level 3 — equality

| Operator | Type | Result | Backend | Example |
|---|---|---|---|---|
| `A == B` | any matching type¹ | `Bool` | `=` | `y == x * 2` |
| `A != B` | any matching type¹ | `Bool` | `distinct` | `s != ""` |

¹ `Bool`, `Int`, `Real`, `String`, `BitVec(N)` with matching widths.

### Level 4 — ordering

| Operator | Type | Result | Backend | Example |
|---|---|---|---|---|
| `A < B` | `Int`/`Real`/equal-width `BitVec` | `Bool` | `<` / `bvult` | `i < len` |
| `A <= B` | same | `Bool` | `<=` / `bvule` | `n <= 9` |
| `A > B` | same | `Bool` | `>` / `bvugt` | `c > 0x41` |
| `A >= B` | same | `Bool` | `>=` / `bvuge` | `v >= 33` |

BitVec comparisons are **unsigned**.

### Level 5 — bitwise AND/OR/XOR

| Operator | Type | Result | Backend | Example |
|---|---|---|---|---|
| `A & B` | pair of `BitVec(N)` | `BitVec(N)` | `bvand` | `v & 0xFF` |
| `A \| B` | pair of `BitVec(N)` | `BitVec(N)` | `bvor` | `flags \| 0x80` |
| `A ^ B` | pair of `BitVec(N)` | `BitVec(N)` | `bvxor` | `key ^ 0x1337` |

### Level 6 — shifts

| Operator | Type | Result | Backend | Example |
|---|---|---|---|---|
| `A << B` | pair of `BitVec(N)` | `BitVec(N)` | `bvshl` | `b << 2` |
| `A >> B` | pair of `BitVec(N)` | `BitVec(N)` | `bvlshr` | `w >> 8` |

There is no arithmetic (signed) shift; shifts are unsigned, like everything
else.

### Level 7 — additive

| Operator | Type | Result | Backend | Example |
|---|---|---|---|---|
| `A + B` | `Int`/`Real`/equal-width `BitVec` | same | `+` | `x + y == 30` |
| `A - B` | same | same | `-` | `p - q == 2` |

### Level 8 — multiplicative

| Operator | Type | Result | Backend | Example |
|---|---|---|---|---|
| `A * B` | `Int`/`Real`/equal-width `BitVec` | same | `*` | `3 * key0` |
| `A / B` | `Real`×`Real`, pair of `BitVec(N)` | same | `/` / `div` / `bvudiv` | `total / 4` |
| `A % B` | pair of `BitVec(N)` | `BitVec(N)` | `bvurem` | `crc % 0xFFFF` |

### Level 9 — unary (see §6)

Strongest binding: `!E`, `-E`, `~E`.

---

## 8. Precedence summary

```text
1   ||
2   &&
3   ==  !=
4   <   <=  >   >=
5   &   |   ^          <- binds tighter than comparison (unlike C!)
6   <<  >>
7   +   -
8   *   /   %
9   !   -   ~          (unary, strongest)
```

Ambiguity parsing examples:

| Source | Parses as |
|---|---|
| `x ^ 0x1337 == 0x4242` | `(x ^ 0x1337) == 0x4242` |
| `1 + 2 * 3 == 7` | `(1 + (2 * 3)) == 7` |
| `x & 1 << 2` | `x & (1 << 2)` |
| `-x + y < 0` | `((-x) + y) < 0` |

---

## 9. Integer literal adaptation to BitVec

A plain integer literal standing as an operand next to a `BitVec` is automatically widened to its width:

| Pattern | Effect | Example |
|---|---|---|
| `bitvec_expr OR INT_LITERAL` | the literal becomes `BitVec(W)` of the other operand | `(x ^ 0x1337) + 10 == 0x4242` (all literals → BitVec(32)) |
| range check | requires `0 ≤ v < 2^W`; violations are errors | `x < 100` — ok · `x < -1`, `b8 < 300` — rejected |

Without a BitVec neighbor the literal stays an `Int`. Computed subexpressions never adapt — direct literals only.

---

## 10. Typing rules

| # | Rule | Error message (verbatim) |
|---|---|---|
| 1 | Binary operation operands have one type | `Type mismatch: operator '…' requires …, got A and B` |
| 2 | Equality types: Bool/Int/Real/String/BitVec(same W); Array excluded | `Type mismatch: cannot compare A with B` |
| 3 | BitVec widths must match exactly | included in the messages above |
| 4 | `Int` and `Real` never mix implicitly | `Type mismatch: operator '+' requires numeric operands of the same sort, got Int and Real` |
| 5 | Only `==` / `!=` are available for strings | via the rule 1 family |
| 6 | The root of every `constraint` must be `Bool` | `Constraint must be a Boolean expression, got TYPE` |
| 7 | A variable must be declared somewhere in the problem | `Unknown variable "name"` |
| 8 | Variable names are unique within a problem | `Variable "x" is already defined` |

---

## 11. Application commands

### File operations

| Command | Shortcut | Format |
|---|---|---|
| New Problem | `Ctrl+N` | — |
| Open Project | `Ctrl+O` | JSON v1 |
| Save Project | `Ctrl+S` | JSON v1 |
| Save Project As | `Ctrl+Shift+S` | JSON v1 |
| Export ▸ SMT-LIB2 | menu | `.smt2` |
| Export ▸ JSON | menu | `.json` |
| Export ▸ TXT | menu | `.txt` |
| Import SMT-LIB2 | menu | `.smt2` |
| View SMT-LIB2 | `Ctrl+M` | popup |
| Quit | `Ctrl+Q` | — |

### Solver commands

| Command | Shortcut |
|---|---|
| Solve | `F5` / `Ctrl+Enter` |
| Stop | `Shift+F5` |

### Explorer commands

| Command |
|---|
| New |
| Duplicate |
| Rename |
| Delete |

---

## 12. Solver behavior

| Aspect | Behavior |
|---|---|
| Outcomes | `SAT` / `UNSAT` / `UNKNOWN` (+ internal `Error`) |
| Timeout | fixed 5000 ms per run in this build |
| Model | one assignment for ALL declared variables (completion enabled) |
| Statistics | collected from Z3 (conflicts, decisions, …) into the result |
| Threads | solving runs off the GUI thread; results arrive via an event queue |
| Cancellation | cooperative flag + backend timeout bound |

---

## 13. Project file format (`.z3w`)

JSON, schema version 1; sources are the single source of truth:

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
