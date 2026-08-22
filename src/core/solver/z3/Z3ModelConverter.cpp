#include "core/solver/z3/Z3ModelConverter.hpp"
#include "core/solver/z3/Z3ExpressionConverter.hpp"

namespace z3wb {

namespace {

[[nodiscard]] std::optional<ModelEntry> evalVariable(z3::model& oModel,
    const Variable& oVariable, std::vector<Diagnostic>& vecDiags)
{
    z3::context& oContext = oModel.ctx();
    const z3::expr oConst = makeVariableExpr(oContext, oVariable);
    // model_completion=true: unconstrained variables still get a concrete
    // value so the Model Viewer always shows something useful.
    const z3::expr oValue = oModel.eval(oConst, true);

    ModelEntry oEntry;
    oEntry.name = oVariable.name;
    oEntry.type = oVariable.type;

    switch (oVariable.type)
    {
        case VariableType::Bool:
            oEntry.value = oValue.is_true();
            break;

        case VariableType::Int:
        {
            std::int64_t iValue = 0;
            if (oValue.is_numeral_i64(iValue))
            {
                oEntry.value = iValue;
            }
            else
            {
                // Values beyond int64 stay exact via their decimal string.
                oEntry.value = oValue.get_decimal_string(0);
            }
            break;
        }

        case VariableType::Real:
            oEntry.value = oValue.get_decimal_string(12);
            break;

        case VariableType::BitVec:
        {
            std::uint64_t uBits = 0;
            if (!oValue.is_numeral_u64(uBits) && oVariable.params.uBitVecWidth > 64)
            {
                Diagnostic oDiagnostic;
                oDiagnostic.severity = DiagnosticSeverity::Warning;
                oDiagnostic.message = "BitVec value of \"" + oVariable.name
                    + "\" exceeds 64 bits and is shown as a numeral string";
                vecDiags.push_back(std::move(oDiagnostic));
                oEntry.value = oValue.get_decimal_string(0);
            }
            else
            {
                oEntry.value = BitVecValue{oVariable.params.uBitVecWidth, uBits};
            }
            break;
        }

        case VariableType::String:
            oEntry.value = oValue.is_string_value() ? oValue.get_string() : std::string();
            break;

        case VariableType::Array:
            return std::nullopt;
    }

    return oEntry;
}

} // namespace

std::optional<Model> Z3ModelConverter::convert(z3::model& oModel,
    const Problem& oProblem, std::vector<Diagnostic>& vecDiags)
{
    Model oDomainModel;

    for (const Variable& oVariable : oProblem.variables())
    {
        if (oVariable.type == VariableType::Array)
        {
            continue; // unsupported sort; reported by the expression converter
        }

        std::optional<ModelEntry> oEntry = evalVariable(oModel, oVariable, vecDiags);
        if (oEntry.has_value())
        {
            oDomainModel.entries.push_back(std::move(*oEntry));
        }
    }

    return oDomainModel;
}

} // namespace z3wb
