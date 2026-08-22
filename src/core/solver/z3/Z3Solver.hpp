#pragma once

#include "core/domain/Problem.hpp"
#include "core/domain/SolverResult.hpp"
#include "core/solver/ISolver.hpp"
#include "core/solver/SolverConfig.hpp"

#include <z3++.h>

namespace z3wb {

// Z3-backed ISolver implementation. Each solve() call uses a fresh z3::context
// so solver state never leaks between requests and the call is trivially
// thread-safe as long as one instance serves one request at a time.
class Z3Solver final : public ISolver
{
public:
    [[nodiscard]] std::string_view name() const override;

    [[nodiscard]] SolverResult solve(const Problem& oProblem,
        const SolverConfig& oConfig, const std::shared_ptr<ICancellation>& spCancellation) const override;

    [[nodiscard]] std::string toSmtLib2(const Problem& oProblem,
        const SolverConfig& oConfig) const override;

private:
    // Builds the backend solver with declarations and enabled assertions.
    // Returns nullopt and fills oDiags on conversion errors.
    [[nodiscard]] static std::optional<z3::solver> buildBackend(z3::context& oContext,
        const Problem& oProblem, const SolverConfig& oConfig,
        std::vector<Diagnostic>& vecDiags);
};

} // namespace z3wb
