#include <doctest/doctest.h>

#include "core/domain/Expression.hpp"

TEST_CASE("expression factories build the expected node kinds")
{
    using namespace z3wb;

    const Expression oConst = Expression::integer(42);
    CHECK(std::holds_alternative<Constant>(oConst.node()));

    const VariableId oX = makeVariableId();
    const Expression oVar = Expression::variable(oX, "x");
    const auto* pRef = std::get_if<VariableRef>(&oVar.node());
    REQUIRE(pRef != nullptr);
    CHECK(pRef->id == oX);
    CHECK(pRef->strName == "x");

    const Expression oBin = Expression::binary(BinaryOp::Add, oVar, oConst);
    CHECK(std::holds_alternative<BinaryExpression>(oBin.node()));
}

TEST_CASE("structural equality ignores subtree sharing")
{
    using namespace z3wb;

    const Expression oShared = Expression::integer(7);
    const Expression oA = Expression::binary(BinaryOp::Add, oShared, oShared);
    const Expression oB = Expression::binary(BinaryOp::Add,
        Expression::integer(7), Expression::integer(7));

    // Parenthesized so doctest does not try to stringify Expression operands.
    CHECK((oA == oB));
    CHECK((oA != Expression::binary(BinaryOp::Sub, oShared, oShared)));
    CHECK((oA != Expression::binary(BinaryOp::Add, oShared, Expression::integer(8))));
}

TEST_CASE("canonical display form is fully parenthesized")
{
    using namespace z3wb;

    SUBCASE("arithmetic")
    {
        const Expression oExpr = Expression::binary(BinaryOp::Eq,
            Expression::binary(BinaryOp::Add,
                Expression::variable(makeVariableId(), "x"), Expression::integer(10)),
            Expression::variable(makeVariableId(), "y"));

        CHECK(toString(oExpr) == "((x + 10) == y)");
    }

    SUBCASE("bit-vector literal keeps its width suffix")
    {
        const BitVecValue oValue{32, 0x1337};
        CHECK(toString(Expression::bitVector(oValue)) == "0x1337:32");
    }

    SUBCASE("unary and boolean operators")
    {
        const Expression oExpr = Expression::unary(UnaryOp::Not,
            Expression::binary(BinaryOp::And,
                Expression::boolean(true), Expression::boolean(false)));

        CHECK(toString(oExpr) == "(!(true && false))");
    }

    SUBCASE("real and call constants")
    {
        CHECK(toString(Expression::real("3.14")) == "3.14");
        CHECK(toString(Expression::call("f", {Expression::integer(1), Expression::integer(2)}))
            == "f(1, 2)");
    }
}

TEST_CASE("remapVariables rewrites references including nested ones")
{
    using namespace z3wb;

    const VariableId oOldX = makeVariableId();
    const VariableId oNewX = makeVariableId();

    const Expression oSource = Expression::binary(BinaryOp::Eq,
        Expression::binary(BinaryOp::BvXor,
            Expression::variable(oOldX, "x"),
            Expression::bitVector(BitVecValue{32, 0x1337})),
        Expression::bitVector(BitVecValue{32, 0x4242}));

    const VariableIdMap mapIds{{oOldX, oNewX}};
    const Expression oRemapped = remapVariables(oSource, mapIds);

    const auto* pBinary = std::get_if<BinaryExpression>(&oRemapped.node());
    REQUIRE(pBinary != nullptr);

    const auto* pLhs = std::get_if<BinaryExpression>(&pBinary->spLhs->node());
    REQUIRE(pLhs != nullptr);

    const auto* pRef = std::get_if<VariableRef>(&pLhs->spLhs->node());
    REQUIRE(pRef != nullptr);
    CHECK(pRef->id == oNewX);
    CHECK((oRemapped != oSource));
}
