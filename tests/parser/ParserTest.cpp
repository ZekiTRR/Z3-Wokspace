#include <doctest/doctest.h>

#include "core/parser/AST.hpp"
#include "core/domain/Expression.hpp"

namespace {

z3wb::ast::ParseOutput parseText(std::string_view svSource)
{
    const z3wb::LexResult oLexed = z3wb::lex(svSource);
    return z3wb::ast::parse(oLexed.tokens);
}

} // namespace

TEST_CASE("parser reads var and constraint statements")
{
    using namespace z3wb;

    const ast::ParseOutput oOut = parseText(
        "var x: Int\n"
        "var flag: Bool\n"
        "var eax: BitVec(32)\n"
        "constraint x > 10\n");

    CHECK_FALSE(oOut.hasErrors());
    REQUIRE(oOut.statements.size() == 4);

    const auto* pVarX = std::get_if<ast::VarDecl>(&oOut.statements[0]);
    REQUIRE(pVarX != nullptr);
    CHECK(pVarX->name == "x");
    CHECK((pVarX->type == VariableType::Int));

    const auto* pVarEax = std::get_if<ast::VarDecl>(&oOut.statements[2]);
    REQUIRE(pVarEax != nullptr);
    CHECK((pVarEax->type == VariableType::BitVec));
    CHECK(pVarEax->params.uBitVecWidth == 32);

    const auto* pConstraint = std::get_if<ast::ConstraintDecl>(&oOut.statements[3]);
    REQUIRE(pConstraint != nullptr);
    CHECK(toString(pConstraint->expr) == "(x > 10)");
}

TEST_CASE("precedence: bitwise binds tighter than comparison")
{
    using namespace z3wb;

    // RE-oriented precedence: (x ^ 0x1337) == 0x4242, not x ^ (0x1337 == 0x4242).
    const ast::ParseOutput oOut = parseText("constraint x ^ 0x1337 == 0x4242");
    CHECK_FALSE(oOut.hasErrors());
    REQUIRE(oOut.statements.size() == 1);

    const auto* pConstraint = std::get_if<ast::ConstraintDecl>(&oOut.statements[0]);
    REQUIRE(pConstraint != nullptr);
    // Canonical form prints Int constants in decimal; hex spelling is kept
    // only for BitVec values after semantic analysis.
    CHECK(toString(pConstraint->expr) == "((x ^ 4919) == 16962)");
}

TEST_CASE("precedence table follows the documented order")
{
    using namespace z3wb;

    SUBCASE("multiplication over addition")
    {
        const ast::ParseOutput oOut = parseText("constraint 1 + 2 * 3 == 7");
        const auto* pConstraint = std::get_if<ast::ConstraintDecl>(&oOut.statements[0]);
        CHECK(toString(pConstraint->expr) == "((1 + (2 * 3)) == 7)");
    }

    SUBCASE("and over or")
    {
        const ast::ParseOutput oOut = parseText("constraint true || false && true");
        const auto* pConstraint = std::get_if<ast::ConstraintDecl>(&oOut.statements[0]);
        CHECK(toString(pConstraint->expr) == "(true || (false && true))");
    }

    SUBCASE("shifts bind tighter than bitwise and/or")
    {
        const ast::ParseOutput oOut = parseText("constraint x & 1 << 2");
        const auto* pConstraint = std::get_if<ast::ConstraintDecl>(&oOut.statements[0]);
        // At parse level literals stay plain Int constants; the BitVec width
        // is assigned later during semantic analysis.
        CHECK(toString(pConstraint->expr) == "(x & (1 << 2))");
    }

    SUBCASE("unary minus binds tightest")
    {
        const ast::ParseOutput oOut = parseText("constraint -x + y < 0");
        const auto* pConstraint = std::get_if<ast::ConstraintDecl>(&oOut.statements[0]);
        CHECK(toString(pConstraint->expr) == "(((-x) + y) < 0)");
    }

    SUBCASE("parentheses override everything")
    {
        const ast::ParseOutput oOut = parseText("constraint (1 + 2) * 3 == 9");
        const auto* pConstraint = std::get_if<ast::ConstraintDecl>(&oOut.statements[0]);
        CHECK(toString(pConstraint->expr) == "(((1 + 2) * 3) == 9)");
    }
}

TEST_CASE("parser reports errors with positions and recovers")
{
    using namespace z3wb;

    const ast::ParseOutput oOut = parseText(
        "var x Int\n"          // missing ':' — error on line 1
        "constraint x > )\n"   // unexpected token — line 2
        "constraint y > 0\n"); // must still parse — line 3

    REQUIRE(oOut.hasErrors());
    REQUIRE(oOut.diagnostics.size() >= 2);
    CHECK(oOut.diagnostics[0].location.iLine == 1);
    CHECK(oOut.diagnostics[1].location.iLine == 2);

    // The third statement survived recovery.
    bool bFoundLast = false;
    for (const auto& oStatement : oOut.statements)
    {
        if (const auto* pConstraint = std::get_if<ast::ConstraintDecl>(&oStatement))
        {
            if (toString(pConstraint->expr) == "(y > 0)")
            {
                bFoundLast = true;
            }
        }
    }
    CHECK(bFoundLast);
}

TEST_CASE("bit-vector width validation")
{
    using namespace z3wb;

    SUBCASE("valid width")
    {
        const ast::ParseOutput oOut = parseText("var a: BitVec(64)");
        CHECK_FALSE(oOut.hasErrors());
        CHECK(oOut.statements.size() == 1);
    }

    SUBCASE("zero width is rejected")
    {
        CHECK(parseText("var a: BitVec(0)").hasErrors());
    }

    SUBCASE("width above 64 is rejected")
    {
        CHECK(parseText("var a: BitVec(65)").hasErrors());
    }

    SUBCASE("non-numeric width is rejected")
    {
        CHECK(parseText("var a: BitVec(x)").hasErrors());
    }
}
