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
#include "parser.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

static int get_precedence(const TokenType type) {
    switch (type) {
        case TokenType::ASTERISK:
        case TokenType::SLASH:
        case TokenType::PERCENT:
            return 5;
        case TokenType::PLUS:
        case TokenType::MINUS:
            return 4;
        case TokenType::EQUALS:
        case TokenType::CARET:
            return 3;
        case TokenType::UNKNOWN:
            return 2;
        default:
            return 0;
    }
}

static BinaryOp::Op token_to_binop(const TokenType type) {
    switch (type) {
        case TokenType::PLUS:
            return BinaryOp::Op::PLUS;
        case TokenType::MINUS:
            return BinaryOp::Op::MINUS;
        case TokenType::ASTERISK:
            return BinaryOp::Op::MUL;
        case TokenType::SLASH:
            return BinaryOp::Op::DIV;
        case TokenType::EQUALS:
            return BinaryOp::Op::EQ;
        case TokenType::PERCENT:
            return BinaryOp::Op::MOD;
        default:
            throw std::runtime_error("Unknown binary operator token");
    }
}

static DataType token_to_data_type(const Token& token) {
    if (token.type == TokenType::IDENTIFIER) {
        std::string type_name = token.literal;
        std::ranges::transform(type_name, type_name.begin(), ::toupper);

        if (type_name == "INT" || type_name == "INTEGER") {
            return DataType::INTEGER;
        }
        if (type_name == "BIGINT") {
            return DataType::BIGINT;
        }
        if (type_name == "DOUBLE" || type_name == "FLOAT" || type_name == "REAL") {
            return DataType::DOUBLE;
        }
        if (type_name == "TEXT") {
            return DataType::TEXT;
        }
        if (type_name == "VARCHAR") {
            return DataType::VARCHAR;
        }
        if (type_name == "BOOLEAN" || type_name == "BOOL") {
            return DataType::BOOLEAN;
        }
        if (type_name == "DATE") {
            return DataType::DATE;
        }
    }
    throw std::runtime_error("Unknown data type: " + token.literal + "at line: " +
        std::to_string(token.line) + ", column: " +
        std::to_string(token.column));
}

Parser::Parser(Lexer &lexer) : lexer_(lexer) {
    // Tokenize the entire input upfront
    Token token = lexer_.NextToken();
    while (token.type != TokenType::EOF_TOKEN) {
        tokens.push_back(token);
        token = lexer_.NextToken();
    }
    tokens.push_back(token); // Add EOF token
    current_token_ = tokens[0];
}

// Current token without advancing
Token Parser::current() const {
    if (position < tokens.size()) { // Check boundaries
        return tokens[position];
    }
    return Token{TokenType::EOF_TOKEN, "", -1, -1};
}

// Advance to the next token and return the current one
Token Parser::advance() {
    if (position < tokens.size()) { // Check boundaries
        return tokens[position++];
    }
    return Token{TokenType::EOF_TOKEN, "", -1, -1};
}

// Peek at a token ahead without advancing
Token Parser::peek(const size_t offset) const {
    if (const size_t peekPosition = position + offset; peekPosition < tokens.size()) { // Check boundaries
        return tokens[peekPosition];
    }
    return Token{TokenType::EOF_TOKEN, "", -1, -1};
}

// Match the current token type and advance if it matches
bool Parser::match(const TokenType type) {
    if (current().type == type) {
        advance();
        return true;
    }
    return false;
}

// Expect a specific token type, throw an error if it doesn't match
Token Parser::expect(const TokenType type, const std::string& error_msg) {
    if (match(type)) {
        return tokens[position - 1]; // Return the matched token
    }
    throw std::runtime_error(error_msg + " at line " +
        std::to_string(current().line) + ", column " +
        std::to_string(current().column));
}

// Check if we've reached the end of the token stream
bool Parser::is_end() const {
    return current().type == TokenType::EOF_TOKEN;
}

std::vector<Statement> Parser::parse() {
    std::vector<Statement> statements;
    while (!is_end()) {
        statements.push_back(parse_statement());
        match(TokenType::SEMICOLON);
    }
    return statements;
}

Statement Parser::parse_statement() {
    if (match(TokenType::SELECT)) {
        return parse_select_stmt();
    }
    if (match(TokenType::INSERT)) {
        return parse_insert_stmt();
    }
    if (match(TokenType::CREATE)) {
        return parse_create_table_stmt();
    }
    throw std::runtime_error("Unsupported statement type at line " +
        std::to_string(current().line) + ", column " +
        std::to_string(current().column));
}

SelectStmt Parser::parse_select_stmt() {
    SelectStmt stmt;

    // 1. Parse projections
    do {
        if (match(TokenType::ASTERISK)) {
            // Handle wildcard *
            stmt.projections.emplace_back(ColumnRef{"*", std::nullopt});
        } else {
            stmt.projections.push_back(parse_expression());
        }
    } while (match(TokenType::COMMA));

    // 2. Parse FROM clause
    if (match(TokenType::FROM)) {
        do {
            const Token table_token = expect(TokenType::IDENTIFIER, "Expected table name after FROM");
            TableRef table_ref{table_token.literal, std::nullopt};
            stmt.from.push_back(table_ref);
        } while (match(TokenType::COMMA));
    }

    // 3. Parse WHERE clause
    if (match(TokenType::WHERE)) {
        stmt.where = parse_expression();
    }

    return stmt;
}

