#pragma once

#include "core/domain/Problem.hpp"
#include "core/domain/SolverResult.hpp"
#include "core/solver/ICancellation.hpp"
#include "core/solver/SolverConfig.hpp"

#include <memory>
#include <string>

namespace z3wb {

// Backend-neutral solving interface. Implementations own all backend-specific
// state (contexts, processes); nothing backend-related may leak through this
// boundary. All calls are const: backends must keep per-request state local.
class ISolver
{
public:
    virtual ~ISolver() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;

    [[nodiscard]] virtual SolverResult solve(const Problem& oProblem,
        const SolverConfig& oConfig, const std::shared_ptr<ICancellation>& spCancellation) const = 0;

    // Human-inspectable SMT-LIB2 rendering of what the backend receives.
    // Used by the SMT-LIB2 viewer and the export function.
    [[nodiscard]] virtual std::string toSmtLib2(const Problem& oProblem,
        const SolverConfig& oConfig) const = 0;
};

} // namespace z3wb
