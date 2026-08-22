#pragma once

#include "core/domain/Problem.hpp"
#include "core/domain/SolverResult.hpp"

#include <optional>
#include <vector>

#include <z3++.h>

namespace z3wb {

// Converts a Z3 model into a domain Model. Values are produced for every
// declared variable in problem order, so the UI keeps a stable layout.
class Z3ModelConverter
{
public:
    [[nodiscard]] static std::optional<Model> convert(z3::model& oModel,
        const Problem& oProblem, std::vector<Diagnostic>& vecDiags);
};

} // namespace z3wb
