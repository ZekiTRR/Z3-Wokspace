#include "core/domain/SolverResult.hpp"

#include <algorithm>

namespace z3wb {

const ModelEntry* Model::find(std::string_view svName) const
{
    const auto itFound = std::find_if(entries.begin(), entries.end(),
        [svName](const ModelEntry& oEntry)
        {
            return oEntry.name == svName;
        });
    return itFound == entries.end() ? nullptr : &*itFound;
}

SolverResult SolverResult::makeError(std::string strMessage)
{
    SolverResult oResult;
    oResult.status = SolverStatus::Error;

    Diagnostic oDiagnostic;
    oDiagnostic.severity = DiagnosticSeverity::Error;
    oDiagnostic.message = std::move(strMessage);
    oResult.diagnostics.push_back(std::move(oDiagnostic));

    return oResult;
}

} // namespace z3wb
