#include "core/serialization/SmtLib2Reader.hpp"

#include <cctype>
#include <cstdlib>
#include <optional>
#include <unordered_map>
#include <utility>

namespace z3wb {

namespace {

constexpr unsigned k_uMaxBvWidth = 64;

class Lexer
{
public:
    enum class Kind { LParen, RParen, Symbol, Numeral, String, End };

    struct Token
    {
        Kind eKind = Kind::End;
        std::string strText;
        SourceLocation location{};
    };

    explicit Lexer(const std::string& strText)
        : m_strText(strText)
    {
    }

    [[nodiscard]] Token next()
    {
        skipSpaces();
        Token oToken;
        oToken.location.iLine = m_iLine;
        oToken.location.iColumn = m_iColumn;

        if (m_stPos >= m_strText.size())
        {
            oToken.eKind = Kind::End;
            return oToken;
        }

        const char ch = m_strText[m_stPos];
        if (ch == '(')
        {
            advance();
            oToken.eKind = Kind::LParen;
            oToken.strText = "(";
            return oToken;
        }
        if (ch == ')')
        {
            advance();
            oToken.eKind = Kind::RParen;
            oToken.strText = ")";
            return oToken;
        }
        if (ch == '"')
        {
            return lexString();
        }
        if (isSymbolChar(ch))
        {
            const std::size_t stStart = m_stPos;
            while (m_stPos < m_strText.size() && isSymbolChar(m_strText[m_stPos]))
            {
                advance();
            }
            oToken.eKind = Kind::Symbol;
            // Distinguish numerals (incl. #x hex) from symbols.
            const std::string strLexeme = m_strText.substr(stStart, m_stPos - stStart);
            if (std::isdigit(static_cast<unsigned char>(strLexeme[0])) != 0
                || strLexeme.rfind("#x", 0) == 0)
            {
                oToken.eKind = Kind::Numeral;
            }
            oToken.strText = std::move(strLexeme);
            return oToken;
        }

        // Unknown character: consume and report through a symbol token that
        // the parser will reject with context.
        advance();
        oToken.eKind = Kind::Symbol;
        oToken.strText = std::string(1, ch);
        return oToken;
    }

private:
    void advance()
    {
        if (m_strText[m_stPos] == '\n')
        {
            ++m_iLine;
            m_iColumn = 1;
        }
        else
        {
            ++m_iColumn;
        }
        ++m_stPos;
    }

    void skipSpaces()
    {
        while (m_stPos < m_strText.size())
        {
            const char ch = m_strText[m_stPos];
            if (ch == ';')
            {
                while (m_stPos < m_strText.size() && m_strText[m_stPos] != '\n')
                {
                    advance();
                }
                continue;
            }
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
            {
                advance();
                continue;
            }
            break;
        }
    }

    [[nodiscard]] Token lexString()
    {
        advance(); // opening quote
        Token oToken;
        oToken.eKind = Kind::String;
        while (m_stPos < m_strText.size())
        {
            const char ch = m_strText[m_stPos];
            if (ch == '"')
            {
                if (m_stPos + 1 < m_strText.size() && m_strText[m_stPos + 1] == '"')
                {
                    oToken.strText += '"';
                    advance();
                    advance();
                    continue;
                }
                advance();
                return oToken;
            }
            oToken.strText += ch;
            advance();
        }

        oToken.strText.insert(0, "unterminated string: ");
        oToken.eKind = Kind::Symbol; // becomes an error at parse level
        return oToken;
    }

    [[nodiscard]] static bool isSymbolChar(char ch)
    {
        return std::isalnum(static_cast<unsigned char>(ch)) != 0
            || ch == '_' || ch == '-' || ch == '.' || ch == '#' || ch == '/'
            || ch == '*' || ch == '+' || ch == '<' || ch == '>' || ch == '=';
    }

    const std::string& m_strText;
    std::size_t m_stPos = 0;
    int m_iLine = 1;
    int m_iColumn = 1;
};

class Parser
{
public:
    Parser(Lexer oLexer, StorageError* pError)
        : m_oLexer(std::move(oLexer))
        , m_pError(pError)
    {
    }

