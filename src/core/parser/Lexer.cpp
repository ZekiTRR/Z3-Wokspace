#include "core/parser/Lexer.hpp"

namespace z3wb {

namespace {

constexpr bool isAsciiDigit(char ch) noexcept
{
    return ch >= '0' && ch <= '9';
}

constexpr bool isAsciiHexDigit(char ch) noexcept
{
    return isAsciiDigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

constexpr bool isIdentifierStart(char ch) noexcept
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

constexpr bool isIdentifierPart(char ch) noexcept
{
    return isIdentifierStart(ch) || isAsciiDigit(ch);
}

class Scanner
{
public:
    Scanner(std::string_view svSource, std::vector<Diagnostic>& vecDiags)
        : m_svSource(svSource)
        , m_vecDiags(vecDiags)
    {
    }

    [[nodiscard]] LexResult run()
    {
        LexResult oResult;
        // The diagnostics vector is filled by this scanner; tokens are built
        // locally and moved out at the end.
        std::vector<Token> vecTokens;
        m_vecDiags.clear();

        while (!atEnd())
        {
            skipTrivia();
            if (atEnd())
            {
                break;
            }

            SourceLocation oStart = currentLocation();
            std::optional<Token> oToken = scanToken(oStart);
            if (oToken.has_value())
            {
                vecTokens.push_back(std::move(*oToken));
            }
        }

        Token oEof;
        oEof.type = TokenType::Eof;
        oEof.location = currentLocation();
        vecTokens.push_back(std::move(oEof));

        oResult.tokens = std::move(vecTokens);
        oResult.diagnostics = std::move(m_vecDiags);
        return oResult;
    }

private:
    [[nodiscard]] bool atEnd() const noexcept { return m_stPos >= m_svSource.size(); }

    [[nodiscard]] char cur() const noexcept { return m_svSource[m_stPos]; }

    [[nodiscard]] char peek(std::size_t stOffset) const noexcept
    {
        const std::size_t stIndex = m_stPos + stOffset;
        return stIndex < m_svSource.size() ? m_svSource[stIndex] : '\0';
    }

    void advance()
    {
        if (cur() == '\n')
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

    [[nodiscard]] SourceLocation currentLocation() const
    {
        SourceLocation oLoc;
        oLoc.iLine = m_iLine;
        oLoc.iColumn = m_iColumn;
        return oLoc;
    }

    void error(const SourceLocation& oLoc, std::string strMessage)
    {
        Diagnostic oDiagnostic;
        oDiagnostic.severity = DiagnosticSeverity::Error;
        oDiagnostic.message = std::move(strMessage);
        oDiagnostic.location = oLoc;
        m_vecDiags.push_back(std::move(oDiagnostic));
    }

    static Token makeToken(TokenType eType, std::string strText, const SourceLocation& oLoc)
    {
        Token oToken;
        oToken.type = eType;
        oToken.text = std::move(strText);
        oToken.location = oLoc;
        return oToken;
    }

    void skipTrivia()
    {
        while (!atEnd())
        {
            const char ch = cur();
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
            {
                advance();
                continue;
            }

            if (ch == '/' && peek(1) == '/')
            {
                while (!atEnd() && cur() != '\n')
                {
                    advance();
                }
                continue;
            }

            if (ch == '/' && peek(1) == '*')
            {
                const SourceLocation oStart = currentLocation();
                advance();
                advance();
                bool bTerminated = false;
                while (!atEnd())
                {
                    if (cur() == '*' && peek(1) == '/')
                    {
                        advance();
                        advance();
                        bTerminated = true;
                        break;
                    }
                    advance();
                }
                if (!bTerminated)
                {
                    error(oStart, "Unterminated block comment");
                }
                continue;
            }

            break;
        }
    }

    [[nodiscard]] std::optional<Token> scanToken(const SourceLocation& oStart)
    {
        const char ch = cur();

        if (isIdentifierStart(ch))
        {
            return scanIdentifierOrKeyword(oStart);
        }

        if (isAsciiDigit(ch))
        {
            return scanNumber(oStart);
        }

        if (ch == '"')
        {
            return scanString(oStart);
        }

        switch (ch)
        {
            case ':':
                advance();
                return makeToken(TokenType::Colon, ":", oStart);
            case '(':
                advance();
                return makeToken(TokenType::LParen, "(", oStart);
            case ')':
                advance();
                return makeToken(TokenType::RParen, ")", oStart);

            case '|':
                if (peek(1) == '|')
                {
                    advance();
                    advance();
                    return makeToken(TokenType::OpOr, "||", oStart);
                }
                advance();
                return makeToken(TokenType::OpPipe, "|", oStart);

            case '&':
                if (peek(1) == '&')
                {
                    advance();
                    advance();
                    return makeToken(TokenType::OpAnd, "&&", oStart);
                }
                advance();
                return makeToken(TokenType::OpAmp, "&", oStart);

            case '=':
                if (peek(1) == '=')
                {
                    advance();
                    advance();
                    return makeToken(TokenType::OpEq, "==", oStart);
                }
                error(oStart, "Unexpected character '=' (did you mean '=='?)");
                advance();
                return std::nullopt;

            case '!':
                if (peek(1) == '=')
                {
                    advance();
                    advance();
                    return makeToken(TokenType::OpNeq, "!=", oStart);
                }
                advance();
                return makeToken(TokenType::OpNot, "!", oStart);

            case '<':
                if (peek(1) == '<')
                {
                    advance();
                    advance();
                    return makeToken(TokenType::OpShl, "<<", oStart);
                }
                if (peek(1) == '=')
                {
                    advance();
                    advance();
                    return makeToken(TokenType::OpLe, "<=", oStart);
                }
                advance();
                return makeToken(TokenType::OpLt, "<", oStart);

            case '>':
                if (peek(1) == '>')
                {
                    advance();
                    advance();
                    return makeToken(TokenType::OpShr, ">>", oStart);
                }
                if (peek(1) == '=')
                {
                    advance();
                    advance();
                    return makeToken(TokenType::OpGe, ">=", oStart);
                }
                advance();
                return makeToken(TokenType::OpGt, ">", oStart);

            case '+':
                advance();
                return makeToken(TokenType::OpPlus, "+", oStart);
            case '-':
                advance();
                return makeToken(TokenType::OpMinus, "-", oStart);
            case '*':
                advance();
                return makeToken(TokenType::OpStar, "*", oStart);
            case '/':
                advance();
                return makeToken(TokenType::OpSlash, "/", oStart);
            case '%':
                advance();
                return makeToken(TokenType::OpPercent, "%", oStart);
            case '^':
                advance();
                return makeToken(TokenType::OpCaret, "^", oStart);
            case '~':
                advance();
                return makeToken(TokenType::OpTilde, "~", oStart);

            default:
                error(oStart, "Unexpected character '" + std::string(1, ch) + "'");
                advance();
                return std::nullopt;
        }
    }

    [[nodiscard]] std::optional<Token> scanIdentifierOrKeyword(const SourceLocation& oStart)
    {
        const std::size_t stStart = m_stPos;
        while (!atEnd() && isIdentifierPart(cur()))
        {
            advance();
        }

        const std::string_view svText = m_svSource.substr(stStart, m_stPos - stStart);

        TokenType eType = TokenType::Identifier;
        if (svText == "var")
        {
            eType = TokenType::KwVar;
        }
        else if (svText == "constraint")
        {
            eType = TokenType::KwConstraint;
        }
        else if (svText == "true")
        {
            eType = TokenType::KwTrue;
        }
        else if (svText == "false")
        {
            eType = TokenType::KwFalse;
        }
        else if (svText == "Bool")
        {
            eType = TokenType::TyBool;
        }
        else if (svText == "Int")
        {
            eType = TokenType::TyInt;
        }
        else if (svText == "Real")
        {
            eType = TokenType::TyReal;
        }
        else if (svText == "String")
        {
            eType = TokenType::TyString;
        }
        else if (svText == "BitVec")
        {
            eType = TokenType::TyBitVec;
        }

        return makeToken(eType, std::string(svText), oStart);
    }

    void consumeDigits(bool (*pfnIsDigit)(char))
    {
        while (!atEnd() && pfnIsDigit(cur()))
        {
            advance();
        }
    }

    // Rejects trailing identifier characters glued to a number ("123abc") and
    // consumes them so the parser does not cascade the same problem.
    void rejectTrailingGarbage(const SourceLocation& oStart, std::string_view svKindName)
    {
        if (!atEnd() && (isIdentifierStart(cur()) || isAsciiDigit(cur())))
        {
            while (!atEnd() && (isIdentifierPart(cur())))
            {
                advance();
            }
            error(oStart, "Malformed " + std::string(svKindName) + " literal");
        }
    }

    [[nodiscard]] std::optional<Token> scanNumber(const SourceLocation& oStart)
    {
        const std::size_t stStart = m_stPos;

        if (cur() == '0' && (peek(1) == 'x' || peek(1) == 'X'))
        {
            advance();
            advance();
            consumeDigits(isAsciiHexDigit);
            if (m_stPos - stStart == 2)
            {
                error(oStart, "Hexadecimal literal has no digits");
                rejectTrailingGarbage(oStart, "hexadecimal");
                return std::nullopt;
            }
            const std::size_t stNumericEnd = m_stPos;
            rejectTrailingGarbage(oStart, "hexadecimal");
            return makeToken(TokenType::IntLiteral,
                std::string(m_svSource.substr(stStart, stNumericEnd - stStart)), oStart);
        }

        if (cur() == '0' && (peek(1) == 'b' || peek(1) == 'B'))
        {
            advance();
            advance();
            consumeDigits([](char ch) noexcept { return ch == '0' || ch == '1'; });
            if (m_stPos - stStart == 2)
            {
                error(oStart, "Binary literal has no digits");
                rejectTrailingGarbage(oStart, "binary");
                return std::nullopt;
            }
            const std::size_t stNumericEnd = m_stPos;
            rejectTrailingGarbage(oStart, "binary");
            return makeToken(TokenType::IntLiteral,
                std::string(m_svSource.substr(stStart, stNumericEnd - stStart)), oStart);
        }

        consumeDigits(isAsciiDigit);

        bool bReal = false;
        if (cur() == '.')
        {
            if (!isAsciiDigit(peek(1)))
            {
                error(oStart, "Malformed real literal (missing digits after '.')");
                rejectTrailingGarbage(oStart, "real");
                return std::nullopt;
            }
            bReal = true;
            advance();
            consumeDigits(isAsciiDigit);
        }

        const std::size_t stNumericEnd = m_stPos;
        rejectTrailingGarbage(oStart, bReal ? "real" : "number");

        return makeToken(bReal ? TokenType::RealLiteral : TokenType::IntLiteral,
            std::string(m_svSource.substr(stStart, stNumericEnd - stStart)), oStart);
    }

    [[nodiscard]] std::optional<Token> scanString(const SourceLocation& oStart)
    {
        advance(); // opening quote

        std::string strValue;
        while (!atEnd() && cur() != '"')
        {
            char ch = cur();
            if (ch == '\n')
            {
                break; // strings must not span lines; reported as unterminated
            }

            if (ch == '\\')
            {
                advance();
                if (atEnd())
                {
                    break;
                }
                switch (cur())
                {
                    case '\\':
                        strValue += '\\';
                        break;
                    case '"':
                        strValue += '"';
                        break;
                    case 'n':
                        strValue += '\n';
                        break;
                    case 't':
                        strValue += '\t';
                        break;
                    default:
                        error(currentLocation(), "Unknown escape sequence '\\" + std::string(1, cur()) + "'");
                        break;
                }
                advance();
                continue;
            }

            strValue += ch;
            advance();
        }

        if (atEnd() || cur() != '"')
        {
            error(oStart, "Unterminated string literal");
            return std::nullopt;
        }
        advance(); // closing quote

        return makeToken(TokenType::StringLiteral, std::move(strValue), oStart);
    }

    std::string_view m_svSource;
    std::vector<Diagnostic>& m_vecDiags;
    std::size_t m_stPos = 0;
    int m_iLine = 1;
    int m_iColumn = 1;
};

} // namespace

bool LexResult::hasErrors() const noexcept
{
    for (const Diagnostic& oDiagnostic : diagnostics)
    {
        if (oDiagnostic.severity == DiagnosticSeverity::Error)
        {
            return true;
        }
    }
    return false;
}

LexResult lex(std::string_view svSource)
{
    std::vector<Diagnostic> vecDiags;
    Scanner oScanner(svSource, vecDiags);
    return oScanner.run();
}

} // namespace z3wb
