#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace z3wb {

// -----------------------------------------------------------------------------
// Strong-typed identifiers. The tag type prevents mixing ids of different
// entities while keeping zero runtime overhead.
// -----------------------------------------------------------------------------
template<typename TagT>
class StrongId
{
public:
    constexpr StrongId() noexcept = default;
    explicit constexpr StrongId(std::uint64_t uValue) noexcept
        : m_uValue(uValue)
    {
    }

    // Zero is reserved as "no id".
    [[nodiscard]] constexpr bool isValid() const noexcept { return m_uValue != 0; }
    [[nodiscard]] constexpr std::uint64_t value() const noexcept { return m_uValue; }

    friend constexpr bool operator==(const StrongId& oLhs, const StrongId& oRhs) noexcept
    {
        return oLhs.m_uValue == oRhs.m_uValue;
    }
    friend constexpr bool operator!=(const StrongId& oLhs, const StrongId& oRhs) noexcept
    {
        return !(oLhs == oRhs);
    }
    friend constexpr bool operator<(const StrongId& oLhs, const StrongId& oRhs) noexcept
    {
        return oLhs.m_uValue < oRhs.m_uValue;
    }

private:
    std::uint64_t m_uValue = 0;
};

struct VariableIdTag
{
};
struct ConstraintIdTag
{
};
struct ProblemIdTag
{
};

using VariableId = StrongId<VariableIdTag>;
using ConstraintId = StrongId<ConstraintIdTag>;
using ProblemId = StrongId<ProblemIdTag>;

// Session-unique id issuers. Process-wide counters are deliberate: ids only
// need uniqueness within one application run — persisted documents reference
// entities by name and source text, not by id.
[[nodiscard]] VariableId makeVariableId();
[[nodiscard]] ConstraintId makeConstraintId();
[[nodiscard]] ProblemId makeProblemId();

} // namespace z3wb

template<typename TagT>
struct std::hash<z3wb::StrongId<TagT>>
{
    std::size_t operator()(const z3wb::StrongId<TagT>& oId) const noexcept
    {
        return std::hash<std::uint64_t>{}(oId.value());
    }
};
