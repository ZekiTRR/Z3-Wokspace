#pragma once

#include "core/domain/SourceLocation.hpp"

#include <string>

namespace z3wb {

enum class TokenType
{
    Eof,

    // Statement keywords.
    KwVar,
    KwConstraint,

    // Literal keywords.
    KwTrue,
    KwFalse,

    // Type names.
    TyBool,
    TyInt,
    TyReal,
    TyString,
    TyBitVec,

    // Literals and identifiers.
    IntLiteral,     // decimal, 0x hex, 0b binary; raw spelling kept in text
    RealLiteral,    // digits.digits; raw spelling kept in text
    StringLiteral,  // decoded value kept in text
    Identifier,

    // Punctuation.
    Colon,
    LParen,
    RParen,

    // Operators.
    OpOr,       // ||
    OpAnd,      // &&
    OpEq,       // ==
    OpNeq,      // !=
    OpLt,       // <
    OpLe,       // <=
    OpGt,       // >
    OpGe,       // >=
    OpPlus,     // +
    OpMinus,    // -
    OpStar,     // *
    OpSlash,    // /
    OpPercent,  // %
    OpAmp,      // &
    OpPipe,     // |
    OpCaret,    // ^
    OpShl,      // <<
    OpShr,      // >>
    OpTilde,    // ~
    OpNot,      // !
};

struct Token
{
    TokenType type = TokenType::Eof;
    std::string text;
    SourceLocation location{};
};

} // namespace z3wb
