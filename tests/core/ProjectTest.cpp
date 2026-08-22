#include <doctest/doctest.h>

#include "core/domain/Project.hpp"

#include <type_traits>

namespace {

// Fills an existing problem with a BitVec crackme-style setup:
// x ^ 0x1337 == 0x4242.
void fillCrackmeProblem(z3wb::Problem& oProblem)
{
    z3wb::Variable oX;
    oX.name = "x";
    oX.type = z3wb::VariableType::BitVec;
    oX.params.uBitVecWidth = 32;
    REQUIRE(oProblem.addVariable(oX));

    const z3wb::Variable* pX = oProblem.findVariable("x");

    z3wb::Constraint oConstraint;
    oConstraint.expr = z3wb::Expression::binary(z3wb::BinaryOp::Eq,
        z3wb::Expression::binary(z3wb::BinaryOp::BvXor,
            z3wb::Expression::variable(pX->id, "x"),
            z3wb::Expression::bitVector(z3wb::BitVecValue{32, 0x1337})),
        z3wb::Expression::bitVector(z3wb::BitVecValue{32, 0x4242}));
    REQUIRE(oProblem.addConstraint(oConstraint));
}

} // namespace

TEST_CASE("project manages problems")
{
    using namespace z3wb;

    Project oProject("Example");

    Problem* pFirst = oProject.addProblem("crackme_01");
    REQUIRE(pFirst != nullptr);
    CHECK(pFirst->id().isValid());

    CHECK(oProject.addProblem("crackme_01") == nullptr); // duplicate name

    // Pointers into problems() are invalidated by mutations, so capture the
    // id before adding another problem.
    const ProblemId oFirstId = pFirst->id();

    Problem* pSecond = oProject.addProblem("test");
    REQUIRE(pSecond != nullptr);
    CHECK(oFirstId != pSecond->id());

    REQUIRE(oProject.findProblem("crackme_01") != nullptr);
    REQUIRE(oProject.findProblem(oFirstId) != nullptr);
    CHECK(oProject.removeProblem(oFirstId));
    CHECK(oProject.findProblem(oFirstId) == nullptr);
    CHECK_FALSE(oProject.removeProblem(oFirstId));
}

TEST_CASE("duplicateProblem deep-copies with fresh ids and remapped references")
{
    using namespace z3wb;

    Project oProject("Example");

    Problem* pSource = oProject.addProblem("original");
    REQUIRE(pSource != nullptr);
    fillCrackmeProblem(*pSource);

    const VariableId oOriginalVarId = pSource->findVariable("x")->id;
    const std::size_t stEnabledConstraints = pSource->enabledConstraintCount();

    Problem* pCopy = oProject.duplicateProblem(pSource->id(), "original_copy");
    REQUIRE(pCopy != nullptr);

    CHECK(pCopy->id().isValid());
    CHECK(pCopy->variableCount() == 1);
    CHECK(pCopy->constraintCount() == 1);
    CHECK(pCopy->enabledConstraintCount() == stEnabledConstraints);

    const Variable* pCopyVar = pCopy->findVariable("x");
    REQUIRE(pCopyVar != nullptr);
    CHECK(pCopyVar->id != oOriginalVarId);

    // The duplicated constraint must reference the copy's variable, not the
    // original one: compare structurally against a rebuilt expression.
    const Constraint& oConstraint = pCopy->constraints().front();
    const Expression oExpected = Expression::binary(BinaryOp::Eq,
        Expression::binary(BinaryOp::BvXor,
            Expression::variable(pCopyVar->id, "x"),
            Expression::bitVector(BitVecValue{32, 0x1337})),
        Expression::bitVector(BitVecValue{32, 0x4242}));
    CHECK((oConstraint.expr == oExpected));

    CHECK(oProject.duplicateProblem(pSource->id(), "original_copy") == nullptr); // name taken
}
