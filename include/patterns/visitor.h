#pragma once
/// @file visitor.h
/// @brief Modern Visitor pattern using std::variant + std::visit
///
/// Replaces the classic double-dispatch visitor with a type-safe, closed set of
/// alternatives and overloaded lambdas.

#include <string>
#include <variant>
#include <vector>

namespace patterns {

// ── Overload helper (C++17 idiom for visiting variants) ─────────────

/// @brief Aggregate that inherits from all supplied callables, exposing each `operator()`.
/// @tparam Ts Callable types to combine.
template <typename... Ts>
struct Overload : Ts... { using Ts::operator()...; };

/// @brief Deduction guide for Overload.
template <typename... Ts>
Overload(Ts...) -> Overload<Ts...>;

// ── AST node types (example domain) ────────────────────────────────

/// @brief A numeric literal expression node.
struct LiteralExpr {
    double value; ///< The literal numeric value.
};

/// @brief Forward declaration of a binary operator expression node.
struct BinaryExpr;

/// @brief Forward declaration of a unary operator expression node.
struct UnaryExpr;

using Expr = std::variant<
    LiteralExpr,
    std::unique_ptr<BinaryExpr>,
    std::unique_ptr<UnaryExpr>
>;

/// @brief A binary operator expression (e.g. `a + b`).
struct BinaryExpr {
    char op;   ///< The operator character (+, -, *, /).
    Expr lhs;  ///< Left-hand side expression.
    Expr rhs;  ///< Right-hand side expression.
};

/// @brief A unary operator expression (e.g. `-x`).
struct UnaryExpr {
    char op;      ///< The operator character.
    Expr operand; ///< The operand expression.
};

// ── Visitor: evaluate an expression tree ────────────────────────────

/// @brief Recursively evaluate an expression tree to a numeric result.
/// @param expr The expression variant to evaluate.
/// @return The computed double value.
inline double evaluate(const Expr& expr) {
    return std::visit(Overload{
        [](const LiteralExpr& e) { return e.value; },
        [](const std::unique_ptr<BinaryExpr>& e) -> double {
            double l = evaluate(e->lhs);
            double r = evaluate(e->rhs);
            switch (e->op) {
                case '+': return l + r;
                case '-': return l - r;
                case '*': return l * r;
                case '/': return r != 0.0 ? l / r : 0.0;
                default:  return 0.0;
            }
        },
        [](const std::unique_ptr<UnaryExpr>& e) -> double {
            double v = evaluate(e->operand);
            return e->op == '-' ? -v : v;
        },
    }, expr);
}

// ── Visitor: pretty-print an expression tree ────────────────────────

/// @brief Recursively convert an expression tree to a parenthesized string.
/// @param expr The expression variant to stringify.
/// @return A human-readable string representation.
inline std::string to_string(const Expr& expr) {
    return std::visit(Overload{
        [](const LiteralExpr& e) { return std::to_string(e.value); },
        [](const std::unique_ptr<BinaryExpr>& e) -> std::string {
            return "(" + to_string(e->lhs) + " " + e->op + " " + to_string(e->rhs) + ")";
        },
        [](const std::unique_ptr<UnaryExpr>& e) -> std::string {
            return std::string(1, e->op) + to_string(e->operand);
        },
    }, expr);
}

// ── Helper factories ────────────────────────────────────────────────

/// @brief Create a literal expression.
/// @param v The numeric value.
/// @return An Expr containing a LiteralExpr.
inline Expr lit(double v) { return LiteralExpr{v}; }

/// @brief Create a binary expression.
/// @param op The operator character (+, -, *, /).
/// @param lhs Left-hand side expression.
/// @param rhs Right-hand side expression.
/// @return An Expr containing a BinaryExpr.
inline Expr bin(char op, Expr lhs, Expr rhs) {
    return std::make_unique<BinaryExpr>(BinaryExpr{op, std::move(lhs), std::move(rhs)});
}

/// @brief Create a negation (unary minus) expression.
/// @param e The expression to negate.
/// @return An Expr containing a UnaryExpr with op '-'.
inline Expr neg(Expr e) {
    return std::make_unique<UnaryExpr>(UnaryExpr{'-', std::move(e)});
}

}  // namespace patterns
