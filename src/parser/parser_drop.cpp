//
// Created by mikai on 03.01.2026.
//

#include "parser.h"


DropStmt Parser::parse_drop_stmt() {
    DropStmt stmt;

    // Determine object type
    switch (current().type) {
        case TokenType::TABLE: stmt.object_type = ObjectType::TABLE; break;
        case TokenType::VIEW: stmt.object_type = ObjectType::VIEW; break;
        case TokenType::INDEX: stmt.object_type = ObjectType::INDEX; break;
        case TokenType::SCHEMA: stmt.object_type = ObjectType::SCHEMA; break;
        case TokenType::TRIGGER: stmt.object_type = ObjectType::TRIGGER; break;
        case TokenType::SEQUENCE: stmt.object_type = ObjectType::SEQUENCE; break;
        case TokenType::COLLATION: stmt.object_type = ObjectType::COLLATION; break;
        case TokenType::DATABASE: stmt.object_type = ObjectType::DATABASE; break;
        case TokenType::USER: stmt.object_type = ObjectType::USER; break;
        case TokenType::TYPE: stmt.object_type = ObjectType::TYPE; break;
        default:
            throw std::runtime_error("Unknown object type in DROP statement at line " +
                std::to_string(current().line) + ", column " +
                std::to_string(current().column));
    }
    advance(); // Consume the object type keyword

    if (stmt.object_type == ObjectType::INDEX || match(TokenType::CONCURRENTLY)) {
        stmt.concurrently = true;
    }

    // Parse optional IF EXISTS
    if (match(TokenType::IF)) {
        expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after IF in DROP statement"));
        stmt.if_exists = true;
    }

    // Parse object names
    do {
        const Token name_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected object name in DROP statement"));
        stmt.names.push_back(name_token.literal);
    } while (match(TokenType::COMMA));

    // Parse optional CASCADE or RESTRICT
    if (match(TokenType::CASCADE)) {
        stmt.cascade = true;
    } else if (match(TokenType::RESTRICT)) {
        stmt.restrict = true;
    }
    return stmt;
}