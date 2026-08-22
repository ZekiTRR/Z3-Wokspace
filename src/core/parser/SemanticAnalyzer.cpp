#include "core/parser/SemanticAnalyzer.hpp"

#include <limits>
#include <type_traits>

namespace z3wb {

namespace {

constexpr bool isNumeric(VariableType eType) noexcept
{
    return eType == VariableType::Int || eType == VariableType::Real
        || eType == VariableType::BitVec;
}

constexpr bool isArithmetic(VariableType eType) noexcept
{
    return isNumeric(eType);
}

} // namespace

SemanticAnalyzer::SemanticAnalyzer(const Problem& oProblem, std::vector<Diagnostic>& vecDiags)
    : m_oProblem(oProblem)
    , m_vecDiags(vecDiags)
{
}

Diagnostic SemanticAnalyzer::makeError(SourceLocation oLoc, std::string strMessage) const
{
    Diagnostic oDiagnostic;
    oDiagnostic.severity = DiagnosticSeverity::Error;
    oDiagnostic.location = std::move(oLoc);
    oDiagnostic.message = std::move(strMessage);
    return oDiagnostic;
}

void SemanticAnalyzer::reportError(SourceLocation oLoc, std::string strMessage)
{
    m_vecDiags.push_back(makeError(std::move(oLoc), std::move(strMessage)));
}

std::string SemanticAnalyzer::describeType(ExprTypeInfo oInfo)
{
    if (oInfo.eType == VariableType::BitVec)
    {
        return "BitVec(" + std::to_string(oInfo.uWidth) + ")";
    }
    return std::string(toString(oInfo.eType));
}

bool SemanticAnalyzer::alignBitVecConstants(ResolvedNode& oLhs, ResolvedNode& oRhs)
{
    auto coerceSide = [this](ResolvedNode& oSide, const ResolvedNode& oOther) -> bool
    {
        if (oOther.info.eType != VariableType::BitVec || oSide.info.eType != VariableType::Int)
        {
            return true;
        }

        // Only plain integer literals participate in coercion.
        const auto* pConstant = std::get_if<Constant>(&oSide.expr.node());
        if (pConstant == nullptr || pConstant->eKind != Constant::Kind::Int)
        {
            return true;
        }

        const std::int64_t iValue = std::get<std::int64_t>(pConstant->oData);
        const unsigned uWidth = oOther.info.uWidth;
        if (iValue < 0)
        {
            reportError(SourceLocation{}, "Negative integer literal cannot be used as BitVec("
                + std::to_string(uWidth) + ")");
            return false;
        }

        const std::uint64_t uMax = uWidth >= 64
            ? std::numeric_limits<std::uint64_t>::max()
            : ((1ull << uWidth) - 1);
        if (static_cast<std::uint64_t>(iValue) > uMax)
        {
            reportError(SourceLocation{}, "Integer literal " + std::to_string(iValue)
                + " does not fit into BitVec(" + std::to_string(uWidth) + ")");
            return false;
        }

        oSide.expr = Expression::bitVector(BitVecValue{uWidth, static_cast<std::uint64_t>(iValue)});
        oSide.info = oOther.info;
        return true;
    };

    // A literal between two bit-vector operands adapts to either side.
    if (!coerceSide(oLhs, oRhs))
    {
        return false;
    }
    return coerceSide(oRhs, oLhs);
}

std::optional<Expression> SemanticAnalyzer::resolve(const Expression& oUnresolved)
{
    const std::optional<ResolvedNode> oResolved = resolveNode(oUnresolved);
    if (!oResolved.has_value())
    {
        return std::nullopt;
    }

    if (oResolved->info.eType != VariableType::Bool)
    {
        reportError(SourceLocation{},
            "Constraint must be a Boolean expression, got " + describeType(oResolved->info));
        return std::nullopt;
    }

    return oResolved->expr;
}

std::optional<SemanticAnalyzer::ResolvedNode> SemanticAnalyzer::resolveNode(
    const Expression& oExpr)
{
    return oExpr.visit([this, &oExpr](const auto& oNode) -> std::optional<ResolvedNode>
    {
        using AltT = std::decay_t<decltype(oNode)>;

        if constexpr (std::is_same_v<AltT, Constant>)
        {
            ResolvedNode oResult;
            switch (oNode.eKind)
            {
                case Constant::Kind::Bool:
                    oResult.info.eType = VariableType::Bool;
                    break;
                case Constant::Kind::Int:
                    oResult.info.eType = VariableType::Int;
                    break;
                case Constant::Kind::Real:
                    oResult.info.eType = VariableType::Real;
                    break;
                case Constant::Kind::String:
                    oResult.info.eType = VariableType::String;
                    break;
                case Constant::Kind::BitVec:
                {
                    const BitVecValue oValue = std::get<BitVecValue>(oNode.oData);
                    oResult.info.eType = VariableType::BitVec;
                    oResult.info.uWidth = oValue.uWidth;
                    break;
                }
            }
            // Constants pass through unchanged; rebuild via the same node.
            oResult.expr = oExpr;
            return oResult;
        }
        else if constexpr (std::is_same_v<AltT, VariableRef>)
        {
            const Variable* pVariable = m_oProblem.findVariable(oNode.strName);
            if (pVariable == nullptr)
            {
                reportError(SourceLocation{}, "Unknown variable \"" + oNode.strName + "\"");
                return std::nullopt;
            }

            ResolvedNode oResult;
            oResult.expr = Expression::variable(pVariable->id, pVariable->name);
            oResult.info.eType = pVariable->type;
            oResult.info.uWidth = pVariable->params.uBitVecWidth;
            return oResult;
        }
        else if constexpr (std::is_same_v<AltT, UnaryExpression>)
        {
            const std::optional<ResolvedNode> oOperand = resolveNode(*oNode.spOperand);
            if (!oOperand.has_value())
            {
                return std::nullopt;
            }

            const ExprTypeInfo oInfo = oOperand->info;
            bool bValid = false;
            switch (oNode.eOp)
            {
                case UnaryOp::Not:
                    bValid = oInfo.eType == VariableType::Bool;
                    break;
                case UnaryOp::Neg:
                    bValid = oInfo.eType == VariableType::Int || oInfo.eType == VariableType::Real;
                    break;
                case UnaryOp::BvNot:
                    bValid = oInfo.eType == VariableType::BitVec;
                    break;
            }

            if (!bValid)
            {
                reportError(SourceLocation{}, "Operator '" + std::string(toString(oNode.eOp))
                    + "' cannot be applied to " + describeType(oInfo));
                return std::nullopt;
            }

            ResolvedNode oResult;
            oResult.expr = Expression::unary(oNode.eOp, oOperand->expr);
            oResult.info = oInfo; // Not -> Bool stays Bool; Neg/BvNot preserve operand sort
            if (oNode.eOp == UnaryOp::Not)
            {
                oResult.info.eType = VariableType::Bool;
            }
            return oResult;
        }
        else if constexpr (std::is_same_v<AltT, BinaryExpression>)
        {
            const std::optional<ResolvedNode> oLeft = resolveNode(*oNode.spLhs);
            if (!oLeft.has_value())
            {
                return std::nullopt;
            }
            const std::optional<ResolvedNode> oRight = resolveNode(*oNode.spRhs);
            if (!oRight.has_value())
            {
                return std::nullopt;
            }

            // Integer literals adapt to a bit-vector context (x ^ 0x1337).
            ResolvedNode oLeftValue = *oLeft;
            ResolvedNode oRightValue = *oRight;
            if (!alignBitVecConstants(oLeftValue, oRightValue))
            {
                return std::nullopt;
            }

            const ExprTypeInfo oLhsInfo = oLeftValue.info;
            const ExprTypeInfo oRhsInfo = oRightValue.info;
            const std::string strOperator(toString(oNode.eOp));

            // Helper lambdas keep each operator family compact.
            auto failMismatch = [&](const char* szRequirement) -> std::nullopt_t
            {
                reportError(SourceLocation{}, "Type mismatch: operator '" + strOperator
                    + "' requires " + szRequirement + ", got "
                    + describeType(oLhsInfo) + " and " + describeType(oRhsInfo));
                return std::nullopt;
            };

            ResolvedNode oResult;
            oResult.expr = Expression::binary(oNode.eOp, oLeftValue.expr, oRightValue.expr);

            switch (oNode.eOp)
            {
                case BinaryOp::And:
                case BinaryOp::Or:
                {
                    if (oLhsInfo.eType != VariableType::Bool || oRhsInfo.eType != VariableType::Bool)
                    {
                        return failMismatch("Boolean operands");
                    }
                    oResult.info.eType = VariableType::Bool;
                    return oResult;
                }

                case BinaryOp::Eq:
                case BinaryOp::Neq:
                {
                    const bool bComparable = oLhsInfo == oRhsInfo
                        && oLhsInfo.eType != VariableType::Array;
                    if (!bComparable)
                    {
                        reportError(SourceLocation{}, "Type mismatch: cannot compare "
                            + describeType(oLhsInfo) + " with " + describeType(oRhsInfo));
                        return std::nullopt;
                    }
                    oResult.info.eType = VariableType::Bool;
                    return oResult;
                }

                case BinaryOp::Lt:
                case BinaryOp::Le:
                case BinaryOp::Gt:
                case BinaryOp::Ge:
                {
                    if (!(isNumeric(oLhsInfo.eType) && oLhsInfo == oRhsInfo))
                    {
                        return failMismatch("two operands of the same ordered sort");
                    }
                    oResult.info.eType = VariableType::Bool;
                    return oResult;
                }

                case BinaryOp::Add:
                case BinaryOp::Sub:
                case BinaryOp::Mul:
                {
                    if (!(isArithmetic(oLhsInfo.eType) && oLhsInfo == oRhsInfo))
                    {
                        return failMismatch("numeric operands of the same sort");
                    }
                    oResult.info = oLhsInfo;
                    return oResult;
                }

                case BinaryOp::Div:
                {
                    const bool bOk = (oLhsInfo.eType == VariableType::Real && oRhsInfo.eType == VariableType::Real)
                        || (oLhsInfo.eType == VariableType::BitVec && oLhsInfo == oRhsInfo);
                    if (!bOk)
                    {
                        return failMismatch("Real or equal-width BitVec operands");
                    }
                    oResult.info = oLhsInfo;
                    return oResult;
                }

                case BinaryOp::Rem:
                {
                    if (!(oLhsInfo.eType == VariableType::BitVec && oLhsInfo == oRhsInfo))
                    {
                        return failMismatch("equal-width BitVec operands");
                    }
                    oResult.info = oLhsInfo;
                    return oResult;
                }

                case BinaryOp::BvAnd:
                case BinaryOp::BvOr:
                case BinaryOp::BvXor:
                case BinaryOp::BvShl:
                case BinaryOp::BvShr:
                {
                    if (!(oLhsInfo.eType == VariableType::BitVec && oLhsInfo == oRhsInfo))
                    {
                        return failMismatch("equal-width BitVec operands");
                    }
                    oResult.info = oLhsInfo;
                    return oResult;
                }
            }
            return std::nullopt;
        }
        else // FunctionCall
        {
            reportError(SourceLocation{}, "Unknown function \"" + oNode.strName + "\"");
            return std::nullopt;
        }
    });
}

} // namespace z3wb
