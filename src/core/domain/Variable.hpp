#pragma once

#include "core/domain/Ids.hpp"

#include <cstdint>
#include <string>
#include <variant>

namespace z3wb {

enum class VariableType
{
    Bool,
    Int,
    Real,
    BitVec,
    String,
    Array,
};

// Extra sort parameters. Only BitVec width is used in the MVP; the struct is
// the single extension point for future sorts (arrays, floats, enums).
struct TypeParams
{
    unsigned uBitVecWidth = 0;
};

struct Variable
{
    VariableId id;
    std::string name;
    VariableType type = VariableType::Int;
    TypeParams params{};
};

// Names are restricted to [A-Za-z_][A-Za-z0-9_]* so they stay valid across
// the DSL, SMT-LIB2 export and JSON serialization without escaping.
[[nodiscard]] bool isValidVariableName(std::string_view svName);

[[nodiscard]] std::string_view toString(VariableType eType);

} // namespace z3wb