    // Reads commands until End. Returns false on error.
    [[nodiscard]] bool run(Problem& oProblem)
    {
        while (true)
        {
            const Lexer::Token oToken = m_oLexer.next();
            if (oToken.eKind == Lexer::Kind::End)
            {
                return true;
            }
            if (oToken.eKind != Lexer::Kind::LParen)
            {
                return fail(oToken.location, "Expected '(' at command start");
            }

            if (!parseCommand(oProblem))
            {
                return false;
            }
        }
    }

private:
    // Not nodiscard on purpose: expression contexts report via failExpr and
    // ignore the boolean result.
    bool fail(SourceLocation oLoc, std::string strMessage)
    {
        if (m_pError != nullptr && m_pError->message.empty())
        {
            m_pError->kind = StorageErrorKind::Format;
            m_pError->message = "Line " + std::to_string(oLoc.iLine) + ": " + strMessage;
        }
        return false;
    }

    // Expression-context variant: reports and yields an empty optional.
    std::optional<Expression> failExpr(SourceLocation oLoc, std::string strMessage)
    {
        fail(std::move(oLoc), std::move(strMessage));
        return std::nullopt;
    }

    [[nodiscard]] Lexer::Token peek()
    {
        if (!m_oHasPending)
        {
            m_oPending = m_oLexer.next();
            m_oHasPending = true;
        }
        return m_oPending;
    }

    [[nodiscard]] Lexer::Token take()
    {
        if (m_oHasPending)
        {
            m_oHasPending = false;
            return m_oPending;
        }
        return m_oLexer.next();
    }

    void skipList()
    {
        int iDepth = 1; // the opening paren was consumed by the caller path
        while (iDepth > 0)
        {
            const Lexer::Token oToken = take();
            if (oToken.eKind == Lexer::Kind::End)
            {
                return;
            }
            if (oToken.eKind == Lexer::Kind::LParen)
            {
                ++iDepth;
            }
            else if (oToken.eKind == Lexer::Kind::RParen)
            {
                --iDepth;
            }
        }
    }

    [[nodiscard]] bool parseCommand(Problem& oProblem)
    {
        const Lexer::Token oHead = take();
        if (oHead.eKind != Lexer::Kind::Symbol)
        {
            return fail(oHead.location, "Expected a command name");
        }

        if (oHead.strText == "declare-const")
        {
            const Lexer::Token oName = take();
            if (oName.eKind != Lexer::Kind::Symbol)
            {
                return fail(oName.location, "declare-const: expected a name");
            }

            Variable oVariable;
            oVariable.name = oName.strText;
            if (!parseSort(oVariable))
            {
                return false;
            }

            if (!oProblem.addVariable(std::move(oVariable)))
            {
                return fail(oName.location,
                    "Duplicate or invalid variable \"" + oName.strText + "\"");
            }

            const Lexer::Token oClose = take();
            if (oClose.eKind != Lexer::Kind::RParen)
            {
                return fail(oClose.location, "declare-const: expected ')'");
            }
            return true;
        }

        if (oHead.strText == "assert")
        {
            std::optional<Expression> oExpr = parseExpr();
            if (!oExpr.has_value())
            {
                return false; // error already reported
            }

            Constraint oConstraint;
            oConstraint.expr = std::move(*oExpr);

            const Lexer::Token oClose = take();
            if (oClose.eKind != Lexer::Kind::RParen)
            {
                return fail(oClose.location, "assert: expected ')'");
            }

            [[maybe_unused]] const bool bAdded = oProblem.addConstraint(std::move(oConstraint));
            return true;
        }

        // check-sat / get-model / set-info / ... : skip to the closing paren.
        skipList();
        return true;
    }

