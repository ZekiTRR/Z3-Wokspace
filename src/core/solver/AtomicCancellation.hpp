#pragma once

#include "core/solver/ICancellation.hpp"

#include <atomic>

namespace z3wb {

// Thread-safe cooperative cancellation flag. cancel() is called from the GUI
// thread; isCancelled() is polled by solver implementations at safe points.
// Relaxed ordering is sufficient: a slightly delayed visibility only means
// cancellation takes effect one check later.
class AtomicCancellation final : public ICancellation
{
public:
    void cancel() noexcept
    {
        m_bCancelled.store(true, std::memory_order_relaxed);
    }

    [[nodiscard]] bool isCancelled() const override
    {
        return m_bCancelled.load(std::memory_order_relaxed);
    }

private:
    std::atomic<bool> m_bCancelled{false};
};

} // namespace z3wb
