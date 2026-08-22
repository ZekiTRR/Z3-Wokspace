#pragma once

#include <memory>

namespace z3wb {

// Cooperative cancellation signal checked by solver implementations at safe
// points. Thread-safe implementations live on the GUI side (Phase 6); inside
// a single solve request Z3 itself is interrupted via the configured timeout.
class ICancellation
{
public:
    virtual ~ICancellation() = default;

    [[nodiscard]] virtual bool isCancelled() const = 0;
};

// Null-object for callers that do not support cancellation.
class NoCancellation final : public ICancellation
{
public:
    [[nodiscard]] bool isCancelled() const override { return false; }
};

} // namespace z3wb
