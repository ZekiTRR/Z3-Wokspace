#pragma once

#include "core/solver/ISolver.hpp"

#include <memory>

namespace z3wb {

// Creates the application's default solver backend. Declared here so callers
// (app, GUI) depend only on ISolver — the concrete backend and its headers
// stay inside the solver adapter layer.
[[nodiscard]] std::shared_ptr<ISolver> makeDefaultSolver();

} // namespace z3wb
