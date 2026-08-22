#pragma once

#include "core/domain/Expression.hpp"
#include "core/domain/Problem.hpp"
#include "core/domain/SolverResult.hpp"
#include "core/domain/Variable.hpp"

#include <optional>
#include <string>
#include <unordered_map>

#include <z3++.h>

namespace z3wb {

// Builds a Z3 constant expression for a domain variable. Shared by the
// expression converter (declarations) and the model converter (evaluation).
[[nodiscard]] z3::expr makeVariableExpr(z3::context& oContext, const Variable& oVariable);

// Converts an immutable domain expression tree into a Z3 AST. Variable
// references are resolved by id against the problem; unknown ids and
// unsupported sorts are reported as error diagnostics.
class Z3ExpressionConverter
{
public:
    Z3ExpressionConverter(z3::context& oContext, const Problem& oProblem,
        std::vector<Diagnostic>& vecDiags);

    [[nodiscard]] std::optional<z3::expr> convert(const Expression& oExpr);

private:
    [[nodiscard]] std::optional<z3::expr> declareVariable(const VariableRef& oRef);
    void reportError(std::string strMessage);

    z3::context& m_oContext;
    const Problem& m_oProblem;
    std::vector<Diagnostic>& m_vecDiags;
    std::unordered_map<VariableId, z3::expr> m_mapDeclared;
};

} // namespace z3wb
