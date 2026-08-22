#include "core/serialization/SmtLib2Serializer.hpp"

#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace z3wb {

namespace {

// Sort inference mirrors the semantic analyzer's rules; the resolved problem
// is guaranteed to be well-typed, so a single bottom-up pass is sufficient.
class Serializer
{
public:
    explicit Serializer(const Problem& oProblem)
    {
        for (const Variable& oVariable : oProblem.variables())
        {
            m_mapBitVecVars[oVariable.name] = oVariable.type == VariableType::BitVec;
            m_mapRealVars[oVariable.name] = oVariable.type == VariableType::Real;
        }
    }

    [[nodiscard]] std::string serializeProblem(const Problem& oProblem) const
    {
        std::string strOut;

        for (const Variable& oVariable : oProblem.variables())
        {
            strOut += "(declare-const " + oVariable.name + " "
                + serializeSort(oVariable) + ")\n";
        }

        for (const Constraint& oConstraint : oProblem.constraints())
        {
            if (!oConstraint.enabled)
            {
                continue;
            }
            strOut += "(assert " + serializeExpr(oConstraint.expr) + ")\n";
        }

        strOut += "(check-sat)\n(get-model)\n";
        return strOut;
    }

private:
    [[nodiscard]] static std::string serializeSort(const Variable& oVariable)
    {
        switch (oVariable.type)
        {
            case VariableType::Bool:
                return "Bool";
            case VariableType::Int:
                return "Int";
            case VariableType::Real:
                return "Real";
            case VariableType::String:
                return "String";
            case VariableType::BitVec:
                return "(_ BitVec " + std::to_string(oVariable.params.uBitVecWidth) + ")";
            case VariableType::Array:
                break;
        }
        return "Int";
    }

    // Bottom-up BitVec detection. Constants carry their kind; variables are
    // looked up by name; operators propagate from the BitVec-only set.
    [[nodiscard]] bool isBitVec(const Expression& oExpr) const
    {
        return oExpr.visit([this](const auto& oNode) -> bool
        {
            using AltT = std::decay_t<decltype(oNode)>;

            if constexpr (std::is_same_v<AltT, Constant>)
            {
                return oNode.eKind == Constant::Kind::BitVec;
            }
            else if constexpr (std::is_same_v<AltT, VariableRef>)
            {
                const auto itFound = m_mapBitVecVars.find(oNode.strName);
                return itFound != m_mapBitVecVars.end() && itFound->second;
            }
            else if constexpr (std::is_same_v<AltT, UnaryExpression>)
            {
                return oNode.eOp == UnaryOp::BvNot || isBitVec(*oNode.spOperand);
            }
            else if constexpr (std::is_same_v<AltT, BinaryExpression>)
            {
                switch (oNode.eOp)
                {
                    case BinaryOp::BvAnd:
                    case BinaryOp::BvOr:
                    case BinaryOp::BvXor:
                    case BinaryOp::BvShl:
                    case BinaryOp::BvShr:
                        return true;
                    default:
                        return isBitVec(*oNode.spLhs);
                }
            }
            else
            {
                return false;
            }
        });
    }

    [[nodiscard]] bool isReal(const Expression& oExpr) const
    {
        return oExpr.visit([this](const auto& oNode) -> bool
        {
            using AltT = std::decay_t<decltype(oNode)>;

            if constexpr (std::is_same_v<AltT, Constant>)
            {
                return oNode.eKind == Constant::Kind::Real;
            }
            else if constexpr (std::is_same_v<AltT, VariableRef>)
            {
                const auto itFound = m_mapRealVars.find(oNode.strName);
                return itFound != m_mapRealVars.end() && itFound->second;
            }
            else if constexpr (std::is_same_v<AltT, UnaryExpression>)
            {
                return oNode.eOp == UnaryOp::Neg && isReal(*oNode.spOperand);
            }
            else if constexpr (std::is_same_v<AltT, BinaryExpression>)
            {
                return isReal(*oNode.spLhs);
            }
            else
            {
                return false;
            }
        });
    }

    [[nodiscard]] static std::string serializeConstant(const Constant& oConstant)
    {
        switch (oConstant.eKind)
        {
            case Constant::Kind::Bool:
                return std::get<bool>(oConstant.oData) ? "true" : "false";

            case Constant::Kind::Int:
                return std::to_string(std::get<std::int64_t>(oConstant.oData));

            case Constant::Kind::Real:
            {
                // SMT-LIB2 reals require a decimal or rational shape.
                std::string strValue = std::get<std::string>(oConstant.oData);
                if (strValue.find('.') == std::string::npos
                    && strValue.find('/') == std::string::npos)
                {
                    strValue += ".0";
                }
                return strValue;
            }

            case Constant::Kind::String:
            {
                // SMT-LIB2 escapes quotes by doubling them.
                std::string strOut = "\"";
                for (const char ch : std::get<std::string>(oConstant.oData))
                {
                    strOut += ch == '"' ? "\"\"" : std::string(1, ch);
                }
                strOut += "\"";
                return strOut;
            }

            case Constant::Kind::BitVec:
            {
                // (_ bvN W) works for every width, unlike #x hex literals.
                const BitVecValue oValue = std::get<BitVecValue>(oConstant.oData);
                return "(_ bv" + std::to_string(oValue.uBits) + " "
                    + std::to_string(oValue.uWidth) + ")";
            }
        }
        return "?";
    }

