#pragma once

#include "core/domain/Expression.hpp"
#include "core/domain/Problem.hpp"
#include "core/domain/SolverResult.hpp"

#include <optional>
#include <vector>

namespace z3wb {

// Resolves variable references against the problem's declared variables and
// enforces sort correctness. Produces a fully typed expression tree whose
// variable ids point at real Problem entities — ready for the Z3 adapter.
class SemanticAnalyzer
{
public:
    SemanticAnalyzer(const Problem& oProblem, std::vector<Diagnostic>& vecDiags);

    // Returns nullopt and appends error diagnostics when the expression
    // contains unknown variables or type errors.
    [[nodiscard]] std::optional<Expression> resolve(const Expression& oUnresolved);

private:
    struct ExprTypeInfo
    {
        VariableType eType = VariableType::Int;
        unsigned uWidth = 0; // BitVec only

        friend bool operator==(const ExprTypeInfo& oLhs, const ExprTypeInfo& oRhs) noexcept = default;
    };

    struct ResolvedNode
    {
        Expression expr;
        ExprTypeInfo info{};
    };

    [[nodiscard]] std::optional<ResolvedNode> resolveNode(const Expression& oExpr);

    // Adapts plain integer literals to a BitVec context (e.g. x ^ 0x1337 with
    // x: BitVec(32)); reports an error when the value does not fit the width.
    [[nodiscard]] bool alignBitVecConstants(ResolvedNode& oLhs, ResolvedNode& oRhs);

    void reportError(SourceLocation oLoc, std::string strMessage);
    [[nodiscard]] static std::string describeType(ExprTypeInfo oInfo);
    [[nodiscard]] Diagnostic makeError(SourceLocation oLoc, std::string strMessage) const;

    const Problem& m_oProblem;
    std::vector<Diagnostic>& m_vecDiags;
};

} // namespace z3wb
