#include "core/parser/AST.hpp"

#include <charconv>
#include <limits>

namespace z3wb::ast {

namespace {

// Binding powers, low to high. Bitwise operators bind tighter than
// comparisons (unlike C), so RE-style constraints read naturally:
//   x ^ 0x1337 == 0x4242   parses as   (x ^ 0x1337) == 0x4242
struct BinaryOpInfo
{
    BinaryOp eOp;
    int iPrecedence;
};

constexpr int k_iPrefixPrecedence = 9;

[[nodiscard]] std::optional<BinaryOpInfo> binaryOpFor(TokenType eType)
{
    switch (eType)
    {
        case TokenType::OpOr:
            return BinaryOpInfo{BinaryOp::Or, 1};
        case TokenType::OpAnd:
            return BinaryOpInfo{BinaryOp::And, 2};
        case TokenType::OpEq:
            return BinaryOpInfo{BinaryOp::Eq, 3};
        case TokenType::OpNeq:
            return BinaryOpInfo{BinaryOp::Neq, 3};
        case TokenType::OpLt:
            return BinaryOpInfo{BinaryOp::Lt, 4};
        case TokenType::OpLe:
            return BinaryOpInfo{BinaryOp::Le, 4};
        case TokenType::OpGt:
            return BinaryOpInfo{BinaryOp::Gt, 4};
        case TokenType::OpGe:
            return BinaryOpInfo{BinaryOp::Ge, 4};
        case TokenType::OpAmp:
            return BinaryOpInfo{BinaryOp::BvAnd, 5};
        case TokenType::OpPipe:
            return BinaryOpInfo{BinaryOp::BvOr, 5};
        case TokenType::OpCaret:
            return BinaryOpInfo{BinaryOp::BvXor, 5};
        case TokenType::OpShl:
            return BinaryOpInfo{BinaryOp::BvShl, 6};
        case TokenType::OpShr:
            return BinaryOpInfo{BinaryOp::BvShr, 6};
        case TokenType::OpPlus:
            return BinaryOpInfo{BinaryOp::Add, 7};
        case TokenType::OpMinus:
            return BinaryOpInfo{BinaryOp::Sub, 7};
        case TokenType::OpStar:
            return BinaryOpInfo{BinaryOp::Mul, 8};
        case TokenType::OpSlash:
            return BinaryOpInfo{BinaryOp::Div, 8};
        case TokenType::OpPercent:
            return BinaryOpInfo{BinaryOp::Rem, 8};
        default:
            return std::nullopt;
    }
}

class Parser
{
public:
    Parser(std::span<const Token> vecTokens, std::vector<Diagnostic>& vecDiags)
        : m_vecTokens(vecTokens)
        , m_vecDiags(vecDiags)
    {
    }

    [[nodiscard]] std::vector<Statement> parseProgram()
    {
        m_vecDiags.clear();
        std::vector<Statement> vecStatements;

        while (!check(TokenType::Eof))
        {
            const std::size_t stBefore = m_stCurrent;
            std::optional<Statement> oStatement = parseStatement();
            if (oStatement.has_value())
            {
                vecStatements.push_back(std::move(*oStatement));
            }

            if (m_stCurrent == stBefore)
            {
                // Safety net against non-progressing error paths.
                advance();
            }
        }

        return vecStatements;
    }

private:
    [[nodiscard]] const Token& peek() const noexcept { return m_vecTokens[m_stCurrent]; }

    void advance() noexcept
    {
        if (!check(TokenType::Eof))
        {
            ++m_stCurrent;
        }
    }

    [[nodiscard]] bool check(TokenType eType) const noexcept { return peek().type == eType; }

    [[nodiscard]] bool match(TokenType eType)
    {
        if (!check(eType))
        {
            return false;
        }
        advance();
        return true;
    }

    void errorAt(const Token& oToken, std::string strMessage)
    {
        Diagnostic oDiagnostic;
        oDiagnostic.severity = DiagnosticSeverity::Error;
        oDiagnostic.message = std::move(strMessage);
        oDiagnostic.location = oToken.location;
        m_vecDiags.push_back(std::move(oDiagnostic));
    }

    [[nodiscard]] static std::string describeToken(const Token& oToken)
    {
        switch (oToken.type)
        {
            case TokenType::Eof:
                return "end of input";
            case TokenType::Identifier:
            case TokenType::IntLiteral:
            case TokenType::RealLiteral:
                return "'" + oToken.text + "'";
            case TokenType::StringLiteral:
                return "string literal";
            default:
                return "'" + oToken.text + "'";
        }
    }

