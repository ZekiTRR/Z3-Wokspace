#include <doctest/doctest.h>

#include "core/domain/Ids.hpp"

TEST_CASE("strong ids are session-unique and typed")
{
    using namespace z3wb;

    const VariableId oFirst = makeVariableId();
    const VariableId oSecond = makeVariableId();
    const ConstraintId oConstraint = makeConstraintId();

    CHECK(oFirst.isValid());
    CHECK(oSecond.isValid());
    CHECK(oFirst != oSecond);

    // Different id families are distinct types: comparing VariableId with
    // ConstraintId does not compile, which is exactly the intended safety.
    CHECK(oFirst != oSecond);
    CHECK(oConstraint.isValid());
}

TEST_CASE("default-constructed ids are invalid")
{
    const z3wb::ProblemId oId;
    CHECK_FALSE(oId.isValid());
    CHECK(oId.value() == 0);
}
