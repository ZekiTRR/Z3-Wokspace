#include <doctest/doctest.h>

#include "core/domain/Problem.hpp"
#include "core/domain/Variable.hpp"

TEST_CASE("variable name validation follows the DSL identifier rules")
{
    using namespace z3wb;

    CHECK(isValidVariableName("x"));
    CHECK(isValidVariableName("_private"));
    CHECK(isValidVariableName("eax_01"));

    CHECK_FALSE(isValidVariableName(""));
    CHECK_FALSE(isValidVariableName("1x"));
    CHECK_FALSE(isValidVariableName("my-var"));
    CHECK_FALSE(isValidVariableName("x y"));
}

TEST_CASE("problem manages variables")
{
    using namespace z3wb;

    Problem oProblem("basic_arithmetic");

    Variable oX;
    oX.name = "x";
    oX.type = VariableType::Int;
    CHECK(oProblem.addVariable(oX));

    // Duplicate names are rejected; the stored variable got a fresh id.
    Variable oDuplicate;
    oDuplicate.name = "x";
    CHECK_FALSE(oProblem.addVariable(oDuplicate));

    Variable oBv;
    oBv.name = "eax";
    oBv.type = VariableType::BitVec;
    CHECK(oProblem.addVariable(oBv));

    const Variable* pFound = oProblem.findVariable("x");
    REQUIRE(pFound != nullptr);
    CHECK(pFound->id.isValid());

    const Variable* pBitVec = oProblem.findVariable("eax");
    REQUIRE(pBitVec != nullptr);
    CHECK(pBitVec->params.uBitVecWidth == 32); // defaulted

    CHECK(oProblem.variableCount() == 2);
    CHECK(oProblem.findVariable("missing") == nullptr);
    CHECK(oProblem.removeVariable(pFound->id));
    CHECK(oProblem.variableCount() == 1);
}

TEST_CASE("problem manages constraints")
{
    using namespace z3wb;

    Problem oProblem("constraints");

    Constraint oConstraint;
    oConstraint.expr = Expression::binary(BinaryOp::Gt,
        Expression::variable(makeVariableId(), "x"), Expression::integer(10));
    oConstraint.comment = "lower bound";
    CHECK(oProblem.addConstraint(oConstraint));

    const Constraint* pStored = oProblem.constraints().front().id.isValid()
        ? &oProblem.constraints().front()
        : nullptr;
    REQUIRE(pStored != nullptr);

    CHECK(oProblem.enabledConstraintCount() == 1);
    CHECK(oProblem.setConstraintEnabled(pStored->id, false));
    CHECK(oProblem.enabledConstraintCount() == 0);
    CHECK_FALSE(oProblem.setConstraintEnabled(makeConstraintId(), true));

    CHECK(oProblem.removeConstraint(pStored->id));
    CHECK(oProblem.constraintCount() == 0);
    CHECK_FALSE(oProblem.removeConstraint(pStored->id));
}
