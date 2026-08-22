#include "core/solver/z3/Z3Solver.hpp"

#include "core/solver/z3/Z3ExpressionConverter.hpp"
#include "core/solver/z3/Z3ModelConverter.hpp"

#include <chrono>
#include <utility>

namespace z3wb {

namespace {

Diagnostic makeDiagnostic(DiagnosticSeverity eSeverity, std::string strMessage)
{
    Diagnostic oDiagnostic;
    oDiagnostic.severity = eSeverity;
    oDiagnostic.message = std::move(strMessage);
    return oDiagnostic;
}

void collectStatistics(z3::solver& oBackend, SolverStatistics& oStats)
{
    const z3::stats oZ3Stats = oBackend.statistics();
    for (unsigned uIndex = 0; uIndex < oZ3Stats.size(); ++uIndex)
    {
        std::string strValue;
        if (oZ3Stats.is_uint(uIndex))
        {
            strValue = std::to_string(oZ3Stats.uint_value(uIndex));
        }
        else if (oZ3Stats.is_double(uIndex))
        {
            strValue = std::to_string(oZ3Stats.double_value(uIndex));
        }
        else
        {
            continue;
        }
        oStats.entries.emplace_back(oZ3Stats.key(uIndex), std::move(strValue));
    }
}

} // namespace

std::string_view Z3Solver::name() const
{
    return "Z3";
}

std::optional<z3::solver> Z3Solver::buildBackend(z3::context& oContext,
    const Problem& oProblem, const SolverConfig& oConfig,
    std::vector<Diagnostic>& vecDiags)
{
    z3::solver oBackend(oContext);

    z3::params oParams(oContext);
    // :timeout is milliseconds; the backend enforces the budget even though
    // cooperative cancellation cannot interrupt a single check() call.
    oParams.set(":timeout", static_cast<unsigned>(oConfig.timeout.count()));
    oParams.set(":random-seed", static_cast<unsigned>(oConfig.uRandomSeed));
    oParams.set(":model", oConfig.bProduceModel);
    oBackend.set(oParams);

    Z3ExpressionConverter oConverter(oContext, oProblem, vecDiags);

    for (const Variable& oVariable : oProblem.variables())
    {
        if (oVariable.type == VariableType::Array)
        {
            Diagnostic oDiagnostic = makeDiagnostic(DiagnosticSeverity::Error,
                "Sort 'Array' is not supported yet (variable \"" + oVariable.name + "\")");
            vecDiags.push_back(std::move(oDiagnostic));
            return std::nullopt;
        }
    }

    for (const Constraint& oConstraint : oProblem.constraints())
    {
        if (!oConstraint.enabled)
        {
            continue;
        }

        const std::optional<z3::expr> oAssertion = oConverter.convert(oConstraint.expr);
        if (!oAssertion.has_value())
        {
            return std::nullopt;
        }
        oBackend.add(*oAssertion);
    }

    return oBackend;
}

SolverResult Z3Solver::solve(const Problem& oProblem, const SolverConfig& oConfig,
    const std::shared_ptr<ICancellation>& spCancellation) const
{
    using clock = std::chrono::steady_clock;
    const auto tStart = clock::now();

    if (spCancellation != nullptr && spCancellation->isCancelled())
    {
        SolverResult oResult;
        oResult.status = SolverStatus::Unknown;
        oResult.diagnostics.push_back(
            makeDiagnostic(DiagnosticSeverity::Warning, "Solving was cancelled before it started"));
        oResult.solveTime = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - tStart);
        return oResult;
    }

    z3::context oContext;
    std::vector<Diagnostic> vecDiagnostics;

    std::optional<z3::solver> oBackend = buildBackend(oContext, oProblem, oConfig, vecDiagnostics);
    if (!oBackend.has_value())
    {
        SolverResult oResult;
        oResult.status = SolverStatus::Error;
        oResult.diagnostics = std::move(vecDiagnostics);
        oResult.solveTime = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - tStart);
        return oResult;
    }

    const z3::check_result eCheck = oBackend->check();

    SolverResult oResult;

    switch (eCheck)
    {
        case z3::sat:
            oResult.status = SolverStatus::Sat;
            if (oConfig.bProduceModel)
            {
                z3::model oModel = oBackend->get_model();
                std::optional<Model> oDomainModel =
                    Z3ModelConverter::convert(oModel, oProblem, vecDiagnostics);
                if (oDomainModel.has_value())
                {
                    oResult.model = std::move(*oDomainModel);
                }
                else
                {
                    oResult.status = SolverStatus::Error;
                }
            }
            break;

        case z3::unsat:
            oResult.status = SolverStatus::Unsat;
            break;

        case z3::unknown:
        {
            oResult.status = SolverStatus::Unknown;
            std::string strReason = oBackend->reason_unknown();
            if (strReason.empty())
            {
                strReason = "no reason provided";
            }
            oResult.diagnostics.push_back(makeDiagnostic(DiagnosticSeverity::Info,
                "Reason: " + strReason));
            break;
        }
    }

    collectStatistics(oBackend.value(), oResult.statistics);

    if (spCancellation != nullptr && spCancellation->isCancelled()
        && oResult.status == SolverStatus::Sat)
    {
        // The answer arrived but the user gave up meanwhile: keep it, the
        // GUI decides whether to display it.
        oResult.diagnostics.push_back(
            makeDiagnostic(DiagnosticSeverity::Warning, "Solving finished after cancellation was requested"));
    }

    oResult.solveTime = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - tStart);
    oResult.diagnostics.insert(oResult.diagnostics.end(),
        std::make_move_iterator(vecDiagnostics.begin()),
        std::make_move_iterator(vecDiagnostics.end()));
    return oResult;
}

std::string Z3Solver::toSmtLib2(const Problem& oProblem, const SolverConfig& oConfig) const
{
    z3::context oContext;
    std::vector<Diagnostic> vecIgnored;
    const std::optional<z3::solver> oBackend = buildBackend(oContext, oProblem, oConfig, vecIgnored);
    if (!oBackend.has_value())
    {
        return "; failed to render problem as SMT-LIB2\n(check-sat)\n";
    }

    // Z3_solver_to_string renders declarations and assertions in SMT-LIB2
    // syntax; the trailing commands make the text directly runnable.
    std::string strOut(Z3_solver_to_string(oContext, *oBackend));
    strOut += "(check-sat)\n(get-model)\n";
    return strOut;
}

} // namespace z3wb

