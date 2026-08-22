#include "core/domain/Expression.hpp"

#include <cstdio>
#include <type_traits>

namespace z3wb {

namespace {

std::string escapeString(std::string_view svText)
{
    std::string strOut;
    strOut.reserve(svText.size() + 2);
    for (const char ch : svText)
    {
        switch (ch)
        {
            case '\\':
                strOut += "\\\\";
                break;
            case '"':
                strOut += "\\\"";
                break;
            case '\n':
                strOut += "\\n";
                break;
            case '\t':
                strOut += "\\t";
                break;
            default:
                strOut += ch;
                break;
        }
    }
    return strOut;
}

std::string constantToString(const Constant& oConstant)
{
    switch (oConstant.eKind)
    {
        case Constant::Kind::Bool:
            return std::get<bool>(oConstant.oData) ? "true" : "false";
        case Constant::Kind::Int:
            return std::to_string(std::get<std::int64_t>(oConstant.oData));
        case Constant::Kind::Real:
            return std::get<std::string>(oConstant.oData);
        case Constant::Kind::String:
            return "\"" + escapeString(std::get<std::string>(oConstant.oData)) + "\"";
        case Constant::Kind::BitVec:
        {
            // Hex with explicit width, e.g. 0x1337:32 — the width suffix makes
            // clear this is not a plain Int literal.
            char szBuf[32];
            const BitVecValue oValue = std::get<BitVecValue>(oConstant.oData);
            std::snprintf(szBuf, sizeof(szBuf), "0x%llX:%u",
                static_cast<unsigned long long>(oValue.uBits), oValue.uWidth);
            return szBuf;
        }
    }
    return "<?>";
}

} // namespace

std::string_view toString(UnaryOp eOp)
{
    switch (eOp)
    {
        case UnaryOp::Not:
            return "!";
        case UnaryOp::Neg:
            return "-";
        case UnaryOp::BvNot:
            return "~";
    }
    return "<?>";
}

std::string_view toString(BinaryOp eOp)
{
    switch (eOp)
    {
        case BinaryOp::And:
            return "&&";
        case BinaryOp::Or:
            return "||";
        case BinaryOp::Eq:
            return "==";
        case BinaryOp::Neq:
            return "!=";
        case BinaryOp::Lt:
            return "<";
        case BinaryOp::Le:
            return "<=";
        case BinaryOp::Gt:
            return ">";
        case BinaryOp::Ge:
            return ">=";
        case BinaryOp::Add:
            return "+";
        case BinaryOp::Sub:
            return "-";
        case BinaryOp::Mul:
            return "*";
        case BinaryOp::Div:
            return "/";
        case BinaryOp::Rem:
            return "%";
        case BinaryOp::BvAnd:
            return "&";
        case BinaryOp::BvOr:
            return "|";
        case BinaryOp::BvXor:
            return "^";
        case BinaryOp::BvShl:
            return "<<";
        case BinaryOp::BvShr:
            return ">>";
    }
    return "<?>";
}

Expression::Expression()
    : m_node(Constant{})
{
}

Expression Expression::boolean(bool bValue)
{
    return Expression(Constant{Constant::Kind::Bool,
        std::variant<bool, std::int64_t, std::string, BitVecValue>{bValue}});
}

Expression Expression::integer(std::int64_t iValue)
{
    return Expression(Constant{Constant::Kind::Int,
        std::variant<bool, std::int64_t, std::string, BitVecValue>{iValue}});
}

Expression Expression::real(std::string strDecimalLiteral)
{
    return Expression(Constant{Constant::Kind::Real,
        std::variant<bool, std::int64_t, std::string, BitVecValue>{std::move(strDecimalLiteral)}});
}

Expression Expression::stringValue(std::string strValue)
{
    return Expression(Constant{Constant::Kind::String,
        std::variant<bool, std::int64_t, std::string, BitVecValue>{std::move(strValue)}});
}

Expression Expression::bitVector(BitVecValue oValue)
{
    return Expression(Constant{Constant::Kind::BitVec,
        std::variant<bool, std::int64_t, std::string, BitVecValue>{oValue}});
}

Expression Expression::variable(VariableId oId, std::string strName)
{
    return Expression(VariableRef{oId, std::move(strName)});
}

Expression Expression::unary(UnaryOp eOp, Expression oOperand)
{
    UnaryExpression oNode;
    oNode.eOp = eOp;
    oNode.spOperand = std::make_shared<const Expression>(std::move(oOperand));
    return Expression(Node{std::move(oNode)});
}

Expression Expression::binary(BinaryOp eOp, Expression oLhs, Expression oRhs)
{
    BinaryExpression oNode;
    oNode.eOp = eOp;
    oNode.spLhs = std::make_shared<const Expression>(std::move(oLhs));
    oNode.spRhs = std::make_shared<const Expression>(std::move(oRhs));
    return Expression(Node{std::move(oNode)});
}

Expression Expression::call(std::string strName, std::vector<Expression> vecArgs)
{
    FunctionCall oNode;
    oNode.strName = std::move(strName);
    oNode.vecArgs = std::move(vecArgs);
    return Expression(Node{std::move(oNode)});
}

bool operator==(const Expression& oLhs, const Expression& oRhs) noexcept
{
    if (oLhs.node().index() != oRhs.node().index())
    {
        return false;
    }

    const bool bEqual = oLhs.visit([&oRhs](const auto& oLeft) -> bool
    {
        using AltT = std::decay_t<decltype(oLeft)>;
        const AltT* pRight = std::get_if<AltT>(&oRhs.node());
        if (pRight == nullptr)
        {
            return false;
        }

        if constexpr (std::is_same_v<AltT, Constant>)
        {
            return oLeft.eKind == pRight->eKind && oLeft.oData == pRight->oData;
        }
        else if constexpr (std::is_same_v<AltT, VariableRef>)
        {
            return oLeft.id == pRight->id && oLeft.strName == pRight->strName;
        }
        else if constexpr (std::is_same_v<AltT, UnaryExpression>)
        {
            return oLeft.eOp == pRight->eOp && *oLeft.spOperand == *pRight->spOperand;
        }
        else if constexpr (std::is_same_v<AltT, BinaryExpression>)
        {
            return oLeft.eOp == pRight->eOp
                && *oLeft.spLhs == *pRight->spLhs
                && *oLeft.spRhs == *pRight->spRhs;
        }
        else // FunctionCall
        {
            return oLeft.strName == pRight->strName && oLeft.vecArgs == pRight->vecArgs;
        }
    });

    return bEqual;
}

bool operator!=(const Expression& oLhs, const Expression& oRhs) noexcept
{
    return !(oLhs == oRhs);
}

std::string toString(const Expression& oExpr)
{
    return oExpr.visit([](const auto& oNode) -> std::string
    {
        using AltT = std::decay_t<decltype(oNode)>;

        if constexpr (std::is_same_v<AltT, Constant>)
        {
            return constantToString(oNode);
        }
        else if constexpr (std::is_same_v<AltT, VariableRef>)
        {
            return oNode.strName;
        }
        else if constexpr (std::is_same_v<AltT, UnaryExpression>)
        {
            return "(" + std::string(toString(oNode.eOp)) + toString(*oNode.spOperand) + ")";
        }
        else if constexpr (std::is_same_v<AltT, BinaryExpression>)
        {
            return "(" + toString(*oNode.spLhs) + " " + std::string(toString(oNode.eOp))
                + " " + toString(*oNode.spRhs) + ")";
        }
        else // FunctionCall
        {
            std::string strOut = oNode.strName;
            strOut += "(";
            for (std::size_t stIndex = 0; stIndex < oNode.vecArgs.size(); ++stIndex)
            {
                if (stIndex != 0)
                {
                    strOut += ", ";
                }
                strOut += toString(oNode.vecArgs[stIndex]);
            }
            strOut += ")";
            return strOut;
        }
    });
}

Expression remapVariables(const Expression& oExpr, const VariableIdMap& mapOldToNew)
{
    return oExpr.visit([&mapOldToNew](const auto& oNode) -> Expression
    {
        using AltT = std::decay_t<decltype(oNode)>;

        if constexpr (std::is_same_v<AltT, VariableRef>)
        {
            const auto itFound = mapOldToNew.find(oNode.id);
            if (itFound != mapOldToNew.end())
            {
                return Expression::variable(itFound->second, oNode.strName);
            }
            return Expression::variable(oNode.id, oNode.strName);
        }
        else if constexpr (std::is_same_v<AltT, UnaryExpression>)
        {
            return Expression::unary(oNode.eOp, remapVariables(*oNode.spOperand, mapOldToNew));
        }
        else if constexpr (std::is_same_v<AltT, BinaryExpression>)
        {
            return Expression::binary(oNode.eOp,
                remapVariables(*oNode.spLhs, mapOldToNew),
                remapVariables(*oNode.spRhs, mapOldToNew));
        }
        else if constexpr (std::is_same_v<AltT, FunctionCall>)
        {
            std::vector<Expression> vecRemapped;
            vecRemapped.reserve(oNode.vecArgs.size());
            for (const Expression& oArg : oNode.vecArgs)
            {
                vecRemapped.push_back(remapVariables(oArg, mapOldToNew));
            }
            return Expression::call(oNode.strName, std::move(vecRemapped));
        }
        else // Constant
        {
            // Rebuild through factories: the Node constructor is private on
            // purpose, keeping every creation path explicit.
            switch (oNode.eKind)
            {
                case Constant::Kind::Bool:
                    return Expression::boolean(std::get<bool>(oNode.oData));
                case Constant::Kind::Int:
                    return Expression::integer(std::get<std::int64_t>(oNode.oData));
                case Constant::Kind::Real:
                    return Expression::real(std::get<std::string>(oNode.oData));
                case Constant::Kind::String:
                    return Expression::stringValue(std::get<std::string>(oNode.oData));
                case Constant::Kind::BitVec:
                    return Expression::bitVector(std::get<BitVecValue>(oNode.oData));
            }
            return Expression{};
        }
    });
}

} // namespace z3wb
