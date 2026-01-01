//
// Created by mikai on 01.01.2026.
//

#ifndef FLUXO_DB_AST_EXPR_H
#define FLUXO_DB_AST_EXPR_H

#include <memory>
#include <variant>
#include <vector>
#include <optional>

struct ColumnRef;
struct LiteralValue;
struct BinaryOp;
struct UnaryOp;
struct FunctionCall;
struct CastExpr;


struct TableRef {
    std::string name;
    std::optional<std::string> alias;
};

struct ColumnRef {
    std::string name;
    std::optional<std::string> table_name; // Optional table name
};

enum class DataType {
    INTEGER, BIGINT, TEXT, BOOLEAN, DOUBLE, DATE, TIMESTAMP, VARCHAR, NULL_TYPE
};

struct LiteralValue {
    DataType type = DataType::NULL_TYPE; // NULL by default

    // Only store the active value
    std::variant<std::monostate, int64_t, double, bool, std::string> value;

    static LiteralValue Integer(int64_t v) {
        return { DataType::INTEGER, v };
    }
    static LiteralValue BigInt(int64_t v) {
        return { DataType::BIGINT, v };
    }
    static LiteralValue Double(double v) {
        return { DataType::DOUBLE, v };
    }
    static LiteralValue Boolean(bool v) {
        return { DataType::BOOLEAN, v };
    }
    static LiteralValue Text(const std::string& v) {
        return { DataType::TEXT, v };
    }
    static LiteralValue Null() {
        return { DataType::NULL_TYPE, std::monostate{} };
    }
};

struct Expression;
using Expr = std::variant<
    std::monostate, // Represents an empty expression (NULL or empty)
    ColumnRef,
    LiteralValue,
    std::unique_ptr<BinaryOp>,
    std::unique_ptr<UnaryOp>,
    std::unique_ptr<FunctionCall>,
    std::unique_ptr<CastExpr>
>;
// Helper alias
using ExprPtr = std::unique_ptr<Expr>;

struct  BinaryOp {
    enum Op {
        PLUS, MINUS, MUL, DIV, EQ, NEQ, MOD, LT, LTE,
        GT, GTE, AND, OR, LIKE, ILIKE, NOT_LIKE
    } op;
    Expr left;
    Expr right;
};

struct UnaryOp {
    enum Op { NOT, IS_NULL, IS_NOT_NULL, MINUS } op;
    ExprPtr operand;
};

struct FunctionCall {
    std::string name; // "upper", "coalesce", "now", etc.
    std::vector<Expr> args;
    bool is_aggregate = false; // true for aggregate functions like SUM, COUNT, etc.
};

struct CastExpr {
    ExprPtr expr;
    DataType target_type;
};

struct Expression : Expr {
    using Expr::Expr; // Inherit constructors
};

struct ColumnDef {
    std::string name;
    DataType type;
    bool not_null = false;
    bool primary_key = false;
    bool unique = false;
};

#endif //FLUXO_DB_AST_EXPR_H