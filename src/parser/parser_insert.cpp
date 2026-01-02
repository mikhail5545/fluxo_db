//
// Created by mikai on 03.01.2026.
//

#include "parser.h"

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