    // Consumes the expected token and returns it; otherwise reports the given
    // message without consuming (callers decide how to recover).
    [[nodiscard]] const Token* expect(TokenType eType, std::string_view svWhat)
    {
        if (check(eType))
        {
            const Token* pToken = &peek();
            advance();
            return pToken;
        }

        errorAt(peek(), std::string(svWhat) + ", found " + describeToken(peek()));
        return nullptr;
    }

    void synchronize()
    {
        while (!check(TokenType::Eof))
        {
            if (check(TokenType::KwVar) || check(TokenType::KwConstraint))
            {
                return;
            }
            advance();
        }
    }

    [[nodiscard]] std::optional<Statement> parseStatement()
    {
        if (check(TokenType::KwVar))
        {
            VarDecl oDecl = parseVarDecl();
            synchronizeIfBroken(oDecl.name.empty());
            if (oDecl.name.empty())
            {
                return std::nullopt;
            }
            return Statement{std::move(oDecl)};
        }

        if (check(TokenType::KwConstraint))
        {
            return Statement{parseConstraintDecl()};
        }

        errorAt(peek(), "Expected 'var' or 'constraint', found " + describeToken(peek()));
        synchronize();
        return std::nullopt;
    }

    // After a failed declaration, skip to the next statement keyword.
    void synchronizeIfBroken(bool bBroken)
    {
        if (bBroken)
        {
            synchronize();
        }
    }

    [[nodiscard]] VarDecl parseVarDecl()
    {
        const Token oKeyword = peek();
        advance(); // 'var'

        VarDecl oDecl;
        oDecl.location = oKeyword.location;

        const Token* pName = expect(TokenType::Identifier, "Expected variable name");
        if (pName == nullptr || !isValidVariableName(pName->text))
        {
            if (pName != nullptr && !isValidVariableName(pName->text))
            {
                errorAt(*pName, "Invalid variable name '" + pName->text + "'");
            }
            return VarDecl{}; // empty name marks failure
        }
        oDecl.name = pName->text;

        if (expect(TokenType::Colon, "Expected ':' after variable name") == nullptr)
        {
            return VarDecl{};
        }

        auto oType = parseType();
        if (!oType.has_value())
        {
            return VarDecl{};
        }
        oDecl.type = oType->first;
        oDecl.params = oType->second;

        return oDecl;
    }

    [[nodiscard]] std::optional<std::pair<VariableType, TypeParams>> parseType()
    {
        if (match(TokenType::TyBool))
        {
            return std::make_pair(VariableType::Bool, TypeParams{});
        }
        if (match(TokenType::TyInt))
        {
            return std::make_pair(VariableType::Int, TypeParams{});
        }
        if (match(TokenType::TyReal))
        {
            return std::make_pair(VariableType::Real, TypeParams{});
        }
        if (match(TokenType::TyString))
        {
            return std::make_pair(VariableType::String, TypeParams{});
        }

        if (match(TokenType::TyBitVec))
        {
            if (expect(TokenType::LParen, "Expected '(' after 'BitVec'") == nullptr)
            {
                return std::nullopt;
            }

            const Token* pWidth = expect(TokenType::IntLiteral, "Expected bit width");
            if (pWidth == nullptr)
            {
                return std::nullopt;
            }

            const std::uint64_t uWidth = parseUnsigned(*pWidth);
            if (uWidth == 0 || uWidth > 64)
            {
                errorAt(*pWidth, "BitVec width must be between 1 and 64");
                return std::nullopt;
            }

            if (expect(TokenType::RParen, "Expected ')' after bit width") == nullptr)
            {
                return std::nullopt;
            }

            TypeParams oParams;
            oParams.uBitVecWidth = static_cast<unsigned>(uWidth);
            return std::make_pair(VariableType::BitVec, oParams);
        }

        errorAt(peek(), "Expected a type (Bool, Int, Real, String, BitVec), found "
            + describeToken(peek()));
        return std::nullopt;
    }

    [[nodiscard]] ConstraintDecl parseConstraintDecl()
    {
        const Token oKeyword = peek();
        advance(); // 'constraint'

        ConstraintDecl oDecl;
        oDecl.location = oKeyword.location;
        // parsePrimary always returns an expression (with a diagnostic on
        // failure), so this function cannot fail silently.
        oDecl.expr = parseExpression(0);
        return oDecl;
    }

