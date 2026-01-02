//
// Created by mikai on 03.01.2026.
//

#include "parser.h"

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