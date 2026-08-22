#include <doctest/doctest.h>

#include "core/solver/AtomicCancellation.hpp"

TEST_CASE("atomic cancellation starts clear and flips exactly once")
{
    z3wb::AtomicCancellation oCancel;
    CHECK_FALSE(oCancel.isCancelled());

    oCancel.cancel();
    CHECK(oCancel.isCancelled());

    // Repeated cancel calls are idempotent.
    oCancel.cancel();
    CHECK(oCancel.isCancelled());
}