InsertStmt Parser::parse_insert_stmt() {
    InsertStmt stmt;

    // Expect: INTO table_name
    expect(TokenType::INSERT, "Expected INSERT keyword at line " +
        std::to_string(current().line) + ", column " +
        std::to_string(current().column));
    const Token table_token = expect(TokenType::IDENTIFIER, "Expected table name after INSERT");
    stmt.table_name = table_token.literal;

    // Expect: (column1, column2, ...)
    if (match(TokenType::LPAREN)) {
        do {
            Token col = expect(TokenType::IDENTIFIER, "Expected column name in INSERT");
            stmt.columns.push_back(col.literal);
        } while (match(TokenType::COMMA));
        expect(TokenType::VALUES, "Expected VALUES keyword after column list in INSERT");
    }
    // Parse list of values: (1, 'a'), (2, 'b'), ...
    do {
        expect(TokenType::LPAREN, "Expected '(' before values list");
        std::vector<Expr> value_row;
        do {
            value_row.push_back(parse_expression());
        } while (match(TokenType::COMMA));
        expect(TokenType::RPAREN, "Expected ')' after values list");
        stmt.values.push_back(std::move(value_row));
    } while (match(TokenType::COMMA));
    return stmt;
}

CreateTableStmt Parser::parse_create_table_stmt() {
    CreateTableStmt stmt;

    expect(TokenType::TABLE, "Expected TABLE keyword after CREATE");
    const Token table_token = expect(TokenType::IDENTIFIER, "Expected table name after CREATE TABLE");
    stmt.table_name = table_token.literal;

    expect(TokenType::LPAREN, "Expected '(' after table name in CREATE TABLE");

    do {
        ColumnDef col;
        const Token name = expect(TokenType::IDENTIFIER, "Expected column name in CREATE TABLE");
        col.name = name.literal;

        const Token type_token = advance();
        col.type = token_to_data_type(type_token);

        // Parse column constraints
        // Continue looping as long as we don't hit a comma (next column) or closing parenthesis
        while (current().type != TokenType::COMMA && current().type != TokenType::RPAREN) {
            if (match(TokenType::PRIMARY)) {
                expect(TokenType::KEY, "Expected KEY after PRIMARY in column definition");
                col.primary_key = true;
            } else if (match(TokenType::NOT)) {
               if (current().type == TokenType::NULL_TYPE) {
                   advance(); // Consume NULL
                   col.not_null = true;
               } else if (current().type == TokenType::IDENTIFIER && current().literal == "NULL") {
                   advance(); // Consume NULL
                   col.not_null = true;
               } else {
                   throw std::runtime_error("Expected NULL after NOT in column definition at line " +
                       std::to_string(current().line) + ", column " +
                       std::to_string(current().column));
               }
            }else if (match(TokenType::UNIQUE)) {
                col.unique = true;
            } else {
                throw std::runtime_error("Unknown column constraint at line " +
                    std::to_string(current().line) + ", column " +
                    std::to_string(current().column));
            }
        }

        stmt.columns.push_back(std::move(col));
    } while (match(TokenType::COMMA));

    expect(TokenType::RPAREN, "Expected ')' after column definitions in CREATE TABLE");
    return stmt;
}

Expression Parser::parse_expression(const int precedence) {
    // 1. Parse the left-hand side (primary expression)
    Expression left = parse_primary();

    // 2. Precedence Climbing
    while (true) {
        const Token token = current();
        const int tok_precedence = get_precedence(token.type);
        std::cout<< "Current token: " << token.literal << " with precedence " << tok_precedence << "\n";
        std::cout << "Current token type: " << static_cast<int>(token.type) << "\n";

        // If next token is not an operator or has lower precedence, stop
        if (tok_precedence <= precedence) {
            break;
        }

        // Consume the operator
        advance();
        const BinaryOp::Op op = token_to_binop(token.type);

        // Parse the right-hand side expression
        Expression right = parse_expression(tok_precedence);

        // Create a new BinaryOp node
        auto binOp = std::make_unique<BinaryOp>();
        binOp->op = op;
        binOp->left = std::move(left);
        binOp->right = std::move(right);

        // The result becomes the new 'left' for the next iteration
        left = std::move(binOp);
    }
    return left;
}

Expression Parser::parse_primary() {
    switch (const auto [type, literal, line, column] = current(); type) {
        case TokenType::IDENTIFIER: {
            advance();
            // Identifiers are ColumnRefs
            return ColumnRef{literal, std::nullopt};
        }
        case TokenType::NUMBER: {
            advance();
            // Simple heuristic: if it contains a dot, it's a double
            if (literal.find('.') != std::string::npos) {
                return LiteralValue{DataType::DOUBLE, std::stod(literal)};
            } else {
                return LiteralValue{DataType::INTEGER, std::stoi(literal)};
            }
        }
        case TokenType::STRING: {
            advance();
            return LiteralValue{DataType::TEXT, literal};
        }
        case TokenType::LPAREN: {
            advance();
            Expression expr = parse_expression();
            expect(TokenType::RPAREN, "Expected ')'");
            return expr;
        }
        default:
            throw std::runtime_error("Unknown expression token " + literal + " at line " +
                std::to_string(line) + ", column " +
                std::to_string(column));
    }
}
