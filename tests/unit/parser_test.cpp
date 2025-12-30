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
// Created by mikai on 30.12.2025.
//

#include <gtest/gtest.h>
#include <variant>
#include <vector>
#include <string>

#include "src/parser/parser.h"
#include "../../src/parser/ast.h"
#include "src/parser/lexer.h"

class ParserTest : public ::testing::Test {
protected:
    static std::vector<Statement> parseSQL(const std::string& sql) {
        Lexer lexer(sql);
        Parser parser(lexer);
        return parser.parse();
    }
};

TEST_F(ParserTest, ParseSimpleSelect) {
    const auto statements = parseSQL("SELECT name, age FROM users;");

    ASSERT_EQ(statements.size(), 1);
    const auto* selectStmt = std::get_if<SelectStmt>(&statements[0]);
    ASSERT_NE(selectStmt, nullptr) << "Expected a SelectStmt";

    ASSERT_EQ(selectStmt->projections.size(), 2);
    const auto* colRef1 = std::get_if<ColumnRef>(&selectStmt->projections[0]);
    ASSERT_NE(colRef1, nullptr);
    EXPECT_EQ(colRef1->name, "name");

    ASSERT_EQ(selectStmt->from.size(), 1);
    EXPECT_EQ(selectStmt->from[0].name, "users");
}

TEST_F(ParserTest, ParseExpressionPrecedence) {
    const auto statements = parseSQL("SELECT 1 + 2 * 3;");

    const auto* selectStmt = std::get_if<SelectStmt>(&statements[0]);
    const auto& expr = selectStmt->projections[0];

    // The root operation should be PLUS (precedence 4)
    const auto *binOp = std::get_if<std::unique_ptr<BinaryOp>>(&expr);
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->get()->op, BinaryOp::Op::PLUS);

    // The right operand should be a MUL operation (precedence 5)
    const auto *rightOp = std::get_if<std::unique_ptr<BinaryOp>>(&(binOp->get()->right));
    ASSERT_NE(rightOp, nullptr);
    EXPECT_EQ(rightOp->get()->op, BinaryOp::Op::MUL);
}

TEST_F(ParserTest, ParseParentheses) {
    const auto statements = parseSQL("SELECT (1 + 2) * 3;");

    const auto* selectStmt = std::get_if<SelectStmt>(&statements[0]);
    const auto& expr = selectStmt->projections[0];

    // The root operation should be MUL
    const auto *binOp = std::get_if<std::unique_ptr<BinaryOp>>(&expr);
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->get()->op, BinaryOp::Op::MUL);

    // The left operand should be a PLUS operation
    const auto *leftOp = std::get_if<std::unique_ptr<BinaryOp>>(&(binOp->get()->left));
    ASSERT_NE(leftOp, nullptr);
    EXPECT_EQ(leftOp->get()->op, BinaryOp::Op::PLUS);
}

TEST_F(ParserTest, ParseWhereClause) {
    const auto statements = parseSQL("SELECT * FROM users WHERE age = 30;");

    const auto* selectStmt = std::get_if<SelectStmt>(&statements[0]);
    ASSERT_NE(selectStmt->where, std::nullopt);
    ASSERT_EQ(selectStmt->where.has_value(), true);

    const auto& whereExpr = selectStmt->where.value();
    const auto *binOp = std::get_if<std::unique_ptr<BinaryOp>>(&whereExpr);
    ASSERT_NE(binOp, nullptr);
    EXPECT_EQ(binOp->get()->op, BinaryOp::Op::EQ);
}

TEST_F(ParserTest, ThrowsOnInvalidSyntax) {
    EXPECT_THROW({
        parseSQL("SELECT * FROM;");
    }, std::runtime_error);

    EXPECT_THROW({
        parseSQL("SELECT (1 + 2;");
    }, std::runtime_error);
}