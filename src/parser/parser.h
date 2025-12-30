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

#ifndef FLUXO_DB_PARSER_H
#define FLUXO_DB_PARSER_H
#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>

class Parser {
private:
    Lexer &lexer_;
    Token current_token_;

    std::vector<Token> tokens;
    size_t position = 0;

    [[nodiscard]] Token current() const;
    [[nodiscard]] Token peek(size_t offset = 1) const;
    [[nodiscard]] bool is_end() const;

    Token advance();
    Token expect(TokenType type, const std::string& error_msg);

    bool match(TokenType type);

    // Parsing methods
    Statement parse_statement();
    SelectStmt parse_select_stmt();
    InsertStmt parse_insert_stmt();
    CreateTableStmt parse_create_table_stmt();

    Expression parse_expression(int precedence = 0);
    Expression parse_primary();
public:
    explicit Parser(Lexer &lexer);
    std::vector<Statement> parse();
};

#endif //FLUXO_DB_PARSER_H
