#pragma once

#include "core/domain/Problem.hpp"

#include <string>

namespace z3wb {

// Generates workbench DSL source from a parsed problem. Used by the
// SMT-LIB2 import pipeline so imported problems become first-class citizens:
// they get editable DSL text, persist to .z3w, and re-parse like any other.
//
// BitVec constants are printed as hex without a width suffix; the semantic
// analyzer restores the width from the surrounding operands.
class DslPrinter
{
public:
    // var declarations + blank line + one constraint per line.
    [[nodiscard]] static std::string printProblem(const Problem& oProblem);

    // Single expression body (used for tests and future features).
    [[nodiscard]] static std::string printExpression(const Expression& oExpr);
};

} // namespace z3wb
