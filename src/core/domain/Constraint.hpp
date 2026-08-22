#pragma once

#include "core/domain/Expression.hpp"
#include "core/domain/Ids.hpp"
#include "core/domain/SourceLocation.hpp"

#include <string>

namespace z3wb {

// An independently editable assertion: can be toggled, commented and traced
// back to the user-authored source line it came from.
struct Constraint
{
    ConstraintId id;
    Expression expr;
    bool enabled = true;
    std::string comment;
    SourceLocation location{};
};

} // namespace z3wb
