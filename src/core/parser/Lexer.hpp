#pragma once

#include "core/domain/SolverResult.hpp"
#include "core/parser/Token.hpp"

#include <string_view>
#include <vector>

namespace z3wb {

struct LexResult
{
    std::vector<Token> tokens;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool hasErrors() const noexcept;
};

// Converts source text into tokens. Recoverable problems (unknown character,
// unterminated string, malformed number) produce an error diagnostic and
// scanning continues, so the parser still sees a usable token stream.
[[nodiscard]] LexResult lex(std::string_view svSource);

} // namespace z3wb
