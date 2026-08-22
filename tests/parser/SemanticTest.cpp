#include <doctest/doctest.h>

#include "core/domain/Expression.hpp"
#include "core/parser/ProblemParser.hpp"

#include <string>
#include <type_traits>

namespace {

z3wb::ParseResult parse(std::string_view svSource)
{
    return z3wb::parseProblem(svSource, "test");
}

} // namespace

TEST_CASE("full pipeline builds a solvable Problem from DSL text")
{
    using namespace z3wb;

    const ParseResult oResult = parse(
        "var x: Int\n"
        "var y: Int\n"
        "var flag: Bool\n"
        "\n"
        "constraint x >= 10\n"
        "constraint x <= 100\n"
        "constraint y == x + 20\n"
        "constraint flag == true\n");

    REQUIRE(oResult.ok());
    const Problem& oProblem = *oResult.problem;

    CHECK(oProblem.variableCount() == 3);
    CHECK(oProblem.constraintCount() == 4);

    // References resolved to real variable ids: rebuild the third constraint
    // from resolved entities and compare structurally.
    const Variable* pX = oProblem.findVariable("x");
    const Variable* pY = oProblem.findVariable("y");
    REQUIRE(pX != nullptr);
    REQUIRE(pY != nullptr);

    const Constraint& oY = oProblem.constraints()[2];
    const Expression oExpected = Expression::binary(BinaryOp::Eq,
        Expression::variable(pY->id, "y"),
        Expression::binary(BinaryOp::Add,
            Expression::variable(pX->id, "x"), Expression::integer(20)));
    CHECK((oY.expr == oExpected));
}

TEST_CASE("RE-style bit-vector problem parses and resolves")
{
    using namespace z3wb;

    const ParseResult oResult = parse(
        "var x: BitVec(32)\n"
        "constraint (x ^ 0x1337) + 10 == 0x4242\n"
        "// unsigned compare\n"
        "constraint x < 0xFFFFFFFF\n");

    REQUIRE(oResult.ok());
    const Problem& oProblem = *oResult.problem;

    CHECK(oProblem.constraintCount() == 2);

    // Explicit parentheses make the addition bind where the author wanted;
    // both literals were coerced to BitVec(32). BitVec values display in hex.
    const std::string strDisplay = toString(oProblem.constraints()[0].expr);
    CHECK(strDisplay == "(((x ^ 0x1337:32) + 0xA:32) == 0x4242:32)");
}

TEST_CASE("unknown variable is reported")
{
    using namespace z3wb;

    const ParseResult oResult = parse(
        "var x: Int\n"
        "constraint foo > 10\n");

    CHECK_FALSE(oResult.ok());
    REQUIRE_FALSE(oResult.diagnostics.empty());
    CHECK(oResult.diagnostics[0].message.find("Unknown variable \"foo\"") != std::string::npos);
}

TEST_CASE("type mismatch Int vs Bool is reported")
{
    using namespace z3wb;

    const ParseResult oResult = parse(
        "var x: Int\n"
        "var flag: Bool\n"
        "constraint x == flag\n");

    CHECK_FALSE(oResult.ok());
    REQUIRE_FALSE(oResult.diagnostics.empty());
    CHECK(oResult.diagnostics[0].message.find("cannot compare") != std::string::npos);
}

TEST_CASE("bitwise operators require BitVec operands")
{
    using namespace z3wb;

    SUBCASE("and on Int rejected")
    {
        const ParseResult oResult = parse(
            "var x: Int\n"
            "constraint x & 1 == 1\n");
        CHECK_FALSE(oResult.ok());
    }

    SUBCASE("shift on BitVec accepted")
    {
        const ParseResult oResult = parse(
            "var x: BitVec(8)\n"
            "constraint (x << 2) > x\n");
        CHECK(oResult.ok());
    }

    SUBCASE("width mismatch rejected")
    {
        const ParseResult oResult = parse(
            "var a: BitVec(8)\n"
            "var b: BitVec(16)\n"
            "constraint a & b == a\n");
        CHECK_FALSE(oResult.ok());
        CHECK(oResult.diagnostics[0].message.find("BitVec(8)") != std::string::npos);
    }
}

TEST_CASE("Int and Real do not mix implicitly")
{
    using namespace z3wb;

    const ParseResult oResult = parse(
        "var i: Int\n"
        "var r: Real\n"
        "constraint r == i + 1.5\n");

    CHECK_FALSE(oResult.ok());
    REQUIRE_FALSE(oResult.diagnostics.empty());
    CHECK(oResult.diagnostics[0].message.find("Type mismatch") != std::string::npos);
}

TEST_CASE("constraint must be Boolean at the root")
{
    using namespace z3wb;

    const ParseResult oResult = parse(
        "var x: Int\n"
        "constraint x + 1\n");

    CHECK_FALSE(oResult.ok());
    CHECK(oResult.diagnostics[0].message.find("Boolean expression") != std::string::npos);
}

TEST_CASE("duplicate variable declaration is reported")
{
    using namespace z3wb;

    const ParseResult oResult = parse(
        "var x: Int\n"
        "var x: Bool\n"
        "constraint true\n");

    CHECK_FALSE(oResult.ok());
    REQUIRE_FALSE(oResult.diagnostics.empty());
    CHECK(oResult.diagnostics[0].message.find("already defined") != std::string::npos);
}

TEST_CASE("multiple errors are collected in one pass")
{
    using namespace z3wb;

    const ParseResult oResult = parse(
        "constraint unknownA > 1\n"
        "constraint 1 + \"text\"\n"
        "constraint unknownB < 2\n");

    CHECK_FALSE(oResult.ok());
    CHECK(oResult.diagnostics.size() >= 3);
}
