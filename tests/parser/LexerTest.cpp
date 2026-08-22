#include <doctest/doctest.h>

#include "core/parser/Lexer.hpp"

namespace {

std::vector<z3wb::TokenType> tokenTypes(std::string_view svSource)
{
    const z3wb::LexResult oResult = z3wb::lex(svSource);
    std::vector<z3wb::TokenType> vecTypes;
    vecTypes.reserve(oResult.tokens.size());
    for (const z3wb::Token& oToken : oResult.tokens)
    {
        vecTypes.push_back(oToken.type);
    }
    return vecTypes;
}

} // namespace

TEST_CASE("lexer produces the expected token stream")
{
    using namespace z3wb;

    const auto vecTypes = tokenTypes("var x: BitVec(32)\nconstraint x ^ 0x1337 == 0x4242");

    REQUIRE(vecTypes.size() == 14);
    CHECK(vecTypes[0] == TokenType::KwVar);
    CHECK(vecTypes[1] == TokenType::Identifier);
    CHECK(vecTypes[2] == TokenType::Colon);
    CHECK(vecTypes[3] == TokenType::TyBitVec);
    CHECK(vecTypes[4] == TokenType::LParen);
    CHECK(vecTypes[5] == TokenType::IntLiteral);
    CHECK(vecTypes[6] == TokenType::RParen);
    CHECK(vecTypes[7] == TokenType::KwConstraint);
    CHECK(vecTypes[8] == TokenType::Identifier);
    CHECK(vecTypes[9] == TokenType::OpCaret);
    CHECK(vecTypes[10] == TokenType::IntLiteral);
    CHECK(vecTypes[11] == TokenType::OpEq);
    CHECK(vecTypes[12] == TokenType::IntLiteral);
    CHECK(vecTypes[13] == TokenType::Eof);
}

TEST_CASE("lexer tracks line and column positions")
{
    using namespace z3wb;

    const LexResult oResult = lex("var x: Int\nconstraint x > 10");
    REQUIRE(oResult.tokens.size() == 9);

    // 'constraint' starts at line 2, column 1.
    const Token& oConstraintToken = oResult.tokens[4];
    CHECK(oConstraintToken.type == TokenType::KwConstraint);
    CHECK(oConstraintToken.location.iLine == 2);
    CHECK(oConstraintToken.location.iColumn == 1);

    // '10' on line 2, column 16.
    const Token& oTen = oResult.tokens[7];
    CHECK(oTen.text == "10");
    CHECK(oTen.location.iLine == 2);
    CHECK(oTen.location.iColumn == 16);
}

TEST_CASE("lexer skips comments")
{
    using namespace z3wb;

    const auto vecTypes = tokenTypes(
        "// line comment\n"
        "var /* inline */ x: Int");

    REQUIRE(vecTypes.size() == 5);
    CHECK(vecTypes[0] == TokenType::KwVar);
    CHECK(vecTypes[1] == TokenType::Identifier);
    CHECK(vecTypes[2] == TokenType::Colon);
    CHECK(vecTypes[3] == TokenType::TyInt);
    CHECK(vecTypes[4] == TokenType::Eof);
}

TEST_CASE("string literals decode escape sequences")
{
    using namespace z3wb;

    const LexResult oResult = lex("\"a\\n\\\"b\\\"\"");
    REQUIRE(oResult.tokens.size() == 2);
    CHECK(oResult.tokens[0].type == TokenType::StringLiteral);
    CHECK(oResult.tokens[0].text == "a\n\"b\"");
}

TEST_CASE("lexer reports recoverable errors and keeps scanning")
{
    using namespace z3wb;

    SUBCASE("unknown character")
    {
        const LexResult oBad = lex("x @ y");
        REQUIRE(oBad.hasErrors());
        CHECK(oBad.diagnostics.size() == 1);
        CHECK(oBad.diagnostics[0].location.iColumn == 3);
    }

    SUBCASE("unterminated string")
    {
        const LexResult oResult = lex("\"oops");
        CHECK(oResult.hasErrors());
    }

    SUBCASE("malformed number")
    {
        const LexResult oResult = lex("123abc");
        CHECK(oResult.hasErrors());
        // The numeric prefix is kept for recovery; the trailing letters are
        // consumed so the parser does not cascade the same problem.
        REQUIRE(oResult.tokens.size() == 2); // literal + Eof
        CHECK(oResult.tokens[0].type == TokenType::IntLiteral);
        CHECK(oResult.tokens[0].text == "123");
    }

    SUBCASE("hex literal without digits")
    {
        const LexResult oResult = lex("0x");
        CHECK(oResult.hasErrors());
    }
}
