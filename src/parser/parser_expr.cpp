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
// Created by mikai on 03.01.2026.
//

#include "parser.h"

#include <iostream>

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
            }
            return LiteralValue{DataType::INTEGER, std::stoi(literal)};
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