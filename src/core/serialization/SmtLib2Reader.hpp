#pragma once

#include "core/domain/Problem.hpp"
#include "core/serialization/JsonProjectStorage.hpp"

#include <string>

namespace z3wb {

// Reads the SMT-LIB2 subset produced by SmtLib2Serializer:
// declare-const/assert commands over Bool/Int/Real/String/BitVec and the
// operator set of the workbench DSL. Unknown commands (check-sat, set-info,
// ...) are skipped; unknown operators are reported as errors.
//
// The result contains unresolved variable references plus declared variables;
// the caller runs the normal parse pipeline afterwards (see DslPrinter).
class SmtLib2Reader
{
public:
    // On success returns the problem (name assigned by the caller) with
    // variables and constraints filled in. On failure returns nullopt and
    // fills *pError when provided.
    [[nodiscard]] std::optional<Problem> read(const std::string& strText,
        std::string strName, StorageError* pError) const;
};

} // namespace z3wb
