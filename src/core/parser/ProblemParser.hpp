#pragma once

#include "core/domain/Problem.hpp"
#include "core/parser/AST.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace z3wb {

struct ParseResult
{
    // Present only when the source parsed and validated without errors.
    std::optional<Problem> problem;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool ok() const noexcept { return problem.has_value(); }
};

// Full pipeline: lex -> parse -> semantic analysis -> domain Problem.
// Variable declarations may appear anywhere in the source; constraints are
// resolved against all declared variables.
[[nodiscard]] ParseResult parseProblem(std::string_view svSource, std::string strName);

} // namespace z3wb
