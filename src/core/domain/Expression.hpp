#pragma once

#include "core/domain/Ids.hpp"
#include "core/domain/Values.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace z3wb {

// -----------------------------------------------------------------------------
// Operators are stored in resolved form: the semantic analyzer maps source
// tokens to concrete ops, so converters never re-disambiguate. BitVec
// comparisons, division and shifts use unsigned semantics (RE-oriented
// choice); signed variants can be added as separate ops later.
// -----------------------------------------------------------------------------
enum class UnaryOp
{
    Not,    // logical ! (Bool)
    Neg,    // arithmetic - (Int, Real)
    BvNot,  // bitwise ~ (BitVec)
};

enum class BinaryOp
{
    And,    // &&   Bool
    Or,     // ||   Bool
    Eq,     // ==
    Neq,    // !=
    Lt,     // <    Int/Real arith, BitVec unsigned
    Le,     // <=
    Gt,     // >
    Ge,     // >=
    Add,    // +    Int/Real/Bv
    Sub,    // -
    Mul,    // *
    Div,    // /    Int truncating, Real rational, BitVec unsigned
    Rem,    // %    Int remainder, BitVec unsigned remainder
    BvAnd,  // &    BitVec
    BvOr,   // |
    BvXor,  // ^
    BvShl,  // <<
    BvShr,  // >>   BitVec logical shift right
};

[[nodiscard]] std::string_view toString(UnaryOp eOp);
[[nodiscard]] std::string_view toString(BinaryOp eOp);

struct Constant
{
    enum class Kind
    {
        Bool,
        Int,
        Real,
        String,
        BitVec,
    };

    Kind eKind = Kind::Int;
    // Int -> int64; Real -> decimal literal as written (rationals must not
    // pass through binary floating point); String -> UTF-8; BitVec -> bits.
    std::variant<bool, std::int64_t, std::string, BitVecValue> oData{std::int64_t{0}};
};

struct VariableRef
{
    VariableId id;
    std::string strName;
};

class Expression;

struct UnaryExpression
{
    UnaryOp eOp = UnaryOp::Neg;
    // Children are shared immutable subtrees: expressions are copied often
    // while being built, and nodes never mutate after creation.
    std::shared_ptr<const Expression> spOperand;
};

struct BinaryExpression
{
    BinaryOp eOp = BinaryOp::Add;
    std::shared_ptr<const Expression> spLhs;
    std::shared_ptr<const Expression> spRhs;
};

struct FunctionCall
{
    std::string strName;
    std::vector<Expression> vecArgs;
};

// -----------------------------------------------------------------------------
// Immutable expression tree, solver-agnostic by design.
// -----------------------------------------------------------------------------
class Expression
{
public:
    using Node = std::variant<Constant, VariableRef, UnaryExpression, BinaryExpression, FunctionCall>;

    // Default expression: integer constant 0 (keeps the type default-constructible).
    Expression();

    [[nodiscard]] static Expression boolean(bool bValue);
    [[nodiscard]] static Expression integer(std::int64_t iValue);
    [[nodiscard]] static Expression real(std::string strDecimalLiteral);
    [[nodiscard]] static Expression stringValue(std::string strValue);
    [[nodiscard]] static Expression bitVector(BitVecValue oValue);
    [[nodiscard]] static Expression variable(VariableId oId, std::string strName);
    [[nodiscard]] static Expression unary(UnaryOp eOp, Expression oOperand);
    [[nodiscard]] static Expression binary(BinaryOp eOp, Expression oLhs, Expression oRhs);
    [[nodiscard]] static Expression call(std::string strName, std::vector<Expression> vecArgs);

    [[nodiscard]] const Node& node() const noexcept { return m_node; }

    template<typename VisitorT>
    decltype(auto) visit(VisitorT&& visitor) const
    {
        return std::visit(std::forward<VisitorT>(visitor), m_node);
    }

private:
    explicit Expression(Node oNode)
        : m_node(std::move(oNode))
    {
    }

    Node m_node;
};

[[nodiscard]] bool operator==(const Expression& oLhs, const Expression& oRhs) noexcept;
[[nodiscard]] bool operator!=(const Expression& oLhs, const Expression& oRhs) noexcept;

// Fully parenthesized canonical form, used for display and exports.
[[nodiscard]] std::string toString(const Expression& oExpr);

using VariableIdMap = std::unordered_map<VariableId, VariableId>;

// Returns a copy of the tree with variable references remapped. Used when
// duplicating problems (fresh ids) and will back future rename operations.
[[nodiscard]] Expression remapVariables(const Expression& oExpr, const VariableIdMap& mapOldToNew);

} // namespace z3wb
