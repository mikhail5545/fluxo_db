/*
 fluxo_db in-memory database
 Copyright (C) 2025 Mikhail Kulik

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

//
// Created by mikai on 27.12.2025.
//

#ifndef FLUXO_DB_AST_H
#define FLUXO_DB_AST_H

#pragma once
#include <memory>
#include <variant>
#include <vector>
#include <optional>
#include <string>

// Forward declarations
struct ColumnRef;
struct LiteralValue;
struct BinaryOp;
struct UnaryOp;
struct FunctionCall;
struct CastExpr;

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

struct ColumnRef {
    std::string name;
    std::optional<std::string> table_name; // Optional table name
};

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

// --- Statements ---

struct SelectStmt;
struct InsertStmt;
struct CreateTableStmt;

using Statement = std::variant<
    SelectStmt,
    InsertStmt,
    CreateTableStmt
>;

struct ColumnDef {
    std::string name;
    DataType type;
    bool not_null = false;
    bool primary_key = false;
    bool unique = false;
};

struct TableRef {
    std::string name;
    std::optional<std::string> alias;
};

struct SelectStmt {
    std::vector<Expr> projections; // SELECT clause
    std::vector<TableRef> from; // FROM clause
    std::optional<Expr> where; // WHERE clause
    std::optional<Expr> having;
    std::vector<Expr> group_by;
    std::vector<std::pair<Expr, bool>> order_by; // expr, asc
    std::optional<int64_t> limit;
    std::optional<int64_t> offset;
    bool distinct = false;
};

struct InsertStmt {
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::vector<Expr>> values; // Multiple rows support
};

struct CreateTableStmt {
    std::string table_name;
    std::vector<ColumnDef> columns;
    bool if_not_exists = false;
};

#endif //FLUXO_DB_AST_H