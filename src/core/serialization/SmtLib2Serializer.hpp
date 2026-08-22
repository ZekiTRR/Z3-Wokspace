#pragma once

#include "core/domain/Problem.hpp"

#include <string>

namespace z3wb {

// Renders a domain problem as SMT-LIB2 text. Implemented on the Domain layer
// (no Z3 headers) so the output is portable across solver backends.
//
// Supported subset: Bool/Int/Real/String/BitVec sorts and every operator of
// the workbench DSL. Sorts are inferred from the declared variables, so
// BitVec operations are emitted as unsigned SMT-LIB2 functions (bvult,
// bvudiv, ...) and negation picks bvneg vs unary minus correctly.
class SmtLib2Serializer
{
public:
    // Full document: declarations, enabled assertions, check-sat/get-model.
    [[nodiscard]] static std::string serialize(const Problem& oProblem);
};

} // namespace z3wb