    [[nodiscard]] Expression parseExpression(int iMinPrecedence)
    {
        Expression oLhs = parseUnary();

        while (true)
        {
            const std::optional<BinaryOpInfo> oInfo = binaryOpFor(peek().type);
            if (!oInfo.has_value() || oInfo->iPrecedence < iMinPrecedence)
            {
                break;
            }

            advance();
            Expression oRhs = parseExpression(oInfo->iPrecedence + 1); // left-associative
            oLhs = Expression::binary(oInfo->eOp, std::move(oLhs), std::move(oRhs));
        }

        return oLhs;
    }

    [[nodiscard]] Expression parseUnary()
    {
        std::optional<UnaryOp> eOp;
        switch (peek().type)
        {
            case TokenType::OpMinus:
                eOp = UnaryOp::Neg;
                break;
            case TokenType::OpNot:
                eOp = UnaryOp::Not;
                break;
            case TokenType::OpTilde:
                eOp = UnaryOp::BvNot;
                break;
            default:
                break;
        }

        if (!eOp.has_value())
        {
            return parsePrimary();
        }

        advance();
        Expression oOperand = parseExpression(k_iPrefixPrecedence);
        return Expression::unary(*eOp, std::move(oOperand));
    }

    [[nodiscard]] Expression parsePrimary()
    {
        const Token& oToken = peek();

        switch (oToken.type)
        {
            case TokenType::IntLiteral:
                advance();
                return Expression::integer(parseSigned(oToken));

            case TokenType::RealLiteral:
                advance();
                return Expression::real(oToken.text);

            case TokenType::StringLiteral:
                advance();
                return Expression::stringValue(oToken.text);

            case TokenType::KwTrue:
                advance();
                return Expression::boolean(true);

            case TokenType::KwFalse:
                advance();
                return Expression::boolean(false);

            case TokenType::Identifier:
                advance();
                // Id stays invalid here; semantic analysis resolves it.
                return Expression::variable(VariableId{}, oToken.text);

            case TokenType::LParen:
            {
                advance();
                Expression oInner = parseExpression(0);
                if (!match(TokenType::RParen))
                {
                    errorAt(peek(), "Expected ')', found " + describeToken(peek()));
                }
                return oInner;
            }

            default:
                errorAt(oToken, "Unexpected token " + describeToken(oToken));
                advance();
                return Expression::integer(0);
        }
    }

    [[nodiscard]] std::int64_t parseSigned(const Token& oToken)
    {
        const std::string_view svDigits = stripBasePrefix(oToken.text);

        std::uint64_t uValue = 0;
        const auto oConvert = std::from_chars(
            svDigits.data(), svDigits.data() + svDigits.size(), uValue, detectBase(oToken.text));
        if (oConvert.ec != std::errc{} || uValue > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        {
            errorAt(oToken, "Integer literal '" + oToken.text + "' is out of range");
            return 0;
        }
        return static_cast<std::int64_t>(uValue);
    }

    [[nodiscard]] std::uint64_t parseUnsigned(const Token& oToken)
    {
        const std::string_view svDigits = stripBasePrefix(oToken.text);
        std::uint64_t uValue = 0;
        const auto oConvert = std::from_chars(
            svDigits.data(), svDigits.data() + svDigits.size(), uValue, detectBase(oToken.text));
        if (oConvert.ec != std::errc{})
        {
            errorAt(oToken, "Number literal '" + oToken.text + "' is out of range");
            return 0;
        }
        return uValue;
    }

    [[nodiscard]] static int detectBase(std::string_view svText) noexcept
    {
        if (svText.size() >= 2 && svText[0] == '0' && (svText[1] == 'x' || svText[1] == 'X'))
        {
            return 16;
        }
        if (svText.size() >= 2 && svText[0] == '0' && (svText[1] == 'b' || svText[1] == 'B'))
        {
            return 2;
        }
        return 10;
    }

    // std::from_chars does not consume radix prefixes, so "0x1337" must be
    // stripped before conversion.
    [[nodiscard]] static std::string_view stripBasePrefix(std::string_view svText) noexcept
    {
        if (detectBase(svText) != 10)
        {
            return svText.substr(2);
        }
        return svText;
    }

    std::span<const Token> m_vecTokens;
    std::vector<Diagnostic>& m_vecDiags;
    std::size_t m_stCurrent = 0;
};

} // namespace

bool ParseOutput::hasErrors() const noexcept
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

ParseOutput parse(std::span<const Token> vecTokens)
{
    std::vector<Diagnostic> vecDiags;
    Parser oParser(vecTokens, vecDiags);

    ParseOutput oOutput;
    oOutput.statements = oParser.parseProgram();
    oOutput.diagnostics = std::move(vecDiags);
    return oOutput;
}

} // namespace z3wb::ast
