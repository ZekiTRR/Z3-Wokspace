#include "core/solver/z3/Z3ExpressionConverter.hpp"

namespace z3wb {

z3::expr makeVariableExpr(z3::context& oContext, const Variable& oVariable)
{
    const char* szName = oVariable.name.c_str();
    switch (oVariable.type)
    {
        case VariableType::Bool:
            return oContext.bool_const(szName);
        case VariableType::Int:
            return oContext.int_const(szName);
        case VariableType::Real:
            return oContext.real_const(szName);
        case VariableType::BitVec:
            return oContext.bv_const(szName, oVariable.params.uBitVecWidth);
        case VariableType::String:
            return oContext.string_const(szName);
        case VariableType::Array:
            // Not reachable through the DSL yet; guarded by the caller.
            break;
    }
    return oContext.int_val(0);
}

Z3ExpressionConverter::Z3ExpressionConverter(z3::context& oContext, const Problem& oProblem,
    std::vector<Diagnostic>& vecDiags)
    : m_oContext(oContext)
    , m_oProblem(oProblem)
    , m_vecDiags(vecDiags)
{
}

void Z3ExpressionConverter::reportError(std::string strMessage)
{
    Diagnostic oDiagnostic;
    oDiagnostic.severity = DiagnosticSeverity::Error;
    oDiagnostic.message = std::move(strMessage);
    m_vecDiags.push_back(std::move(oDiagnostic));
}

std::optional<z3::expr> Z3ExpressionConverter::declareVariable(const VariableRef& oRef)
{
    const auto itFound = m_mapDeclared.find(oRef.id);
    if (itFound != m_mapDeclared.end())
    {
        return itFound->second;
    }

    const Variable* pVariable = m_oProblem.findVariable(oRef.id);
    if (pVariable == nullptr)
    {
        reportError("Constraint references unknown variable \"" + oRef.strName + "\"");
        return std::nullopt;
    }

    if (pVariable->type == VariableType::Array)
    {
        reportError("Sort 'Array' is not supported yet (variable \"" + pVariable->name + "\")");
        return std::nullopt;
    }

    const z3::expr oExpr = makeVariableExpr(m_oContext, *pVariable);
    m_mapDeclared.emplace(oRef.id, oExpr);
    return oExpr;
}

std::optional<z3::expr> Z3ExpressionConverter::convert(const Expression& oExpr)
{
    return oExpr.visit([this](const auto& oNode) -> std::optional<z3::expr>
    {
        using AltT = std::decay_t<decltype(oNode)>;

        if constexpr (std::is_same_v<AltT, Constant>)
        {
            switch (oNode.eKind)
            {
                case Constant::Kind::Bool:
                    return m_oContext.bool_val(std::get<bool>(oNode.oData));
                case Constant::Kind::Int:
                    return m_oContext.int_val(std::get<std::int64_t>(oNode.oData));
                case Constant::Kind::Real:
                    // Real literals stay strings all the way down: rationals
                    // must not pass through binary floating point.
                    return m_oContext.real_val(std::get<std::string>(oNode.oData).c_str());
                case Constant::Kind::String:
                    return m_oContext.string_val(std::get<std::string>(oNode.oData));
                case Constant::Kind::BitVec:
                {
                    const BitVecValue oValue = std::get<BitVecValue>(oNode.oData);
                    return m_oContext.bv_val(oValue.uBits, oValue.uWidth);
                }
            }
            reportError("Internal error: unknown constant kind");
            return std::nullopt;
        }
        else if constexpr (std::is_same_v<AltT, VariableRef>)
        {
            return declareVariable(oNode);
        }
        else if constexpr (std::is_same_v<AltT, UnaryExpression>)
        {
            const std::optional<z3::expr> oOperand = convert(*oNode.spOperand);
            if (!oOperand.has_value())
            {
                return std::nullopt;
            }

            switch (oNode.eOp)
            {
                case UnaryOp::Not:
                    return !(*oOperand);
                case UnaryOp::Neg:
                    return -*oOperand;
                case UnaryOp::BvNot:
                    return ~(*oOperand);
            }
            reportError("Internal error: unknown unary operator");
            return std::nullopt;
        }
        else if constexpr (std::is_same_v<AltT, BinaryExpression>)
        {
            const std::optional<z3::expr> oLhs = convert(*oNode.spLhs);
            if (!oLhs.has_value())
            {
                return std::nullopt;
            }
            const std::optional<z3::expr> oRhs = convert(*oNode.spRhs);
            if (!oRhs.has_value())
            {
                return std::nullopt;
            }

            const z3::expr oLeft = *oLhs;
            const z3::expr oRight = *oRhs;
            const bool bBv = oLeft.is_bv();

            // The semantic analyzer guarantees operand sorts are consistent,
            // so the BitVec/arith choice here only picks the right backend
            // function for the same source-level operator.
            switch (oNode.eOp)
            {
                case BinaryOp::And:
                    return oLeft && oRight;
                case BinaryOp::Or:
                    return oLeft || oRight;
                case BinaryOp::Eq:
                    return oLeft == oRight;
                case BinaryOp::Neq:
                    return oLeft != oRight;
                case BinaryOp::Lt:
                    return bBv ? z3::ult(oLeft, oRight) : oLeft < oRight;
                case BinaryOp::Le:
                    return bBv ? z3::ule(oLeft, oRight) : oLeft <= oRight;
                case BinaryOp::Gt:
                    return bBv ? z3::ugt(oLeft, oRight) : oLeft > oRight;
                case BinaryOp::Ge:
                    return bBv ? z3::uge(oLeft, oRight) : oLeft >= oRight;
                case BinaryOp::Add:
                    return oLeft + oRight;
                case BinaryOp::Sub:
                    return oLeft - oRight;
                case BinaryOp::Mul:
                    return oLeft * oRight;
                case BinaryOp::Div:
                    return bBv ? z3::udiv(oLeft, oRight) : oLeft / oRight;
                case BinaryOp::Rem:
                    return bBv ? z3::urem(oLeft, oRight) : oLeft % oRight;
                case BinaryOp::BvAnd:
                    return oLeft & oRight;
                case BinaryOp::BvOr:
                    return oLeft | oRight;
                case BinaryOp::BvXor:
                    return oLeft ^ oRight;
                case BinaryOp::BvShl:
                    return z3::shl(oLeft, oRight);
                case BinaryOp::BvShr:
                    return z3::lshr(oLeft, oRight);
            }
            reportError("Internal error: unknown binary operator");
            return std::nullopt;
        }
        else // FunctionCall
        {
            reportError("Function calls are not supported yet: \"" + oNode.strName + "\"");
            return std::nullopt;
        }
    });
}

} // namespace z3wb
