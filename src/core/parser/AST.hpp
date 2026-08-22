#pragma once

#include "core/domain/Expression.hpp"
#include "core/domain/SourceLocation.hpp"
#include "core/domain/Variable.hpp"
#include "core/parser/Lexer.hpp"

#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace z3wb::ast {

// The statement layer of the DSL. Expressions are represented directly by the
// domain Expression tree (with unresolved variable refs carrying invalid ids):
// a second AST expression hierarchy would duplicate every node type without
// adding information, so the parser emits domain expressions immediately.
struct VarDecl
{
    std::string name;
    VariableType type = VariableType::Int;
    TypeParams params{};
    SourceLocation location{};
};

struct ConstraintDecl
{
    Expression expr; // variable references unresolved until semantic analysis
    SourceLocation location{};
};

using Statement = std::variant<VarDecl, ConstraintDecl>;

struct ParseOutput
{
    std::vector<Statement> statements;
    std::vector<Diagnostic> diagnostics;

    [[nodiscard]] bool hasErrors() const noexcept;
};

// Recursive descent with Pratt precedence climbing. On statement-level errors
// it synchronizes to the next 'var'/'constraint' keyword so several errors can
// be reported in one pass.
[[nodiscard]] ParseOutput parse(std::span<const Token> vecTokens);

} // namespace z3wb::ast
