#include "core/solver/SolverFactory.hpp"

#include "core/solver/z3/Z3Solver.hpp"

namespace z3wb {

std::shared_ptr<ISolver> makeDefaultSolver()
{
    return std::make_shared<Z3Solver>();
}

} // namespace z3wb