    [[nodiscard]] bool parseSort(Variable& oVariable)
    {
        const Lexer::Token oSort = take();

        if (oSort.eKind == Lexer::Kind::Symbol)
        {
            if (oSort.strText == "Bool")
            {
                oVariable.type = VariableType::Bool;
                return true;
            }
            if (oSort.strText == "Int")
            {
                oVariable.type = VariableType::Int;
                return true;
            }
            if (oSort.strText == "Real")
            {
                oVariable.type = VariableType::Real;
                return true;
            }
            if (oSort.strText == "String")
            {
                oVariable.type = VariableType::String;
                return true;
            }
            return fail(oSort.location, "Unsupported sort \"" + oSort.strText + "\"");
        }

        // (_ BitVec N): opening paren, underscore, tag, width, closing paren.
        if (oSort.eKind == Lexer::Kind::LParen)
        {
            const Lexer::Token oUnderscore = take();
            if (oUnderscore.eKind != Lexer::Kind::Symbol || oUnderscore.strText != "_")
            {
                return fail(oUnderscore.location, "Expected '_' after '('");
            }

            const Lexer::Token oTag = take();
            if (oTag.eKind != Lexer::Kind::Symbol || oTag.strText != "BitVec")
            {
                return fail(oTag.location,
                    "Unsupported sort \"(" + oUnderscore.strText + " " + oTag.strText + ")\"");
            }

            const Lexer::Token oWidth = take();
            if (oWidth.eKind != Lexer::Kind::Numeral)
            {
                return fail(oWidth.location, "Expected BitVec width");
            }
            const unsigned long long uWidth =
                std::strtoull(oWidth.strText.c_str(), nullptr, 10);
            if (uWidth == 0 || uWidth > k_uMaxBvWidth)
            {
                return fail(oWidth.location, "BitVec width out of supported range");
            }
            oVariable.type = VariableType::BitVec;
            oVariable.params.uBitVecWidth = static_cast<unsigned>(uWidth);

            const Lexer::Token oClose = take();
            if (oClose.eKind != Lexer::Kind::RParen)
            {
                return fail(oClose.location, "Expected ')' after BitVec width");
            }
            return true;
        }

        return fail(oSort.location, "Expected a sort name");
    }

    [[nodiscard]] std::optional<Expression> parseExpr()
    {
        const Lexer::Token oToken = take();

        switch (oToken.eKind)
        {
            case Lexer::Kind::Numeral:
                return parseNumeral(oToken);

            case Lexer::Kind::String:
                return Expression::stringValue(oToken.strText);

            case Lexer::Kind::Symbol:
            {
                if (oToken.strText == "true")
                {
                    return Expression::boolean(true);
                }
                if (oToken.strText == "false")
                {
                    return Expression::boolean(false);
                }
                // A bare symbol is a variable reference; unknown names are
                // rejected later by the normal validation pipeline.
                return Expression::variable(VariableId{}, oToken.strText);
            }

            case Lexer::Kind::LParen:
                break;

            default:
                (void)fail(oToken.location, "Unexpected token in expression");
                return std::nullopt;
        }

        // Compound expression: operator head + operands.
        const Lexer::Token oHead = take();
        if (oHead.eKind != Lexer::Kind::Symbol)
        {
            return failExpr(oHead.location, "Expected an operator");
        }

        // Indexed literals: (_ bvN W).
        if (oHead.strText == "_")
        {
            const Lexer::Token oBv = take();
            if (oBv.strText.rfind("bv", 0) != 0)
            {
                return failExpr(oBv.location, "Expected an indexed literal");
            }
            const Lexer::Token oWidth = take();
            if (oWidth.eKind != Lexer::Kind::Numeral)
            {
                return failExpr(oWidth.location, "Expected bit-vector width");
            }
            const unsigned long long uWidth = std::strtoull(oWidth.strText.c_str(), nullptr, 10);
            if (uWidth == 0 || uWidth > k_uMaxBvWidth)
            {
                return failExpr(oWidth.location, "BitVec width out of supported range");
            }

            BitVecValue oValue;
            oValue.uWidth = static_cast<unsigned>(uWidth);
            oValue.uBits = std::strtoull(oBv.strText.c_str() + 2, nullptr, 10);

            const Lexer::Token oClose = take();
            if (oClose.eKind != Lexer::Kind::RParen)
            {
                return failExpr(oClose.location, "Expected ')' after indexed literal");
            }
            return Expression::bitVector(oValue);
        }

        struct OpMapping
        {
            BinaryOp eOp;
            int iArity;
        };
        static const std::unordered_map<std::string, OpMapping> s_mapBinary = {
            {"and", {BinaryOp::And, 2}},   {"or", {BinaryOp::Or, 2}},
            {"=", {BinaryOp::Eq, 2}},      {"distinct", {BinaryOp::Neq, 2}},
            {"<", {BinaryOp::Lt, 2}},      {"<=", {BinaryOp::Le, 2}},
            {">", {BinaryOp::Gt, 2}},      {">=", {BinaryOp::Ge, 2}},
            {"+", {BinaryOp::Add, 2}},     {"-", {BinaryOp::Sub, 2}},
            {"*", {BinaryOp::Mul, 2}},     {"/", {BinaryOp::Div, 2}},
            {"div", {BinaryOp::Div, 2}},   {"mod", {BinaryOp::Rem, 2}},
            {"bvand", {BinaryOp::BvAnd, 2}}, {"bvor", {BinaryOp::BvOr, 2}},
            {"bvxor", {BinaryOp::BvXor, 2}}, {"bvshl", {BinaryOp::BvShl, 2}},
            {"bvlshr", {BinaryOp::BvShr, 2}}, {"bvudiv", {BinaryOp::Div, 2}},
            {"bvurem", {BinaryOp::Rem, 2}},
        };

        if (oHead.strText == "not")
        {
            std::optional<Expression> oOperand = parseExpr();
            if (!oOperand.has_value())
            {
                return std::nullopt;
            }
            const Lexer::Token oClose = take();
            if (oClose.eKind != Lexer::Kind::RParen)
            {
                return failExpr(oClose.location, "not: expected ')'");
            }
            return Expression::unary(UnaryOp::Not, std::move(*oOperand));
        }
        if (oHead.strText == "-" || oHead.strText == "bvneg")
        {
            std::optional<Expression> oOperand = parseExpr();
            if (!oOperand.has_value())
            {
                return std::nullopt;
            }
            const Lexer::Token oClose = take();
            if (oClose.eKind != Lexer::Kind::RParen)
            {
                return failExpr(oClose.location, "Unary '-': expected ')'");
            }
            // One negation op for all sorts; the DSL printer/analyzer pair
            // resolves Int vs Real vs BitVec on re-parse.
            return Expression::unary(UnaryOp::Neg, std::move(*oOperand));
        }
        if (oHead.strText == "bvnot")
        {
            std::optional<Expression> oOperand = parseExpr();
            if (!oOperand.has_value())
            {
                return std::nullopt;
            }
            const Lexer::Token oClose = take();
            if (oClose.eKind != Lexer::Kind::RParen)
            {
                return failExpr(oClose.location, "bvnot: expected ')'");
            }
            return Expression::unary(UnaryOp::BvNot, std::move(*oOperand));
        }

        const auto itFound = s_mapBinary.find(oHead.strText);
        if (itFound == s_mapBinary.end())
        {
            return failExpr(oHead.location, "Unsupported operator \"" + oHead.strText + "\"");
        }

        std::optional<Expression> oLeft = parseExpr();
        if (!oLeft.has_value())
        {
            return std::nullopt;
        }
        std::optional<Expression> oRight = parseExpr();
        if (!oRight.has_value())
        {
            return std::nullopt;
        }

        const Lexer::Token oClose = take();
        if (oClose.eKind != Lexer::Kind::RParen)
        {
            return failExpr(oClose.location, "Operator " + oHead.strText + ": expected ')'");
        }

        return Expression::binary(itFound->second.eOp,
            std::move(*oLeft), std::move(*oRight));
    }

