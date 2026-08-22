#include "core/parser/DslPrinter.hpp"

#include <type_traits>

namespace z3wb {

namespace {

std::string printConstant(const Constant& oConstant)
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
        {
            std::string strOut = "\"";
            for (const char ch : std::get<std::string>(oConstant.oData))
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
            strOut += "\"";
            return strOut;
        }
        case Constant::Kind::BitVec:
        {
            char szBuf[32];
            const BitVecValue oValue = std::get<BitVecValue>(oConstant.oData);
            std::snprintf(szBuf, sizeof(szBuf), "0x%llX",
                static_cast<unsigned long long>(oValue.uBits));
            return szBuf;
        }
    }
    return "?";
}

std::string printExpr(const Expression& oExpr)
{
    return oExpr.visit([](const auto& oNode) -> std::string
    {
        using AltT = std::decay_t<decltype(oNode)>;

        if constexpr (std::is_same_v<AltT, Constant>)
        {
            return printConstant(oNode);
        }
        else if constexpr (std::is_same_v<AltT, VariableRef>)
        {
            return oNode.strName;
        }
        else if constexpr (std::is_same_v<AltT, UnaryExpression>)
        {
            return "(" + std::string(toString(oNode.eOp))
                + printExpr(*oNode.spOperand) + ")";
        }
        else if constexpr (std::is_same_v<AltT, BinaryExpression>)
        {
            return "(" + printExpr(*oNode.spLhs) + " "
                + std::string(toString(oNode.eOp)) + " "
                + printExpr(*oNode.spRhs) + ")";
        }
        else // FunctionCall: not produced by the DSL yet
        {
            return "?";
        }
    });
}

} // namespace

std::string DslPrinter::printExpression(const Expression& oExpr)
{
    return printExpr(oExpr);
}

std::string DslPrinter::printProblem(const Problem& oProblem)
{
    std::string strOut;

    for (const Variable& oVariable : oProblem.variables())
    {
        strOut += "var " + oVariable.name + ": ";
        switch (oVariable.type)
        {
            case VariableType::Bool:
                strOut += "Bool";
                break;
            case VariableType::Int:
                strOut += "Int";
                break;
            case VariableType::Real:
                strOut += "Real";
                break;
            case VariableType::String:
                strOut += "String";
                break;
            case VariableType::BitVec:
                strOut += "BitVec(" + std::to_string(oVariable.params.uBitVecWidth) + ")";
                break;
            case VariableType::Array:
                strOut += "Int"; // unsupported sort cannot reach here
                break;
        }
        strOut += "\n";
    }

    if (!oProblem.variables().empty())
    {
        strOut += "\n";
    }

    for (const Constraint& oConstraint : oProblem.constraints())
    {
        if (!oConstraint.enabled)
        {
            continue; // disabled constraints have no DSL representation yet
        }
        strOut += "constraint " + printExpr(oConstraint.expr) + "\n";
    }

    return strOut;
}

} // namespace z3wb
