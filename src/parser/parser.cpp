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

static BinaryOp::Op token_to_binop(TokenType type) {
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
    // Placeholder for INSERT, CREATE TABLE, etc.
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
            stmt.projections.push_back(ColumnRef{"*", std::nullopt});
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

Expression Parser::parse_expression(const int precedence) {
    // 1. Parse the left-hand side (primary expression)
    Expression left = parse_primary();

    // 2. Precedence Climbing
    while (true) {
        const Token token = current();
        const int tok_precedence = get_precedence(token.type);

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
