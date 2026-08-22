#include "core/domain/Variable.hpp"

namespace z3wb {

namespace {

constexpr bool isAsciiLetter(char ch) noexcept
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

constexpr bool isAsciiDigit(char ch) noexcept
{
    return ch >= '0' && ch <= '9';
}

} // namespace

bool isValidVariableName(std::string_view svName)
{
    if (svName.empty())
    {
        return false;
    }

    if (!isAsciiLetter(svName.front()) && svName.front() != '_')
    {
        return false;
    }

    for (const char ch : svName)
    {
        if (!isAsciiLetter(ch) && !isAsciiDigit(ch) && ch != '_')
        {
            return false;
        }
    }

    return true;
}

std::string_view toString(VariableType eType)
{
    switch (eType)
    {
        case VariableType::Bool:
            return "Bool";
        case VariableType::Int:
            return "Int";
        case VariableType::Real:
            return "Real";
        case VariableType::BitVec:
            return "BitVec";
        case VariableType::String:
            return "String";
        case VariableType::Array:
            return "Array";
    }
    return "Unknown";
}

} // namespace z3wb