    [[nodiscard]] static std::optional<Expression> parseNumeral(const Lexer::Token& oToken)
    {
        if (oToken.strText.rfind("#x", 0) == 0)
        {
            BitVecValue oValue;
            oValue.uBits = std::strtoull(oToken.strText.c_str() + 2, nullptr, 16);
            // Width from hex digit count, rounded up to a nibble boundary.
            oValue.uWidth = static_cast<unsigned>((oToken.strText.size() - 2) * 4);
            if (oValue.uWidth == 0 || oValue.uWidth > k_uMaxBvWidth)
            {
                return Expression::integer(0);
            }
            return Expression::bitVector(oValue);
        }

        if (oToken.strText.find('.') != std::string::npos
            || oToken.strText.find('/') != std::string::npos)
        {
            return Expression::real(oToken.strText);
        }

        return Expression::integer(
            static_cast<std::int64_t>(std::strtoll(oToken.strText.c_str(), nullptr, 10)));
    }

    Lexer m_oLexer;
    StorageError* m_pError;
    Lexer::Token m_oPending;
    bool m_oHasPending = false;
};

} // namespace

std::optional<Problem> SmtLib2Reader::read(const std::string& strText,
    std::string strName, StorageError* pError) const
{
    if (pError != nullptr)
    {
        *pError = StorageError{};
    }

    Problem oProblem(std::move(strName));

    Parser oParser(Lexer(strText), pError);
    if (!oParser.run(oProblem))
    {
        return std::nullopt;
    }

    return oProblem;
}

} // namespace z3wb
