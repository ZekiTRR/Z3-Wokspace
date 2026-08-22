#pragma once

#include <cstdint>

namespace z3wb {

// Fixed-width bit-vector value, stored in the low uWidth bits.
// The MVP targets the classic RE widths (8/16/32/64); 64 is the storage limit.
struct BitVecValue
{
    unsigned uWidth = 0;
    std::uint64_t uBits = 0;

    friend constexpr bool operator==(const BitVecValue& oLhs, const BitVecValue& oRhs) noexcept
    {
        return oLhs.uWidth == oRhs.uWidth && oLhs.uBits == oRhs.uBits;
    }
    friend constexpr bool operator!=(const BitVecValue& oLhs, const BitVecValue& oRhs) noexcept
    {
        return !(oLhs == oRhs);
    }
};

} // namespace z3wb
