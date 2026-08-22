#pragma once

#include <chrono>
#include <cstdint>

namespace z3wb {

// Solver run configuration. Kept backend-agnostic so future solvers
// (CVC5, Boolector) map their own options onto the same fields.
struct SolverConfig
{
    // Hard budget for a single solve request; enforced through the backend
    // (Z3 :timeout) so a runaway problem cannot block the worker thread.
    std::chrono::milliseconds timeout{5000};
    std::uint64_t uRandomSeed = 0;
    bool bProduceModel = true;
};

} // namespace z3wb