    [[nodiscard]] std::string serializeExpr(const Expression& oExpr) const
    {
        return oExpr.visit([this](const auto& oNode) -> std::string
        {
            using AltT = std::decay_t<decltype(oNode)>;

            if constexpr (std::is_same_v<AltT, Constant>)
            {
                return serializeConstant(oNode);
            }
            else if constexpr (std::is_same_v<AltT, VariableRef>)
            {
                return oNode.strName;
            }
            else if constexpr (std::is_same_v<AltT, UnaryExpression>)
            {
                const std::string strOperand = serializeExpr(*oNode.spOperand);
                switch (oNode.eOp)
                {
                    case UnaryOp::Not:
                        return "(not " + strOperand + ")";
                    case UnaryOp::Neg:
                        return isBitVec(*oNode.spOperand)
                            ? "(bvneg " + strOperand + ")"
                            : "(- " + strOperand + ")";
                    case UnaryOp::BvNot:
                        return "(bvnot " + strOperand + ")";
                }
                return "?";
            }
            else if constexpr (std::is_same_v<AltT, BinaryExpression>)
            {
                const std::string strLeft = serializeExpr(*oNode.spLhs);
                const std::string strRight = serializeExpr(*oNode.spRhs);
                const bool bBv = isBitVec(*oNode.spLhs);

                switch (oNode.eOp)
                {
                    case BinaryOp::And:
                        return "(and " + strLeft + " " + strRight + ")";
                    case BinaryOp::Or:
                        return "(or " + strLeft + " " + strRight + ")";
                    case BinaryOp::Eq:
                        return "(= " + strLeft + " " + strRight + ")";
                    case BinaryOp::Neq:
                        return "(distinct " + strLeft + " " + strRight + ")";

                    case BinaryOp::Lt:
                        return bBv ? "(bvult " + strLeft + " " + strRight + ")"
                            : "(< " + strLeft + " " + strRight + ")";
                    case BinaryOp::Le:
                        return bBv ? "(bvule " + strLeft + " " + strRight + ")"
                            : "(<= " + strLeft + " " + strRight + ")";
                    case BinaryOp::Gt:
                        return bBv ? "(bvugt " + strLeft + " " + strRight + ")"
                            : "(> " + strLeft + " " + strRight + ")";
                    case BinaryOp::Ge:
                        return bBv ? "(bvuge " + strLeft + " " + strRight + ")"
                            : "(>= " + strLeft + " " + strRight + ")";

                    case BinaryOp::Add:
                        return "(+ " + strLeft + " " + strRight + ")";
                    case BinaryOp::Sub:
                        return "(- " + strLeft + " " + strRight + ")";
                    case BinaryOp::Mul:
                        return "(* " + strLeft + " " + strRight + ")";

                    case BinaryOp::Div:
                        if (bBv)
                        {
                            return "(bvudiv " + strLeft + " " + strRight + ")";
                        }
                        // Int division in SMT-LIB2 is div; Real division is /.
                        return isReal(*oNode.spLhs)
                            ? "(/ " + strLeft + " " + strRight + ")"
                            : "(div " + strLeft + " " + strRight + ")";

                    case BinaryOp::Rem:
                        return bBv ? "(bvurem " + strLeft + " " + strRight + ")"
                            : "(mod " + strLeft + " " + strRight + ")";

                    case BinaryOp::BvAnd:
                        return "(bvand " + strLeft + " " + strRight + ")";
                    case BinaryOp::BvOr:
                        return "(bvor " + strLeft + " " + strRight + ")";
                    case BinaryOp::BvXor:
                        return "(bvxor " + strLeft + " " + strRight + ")";
                    case BinaryOp::BvShl:
                        return "(bvshl " + strLeft + " " + strRight + ")";
                    case BinaryOp::BvShr:
                        return "(bvlshr " + strLeft + " " + strRight + ")";
                }
                return "?";
            }
            else // FunctionCall: not produced by the DSL yet
            {
                return "?";
            }
        });
    }

    std::unordered_map<std::string, bool> m_mapBitVecVars;
    std::unordered_map<std::string, bool> m_mapRealVars;
};

} // namespace

std::string SmtLib2Serializer::serialize(const Problem& oProblem)
{
    const Serializer oSerializer(oProblem);
    return oSerializer.serializeProblem(oProblem);
}

} // namespace z3wb
