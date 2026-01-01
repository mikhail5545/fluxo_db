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

int64_t Parser::determine_sign() {
    int64_t sign = 1;
    if (match(TokenType::MINUS)) sign = -1;
    return sign;
}

static std::string errMsg(const Token& token, const std::string& msg) {
    return msg + " at line " + std::to_string(token.line) + ", column " + std::to_string(token.column);
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
    if (match(TokenType::DROP)) {
        return parse_drop_stmt();
    }
    if (match(TokenType::ALTER)) {
        return parse_alter_table_stmt();
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

AlterTableStmt Parser::parse_alter_table_stmt() {
    AlterTableStmt stmt;

    expect(TokenType::TABLE, errMsg(current(), "Expected TABLE keyword after ALTER"));

    // Parse optional IF EXISTS
    if (match(TokenType::IF)) {
        expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after IF in ALTER TABLE"));
        stmt.if_exists = true;
    }

    // Parse table name
    const Token table_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected table name after ALTER TABLE"));
    stmt.table_name = table_token.literal;

    do {
        stmt.actions.push_back(parse_alter_table_action());
    } while (match(TokenType::COMMA));

    return stmt;
}

AlterAction Parser::parse_alter_table_action() {
    if (match(TokenType::ADD)) {
        return parse_add_action();
    }
    if (match(TokenType::DROP)) {
        return parse_drop_action();
    }
    if (match(TokenType::ALTER)) {
        return parse_alter_column_action();
    }
    if (match(TokenType::RENAME)) {
        return parse_rename_action();
    }
    if (match(TokenType::SET)) {
        return parse_set_schema_action();
    }
    if (match(TokenType::OWNER)) {
        return parse_owner_to_action();
    }
    throw std::runtime_error("Unknown ALTER TABLE action at line " +
                             std::to_string(current().line) + ", column " +
                             std::to_string(current().column));
}

AddAction Parser::parse_add_action() {
    AddAction add_action;
    if (match(TokenType::COLUMN)) {
        AddColumnAction action;

        // Parse optional IF NOT EXISTS
        if (match(TokenType::IF)) {
            expect(TokenType::NOT, errMsg(current(), "Expected NOT after IF in ADD COLUMN"));
            expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after NOT in ADD COLUMN"));
            action.if_not_exists = true;
        }

        const Token col_name_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name after ADD COLUMN"));
        action.column_def.name = col_name_token.literal;

        const Token type_token = advance();
        action.column_def.type = token_to_data_type(type_token);

        // Parse optional constraints
        while (current().type != TokenType::COMMA && current().type != TokenType::SEMICOLON && current().type != TokenType::EOF_TOKEN) {
            if (match(TokenType::NOT)) {
                expect(TokenType::NULL_TYPE, errMsg(current(), "Expected NULL after NOT in column constraint"));
                action.column_def.not_null = true;
            } else if (match(TokenType::UNIQUE)) {
                action.column_def.unique = true;
            } else if (match(TokenType::PRIMARY)) {
                expect(TokenType::KEY, errMsg(current(), "Expected KEY after PRIMARY in column constraint"));
                action.column_def.primary_key = true;
            } else {
                throw std::runtime_error("Unknown column constraint in ADD COLUMN at line " +
                    std::to_string(current().line) + ", column " +
                    std::to_string(current().column));
            }
        }
        add_action.emplace<AddColumnAction>(action);
    } else if (match(TokenType::CONSTRAINT)) {
        AddConstraintAction action;
        const Token col_name_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name after ADD CONSTRAINT"));
        action.column_name = col_name_token.literal;

        // Parse constraints
        while (current().type != TokenType::COMMA && current().type != TokenType::SEMICOLON && current().type != TokenType::EOF_TOKEN) {
            if (match(TokenType::NOT)) {
                expect(TokenType::NULL_TYPE, errMsg(current(), "Expected NULL after NOT in constraint"));
                action.not_null = true;
            } else if (match(TokenType::UNIQUE)) {
                action.unique = true;
            } else if (match(TokenType::PRIMARY)) {
                expect(TokenType::KEY, errMsg(current(), "Expected KEY after PRIMARY in constraint"));
                action.primary_key = true;
            } else {
                throw std::runtime_error("Unknown constraint in ADD CONSTRAINT at line " +
                    std::to_string(current().line) + ", column " +
                    std::to_string(current().column));
            }
        }
        add_action.emplace<AddConstraintAction>(action);
    } else {
        throw std::runtime_error("Expected COLUMN or CONSTRAINT after ADD in ALTER TABLE at line " +
            std::to_string(current().line) + ", column " +
            std::to_string(current().column));
    }
    return add_action;
}

DropAction Parser::parse_drop_action() {
    DropAction drop_action;
    if (match(TokenType::COLUMN)) {
        DropColumnAction action;

        // Parse optional IF EXISTS
        if (match(TokenType::IF)) {
            expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after IF in DROP COLUMN"));
            action.if_exists = true;
        }

        const Token col_name_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name after DROP COLUMN"));
        action.column_name = col_name_token.literal;

        // Parse optional CASCADE
        if (match(TokenType::CASCADE)) {
            action.cascade = true;
        }

        drop_action.emplace<DropColumnAction>(action);
    } else if (match(TokenType::CONSTRAINT)) {
        DropConstraintAction action;

        // Parse optional IF EXISTS
        if (match(TokenType::IF)) {
            expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after IF in DROP CONSTRAINT"));
            action.if_exists = true;
        }

        const Token constraint_name_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected constraint name after DROP CONSTRAINT"));
        action.constraint_name = constraint_name_token.literal;

        // Parse optional CASCADE
        if (match(TokenType::CASCADE)) {
            action.cascade = true;
        }

        drop_action.emplace<DropConstraintAction>(action);
    } else {
        throw std::runtime_error("Expected COLUMN or CONSTRAINT after DROP in ALTER TABLE at line " +
            std::to_string(current().line) + ", column " +
            std::to_string(current().column));
    }
    return drop_action;
}

AlterColumnAction Parser::parse_alter_column_action() {
    AlterColumnAction alter_column_action;

    expect(TokenType::COLUMN, errMsg(current(), "Expected COLUMN after ALTER in ALTER TABLE"));
    const Token col_name_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name after ALTER COLUMN"));
    const std::string column_name = col_name_token.literal;

    if (match(TokenType::TYPE)) {
        AlterColumnTypeAction action;
        action.column_name = column_name;

        const Token type_token = advance();
        action.new_type = token_to_data_type(type_token);

        // Optional USING expression
        if (match(TokenType::USING)) {
            action.using_expr = parse_expression();
        }

        // Optional COLLATE
        if (match(TokenType::COLLATE)) {
            const Token collation_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected collation name after COLLATE"));
            action.collation = collation_token.literal;
        }

        alter_column_action.emplace<AlterColumnTypeAction>(std::move(action));
    } else if (match(TokenType::SET)) {
        if (match(TokenType::DEFAULT)) {
            AlterColumnDefaultAction action;
            action.column_name = column_name;
            action.default_expr = parse_expression();
            alter_column_action.emplace<AlterColumnDefaultAction>(std::move(action));
        } else if (match(TokenType::NOT)) {
            expect(TokenType::NULL_TYPE, errMsg(current(), "Expected NULL after NOT in ALTER COLUMN"));
            AlterColumnNotNullAction action;
            action.column_name = column_name;
            action.set_not_null = true;
            alter_column_action.emplace<AlterColumnNotNullAction>(std::move(action));
        }
    } else if (match(TokenType::DROP)) {
        if (match(TokenType::DEFAULT)) {
            AlterColumnDefaultAction action;
            action.column_name = column_name;
            action.is_drop = true;
            alter_column_action.emplace<AlterColumnDefaultAction>(std::move(action));
        } else if (match(TokenType::NOT)) {
            expect(TokenType::NULL_TYPE, errMsg(current(), "Expected NULL after NOT in ALTER COLUMN"));
            AlterColumnNotNullAction action;
            action.column_name = column_name;
            action.set_not_null = false;
            alter_column_action.emplace<AlterColumnNotNullAction>(std::move(action));
        }
    }
    return alter_column_action;
}

RenameAction Parser::parse_rename_action() {
    RenameAction rename_action;

    if (match(TokenType::COLUMN)) {
        RenameColumnAction action;
        action.old_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected old column name after RENAME COLUMN")).literal;
        expect(TokenType::TO, errMsg(current(), "Expected TO after old column name in RENAME COLUMN"));
        action.new_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected new column name after TO in RENAME COLUMN")).literal;
        rename_action.emplace<RenameColumnAction>(action);
    } else if (match(TokenType::CONSTRAINT)) {
        RenameConstraintAction action;
        action.old_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected old constraint name after RENAME CONSTRAINT")).literal;
        expect(TokenType::TO, errMsg(current(), "Expected TO after old constraint name in RENAME CONSTRAINT"));
        action.new_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected new constraint name after TO in RENAME CONSTRAINT")).literal;
        rename_action.emplace<RenameConstraintAction>(action);
    } else {
        // Assume RENAME [TO] new_name for table
        RenameTableAction action;
        if(current().type == TokenType::TO) {
            advance();
        }
        action.new_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected new table name after TO in RENAME TABLE")).literal;
        rename_action.emplace<RenameTableAction>(action);
    }
    return rename_action;
}

SetSchemaAction Parser::parse_set_schema_action() {
    SetSchemaAction action;
    expect(TokenType::SCHEMA, errMsg(current(), "Expected SCHEMA after SET in ALTER TABLE"));
    const Token schema_name_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected schema name after SET SCHEMA"));
    action.schema_name = schema_name_token.literal;
    return action;
}

OwnerToAction Parser::parse_owner_to_action() {
    OwnerToAction action;
    expect(TokenType::TO, errMsg(current(), "Expected TO after OWNER in ALTER TABLE"));
    const Token new_owner_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected new owner name after TO in SET OWNER"));
    action.new_owner = new_owner_token.literal;
    return action;
}

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

CreateStmt Parser::parse_create_stmt() {
    // We have already consumed CREATE keyword
    // Peek ahead to skip optional modifiers and find the object type
    size_t offset = 0;

    if (peek(offset).type == TokenType::TEMPORARY) {
        offset++;
    }
    else if (peek(offset).type == TokenType::UNIQUE) {
        offset++;
    }

    switch (peek(offset).type) {
        case TokenType::TABLE:
            return parse_create_table_stmt();
        case TokenType::SEQUENCE:
            return parse_create_sequence_stmt();
        case TokenType::INDEX:
            return parse_create_index_stmt();
        case TokenType::TRIGGER:
            return parse_create_trigger_stmt();
        case TokenType::SCHEMA:
            return parse_create_schema_stmt();
        case TokenType::COLLATION:
            return parse_create_collation_stmt();
        case TokenType::DATABASE:
            return parse_create_database_stmt();
        case TokenType::ROLE:
            return parse_create_role_stmt();
        default:
            throw std::runtime_error("Unknown object type in CREATE statement at line " +
                std::to_string(current().line) + ", column " +
                std::to_string(current().column));
    }
}

ColumnDef Parser::parse_column_def() {
    ColumnDef column_def;

    // Name
    const Token col_name_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name in column definition"));
    column_def.name = col_name_token.literal;

    // Type
    const Token type_token = advance();
    column_def.type = token_to_data_type(type_token);

    // Inline constraints
    while (current().type != TokenType::COMMA && current().type != TokenType::RPAREN) {
        if (match(TokenType::NOT)) {
            expect(TokenType::NULL_TYPE, errMsg(current(), "Expected NULL after NOT in column constraint"));
            column_def.not_null = true;
        } else if (match(TokenType::UNIQUE)) {
            column_def.unique = true;
        } else if (match(TokenType::PRIMARY)) {
            expect(TokenType::KEY, errMsg(current(), "Expected KEY after PRIMARY in column constraint"));
            column_def.primary_key = true;
        } else {
            throw std::runtime_error("Unknown column constraint in column definition at line " +
                std::to_string(current().line) + ", column " +
                std::to_string(current().column));
        }
    }
    return column_def;
}

TableConstraint Parser::parse_table_constraint() {
    TableConstraint constraint;

    // Handle optional "CONSTRAINT <name>"
    if (match(TokenType::CONSTRAINT)) {
        const Token name_token = expect(TokenType::IDENTIFIER, "Expected constraint name after CONSTRAINT");
        constraint.name = name_token.literal;
    }

    // Determine constraint type
    switch (current().type) {
        case TokenType::PRIMARY: {
            advance();
            expect(TokenType::KEY, errMsg(current(), "Expected KEY after PRIMARY in table constraint"));
            constraint.type = TableConstraint::Type::PRIMARY_KEY;

            expect(TokenType::LPAREN, errMsg(current(), "Expected '(' after PRIMARY KEY in table constraint"));
            do {
                constraint.columns.push_back(expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name in PRIMARY KEY constraint")).literal);
            } while (match(TokenType::COMMA));
            expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after column list in PRIMARY KEY constraint"));
            break;
        } case TokenType::UNIQUE: {
            advance();
            constraint.type = TableConstraint::Type::UNIQUE;

            expect(TokenType::LPAREN, errMsg(current(), "Expected '(' after UNIQUE in table constraint"));
            do {
                constraint.columns.push_back(expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name in UNIQUE constraint")).literal);
            } while (match(TokenType::COMMA));
            expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after column list in UNIQUE constraint"));
            break;
        } case TokenType::FOREIGN: {
            advance();
            expect(TokenType::KEY, errMsg(current(), "Expected KEY after FOREIGN in table constraint"));
            constraint.type = TableConstraint::Type::FOREIGN_KEY;

            expect(TokenType::LPAREN, errMsg(current(), "Expected '(' after FOREIGN KEY in table constraint"));
            do {
                constraint.columns.push_back(expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name in FOREIGN KEY constraint")).literal);
            } while (match(TokenType::COMMA));
            expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after column list in FOREIGN KEY constraint"));

            // References
            expect(TokenType::REFERENCES, errMsg(current(), "Expected REFERENCES in FOREIGN KEY constraint"));
            constraint.foreign_table = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected referenced table name in FOREIGN KEY constraint")).literal;

            expect(TokenType::LPAREN, errMsg(current(), "Expected '(' after referenced table name in FOREIGN KEY constraint"));
            do {
                constraint.foreign_columns.push_back(expect(TokenType::IDENTIFIER, errMsg(current(), "Expected referenced column name in FOREIGN KEY constraint")).literal);
            } while (match(TokenType::COMMA));
            expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after referenced column list in FOREIGN KEY constraint"));
            break;
        } case TokenType::CHECK: {
            advance();
            constraint.type = TableConstraint::Type::CHECK;
            expect(TokenType::LPAREN, errMsg(current(), "Expected '(' after CHECK in table constraint"));
            constraint.check_expr = parse_expression();
            expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after CHECK expression in table constraint"));
            break;
        }
        default:
            throw std::runtime_error("Unknown table constraint type at line " +
                std::to_string(current().line) + ", column " +
                std::to_string(current().column));
    }
    return constraint;
}

CreateTableStmt Parser::parse_create_table_stmt() {
    CreateTableStmt stmt;

    expect(TokenType::TABLE, errMsg(current(), "Expected TABLE keyword after CREATE"));

    // Parse optional IF NOT EXISTS
    if (match(TokenType::IF)) {
        expect(TokenType::NOT, errMsg(current(), "Expected NOT after IF in CREATE TABLE"));
        expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after NOT in CREATE TABLE"));
        stmt.if_not_exists = true;
    }

    // Parse table name
    const Token table_name_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected table name after CREATE TABLE"));
    stmt.table_name = table_name_token.literal;

    bool first = true;
    // Loop through comma-separated list of columns or constraints
    while (current().type != TokenType::RPAREN && current().type != TokenType::EOF_TOKEN) {
        if (!first) {
            expect(TokenType::COMMA, errMsg(current(), "Expected ',' between table elements"));
        }
        first = false;

        // Check if this is a table constraint
        if (const TokenType t = current().type;
            t == TokenType::CONSTRAINT ||
            t == TokenType::PRIMARY ||
            t == TokenType::FOREIGN ||
            t == TokenType::CHECK ||
            t == TokenType::UNIQUE) {
            stmt.constraints.push_back(parse_table_constraint());
        } else {
            stmt.columns.push_back(parse_column_def());
        }

        expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after table definition"));
        return stmt;
    }

    // Parse table constraints
    while (current().type != TokenType::RPAREN) {
        stmt.constraints.push_back(parse_table_constraint());
        match(TokenType::COMMA);
    }

    expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after column definitions in CREATE TABLE"));
    return stmt;
}

CreateRoleStmt Parser::parse_create_role_stmt() {
    CreateRoleStmt stmt;

    expect(TokenType::ROLE, errMsg(current(), "Expected ROLE keyword after CREATE"));

    if (match(TokenType::IF)) {
        expect(TokenType::NOT, errMsg(current(), "Expected NOT after IF in CREATE ROLE"));
        expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after NOT in CREATE ROLE"));
        stmt.if_not_exists = true;
    }

    stmt.role_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected role name after CREATE ROLE")).literal;

    if (match(TokenType::WITH)) {
        while (current().type != TokenType::SEMICOLON && current().type != TokenType::EOF_TOKEN) {
            switch (current().type) {
                case TokenType::LOGIN: stmt.login = true; break;
                case TokenType::NO_LOGIN: stmt.login = false; break;
                case TokenType::SUPERUSER: stmt.superuser = true; break;
                case TokenType::NO_SUPERUSER: stmt.superuser = false; break;
                case TokenType::CREATE_DB: stmt.createdb = true; break;
                case TokenType::NO_CREATE_DB: stmt.createdb = false; break;
                case TokenType::CREATE_ROLE: stmt.createrole = true; break;
                case TokenType::NO_CREATE_ROLE: stmt.createrole = false; break;
                case TokenType::INHERIT: stmt.inherit = true; break;
                case TokenType::NO_INHERIT: stmt.inherit = false; break;
                case TokenType::PASSWORD: {
                    advance();
                    if (match(TokenType::NULL_TYPE)) {
                        stmt.password = std::nullopt;
                    } else {
                        const Token pwd_token = expect(TokenType::STRING, errMsg(current(), "Expected password string after PASSWORD in CREATE ROLE"));
                        stmt.password = pwd_token.literal;
                    }
                } case TokenType::CONNECTION: {
                    advance();
                    expect(TokenType::LIMIT, errMsg(current(), "Expected LIMIT after CONNECTION in CREATE ROLE"));

                    const int64_t sign = determine_sign();

                    const Token limit_token = expect(TokenType::NUMBER, errMsg(current(), "Expected number after LIMIT in CREATE ROLE"));
                    const int64_t limit_value = std::stoi(limit_token.literal);
                    if (sign < 0 && limit_value != 1) {
                        throw std::runtime_error("Connection limit cannot be less than -1 in CREATE ROLE at line " +
                            std::to_string(limit_token.line) + ", column " +
                            std::to_string(limit_token.column));
                    }
                    stmt.conn_limit = limit_value * sign;
                    break;
                }
                default:
                    throw std::runtime_error("Unknown option in CREATE ROLE at line " +
                        std::to_string(current().line) + ", column " +
                        std::to_string(current().column));
                    break;
            }
            advance(); // Consume the matched option
        }
    }
    return stmt;
}

CreateCollationStmt Parser::parse_create_collation_stmt() {
    CreateCollationStmt stmt;

    expect(TokenType::COLLATION, errMsg(current(), "Expected COLLATION keyword after CREATE"));

    if (match(TokenType::IF)) {
        expect(TokenType::NOT, errMsg(current(), "Expected NOT after IF in CREATE COLLATION"));
        expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after NOT in CREATE COLLATION"));
        stmt.if_not_exists = true;
    }

    stmt.collation_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected collation name after CRATE COLLATION")).literal;

    if (match(TokenType::FROM)) {
        stmt.existing_collation_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected collation name after FROM in CREATE COLLATION")).literal;
        return stmt;
    }

    if (match(TokenType::LPAREN)) {
        do {
            if (match(TokenType::LOCALE)) {
                expect(TokenType::EQUALS, errMsg(current(), "Expected '=' after LOCALE in CREATE COLLATION"));
                stmt.locale = expect(TokenType::STRING, errMsg(current(), "Expected locale string after '=' in CREATE COLLATION")).literal;
            } else if (match(TokenType::DETERMINISTIC)) {
                expect(TokenType::EQUALS, errMsg(current(), "Expected '=' after DETERMINISTIC in CREATE COLLATION"));

                const Token bool_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected boolean value after '=' in CREATE COLLATION"));
                std::string bool_str = bool_token.literal;
                std::ranges::transform(bool_str, bool_str.begin(), ::toupper);

                if (bool_str == "TRUE") stmt.deterministic = true;
                else if (bool_str == "FALSE") stmt.deterministic = false;
                else throw std::runtime_error("Expected TRUE or FALSE after '=' in CREATE COLLATION at line " +
                        std::to_string(bool_token.line) + ", column " +
                        std::to_string(bool_token.column));
            } else if (match(TokenType::RULES)) {
                expect(TokenType::EQUALS, errMsg(current(), "Expected '=' after RULES in CREATE COLLATION"));
                stmt.rules = expect(TokenType::STRING, errMsg(current(), "Expected rules string after '=' in CREATE COLLATION")).literal;
            } else if (match(TokenType::PROVIDER)) {
                expect(TokenType::EQUALS, errMsg(current(), "Expected '=' after PROVIDER in CREATE COLLATION"));
                stmt.provider = expect(TokenType::STRING, errMsg(current(), "Expected provider string after '=' in CREATE COLLATION")).literal;
            } else {
                throw std::runtime_error("Unknown option in CREATE COLLATION at line " +
                    std::to_string(current().line) + ", column " +
                    std::to_string(current().column));
            }
        } while (match(TokenType::COMMA)); // Continue if there is a comma
    }
    expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after options in CREATE COLLATION"));
    return stmt;
}

CreateDatabaseStmt Parser::parse_create_database_stmt() {
    CreateDatabaseStmt stmt;

    expect(TokenType::DATABASE, errMsg(current(), "Expected DATABASE keyword after CREATE"));

    if (match(TokenType::IF)) {
        expect(TokenType::NOT, errMsg(current(), "Expected NOT after IF in CREATE DATABASE"));
        expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after NOT in CREATE DATABASE"));
        stmt.if_not_exists = true;
    }

    stmt.name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected database name after CREATE DATABASE")).literal;

    // Expected syntax:
    // CREATE DATABASE dbname ( OWNER = owner_name, ENCODING = 'encoding', ALLOW_CONNECTIONS = TRUE/FALSE, CONNECTION_LIMIT = number );
    if (match(TokenType::LPAREN)) {
        do {
            if (match(TokenType::OWNER)) {
                expect(TokenType::EQUALS, errMsg(current(), "Expected '=' after OWNER in CREATE DATABASE"));
                stmt.user_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected owner name after '=' in CREATE DATABASE")).literal;
            } else if (match(TokenType::ENCODING)) {
                expect(TokenType::EQUALS, errMsg(current(), "Expected '=' after ENCODING in CREATE DATABASE"));
                stmt.encoding = expect(TokenType::STRING, errMsg(current(), "Expected encoding string after '=' in CREATE DATABASE")).literal;
            } else if (match(TokenType::ALLOW_CONNECTIONS)) {
                expect(TokenType::EQUALS, errMsg(current(), "Expected '=' after TEMPLATE in CREATE DATABASE"));
                const Token bool_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected template database name after '=' in CREATE DATABASE"));
                std::string bool_str = bool_token.literal;
                std::ranges::transform(bool_str, bool_str.begin(), ::toupper);
                if (bool_str == "TRUE") stmt.allow_conn = true;
                else if (bool_str == "FALSE") stmt.allow_conn = false;
                else throw std::runtime_error("Expected TRUE or FALSE after '=' in CREATE DATABASE at line " +
                        std::to_string(bool_token.line) + ", column " +
                        std::to_string(bool_token.column));
            } else if (match(TokenType::CONNECTION_LIMIT)) {
                expect(TokenType::EQUALS, errMsg(current(), "Expected '=' after CONNECTION LIMIT in CREATE DATABASE"));
                stmt.conn_limit = std::stoi(expect(TokenType::NUMBER, errMsg(current(), "Expected connection limit number after '=' in CREATE DATABASE")).literal);
            } else {
                throw std::runtime_error("Unknown option in CREATE DATABASE at line " +
                    std::to_string(current().line) + ", column " +
                    std::to_string(current().column));
            }
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after options in CREATE DATABASE"));
    return stmt;
}

CreateIndexStmt Parser::parse_create_index_stmt() {
    CreateIndexStmt stmt;

    if (match(TokenType::UNIQUE)) {
        stmt.unique = true;
    }
    expect(TokenType::INDEX, errMsg(current(), "Expected INDEX keyword in CREATE INDEX"));

    if (match(TokenType::CONCURRENTLY)) {
        stmt.concurrently = true;
    }

    if (match(TokenType::IF)) {
        expect(TokenType::NOT, errMsg(current(), "Expected NOT after IF in CREATE INDEX"));
        expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after NOT in CREATE INDEX"));
        stmt.if_not_exists = true;
    }

    stmt.index_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected index name in CREATE INDEX")).literal;

    expect(TokenType::ON, errMsg(current(), "Expected ON keyword in CREATE INDEX"));
    if (match(TokenType::ONLY)) {
        stmt.only = true;
    }
    stmt.table_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected table name in CREATE INDEX")).literal;

    if (match(TokenType::USING)) {
        stmt.method = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected index method name after USING in CREATE INDEX")).literal;
    }

    expect(TokenType::LPAREN, errMsg(current(), "Expected '(' before index columns in CREATE INDEX"));
    do {
        IndexElem elem;

        Expr expr = parse_expression();

        if (std::holds_alternative<ColumnRef>(expr)) {
            elem.name = std::get_if<ColumnRef>(&expr)->name;
        } else {
            elem.expr = std::move(expr);
        }

        if (match(TokenType::COLLATE)) {
            elem.collation = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected collation name after COLLATE in index element")).literal;
        }

        if (current().type == TokenType::IDENTIFIER) {
            elem.op_class = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected operator class name in index element")).literal;
        }

        if (match(TokenType::ASC)) {
            elem.ordering = OrderDirection::ASC;
        } else if (match(TokenType::DESC)) {
            elem.ordering = OrderDirection::DESC;
        }

        if (match(TokenType::NULLS)) {
            if (match(TokenType::FIRST)) {
                elem.nulls_first = true;
            } else if (match(TokenType::LAST)) {
                elem.nulls_first = false;
            } else {
                throw std::runtime_error("Expected FIRST or LAST after NULLS in index element at line " +
                    std::to_string(current().line) + ", column " +
                    std::to_string(current().column));
            }
        }

        stmt.params.push_back(std::move(elem));
    } while (match(TokenType::COMMA)); // comma separated list of columns

    if (match(TokenType::WHERE)) {
        stmt.where = parse_expression();
    }

    if (match(TokenType::TABLESPACE)) {
        stmt.tablespace = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected tablespace name after TABLESPACE in CREATE INDEX")).literal;
    }

    return stmt;
}

CreateTriggerStmt Parser::parse_create_trigger_stmt() {
    CreateTriggerStmt stmt;

    expect(TokenType::TRIGGER, errMsg(current(), "Expected TRIGGER keyword in CREATE TRIGGER"));

    stmt.trigger_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected trigger name in CREATE TRIGGER")).literal;

    if (match(TokenType::BEFORE)) {
        stmt.timing = TriggerTiming::BEFORE;
    } else if (match(TokenType::AFTER)) {
        stmt.timing = TriggerTiming::AFTER;
    } else if (match(TokenType::INSTEAD)){
        expect(TokenType::OF, errMsg(current(), "Expected OF after INSTEAD in CREATE TRIGGER"));
        stmt.timing = TriggerTiming::INSTEAD_OF;
    } else {
        throw std::runtime_error("Expected trigger timing (BEFORE, AFTER, INSTEAD OF) in CREATE TRIGGER at line " +
            std::to_string(current().line) + ", column " +
            std::to_string(current().column));
    }

    // Parse events
    do {
        if (match(TokenType::INSERT)) {
            stmt.events.push_back(TriggerEvent::INSERT);
        } else if (match(TokenType::UPDATE)) {
            stmt.events.push_back(TriggerEvent::UPDATE);
            if (match(TokenType::OF)) {
                // Parse optional column list for UPDATE OF
                do {
                    const Token col_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name after UPDATE OF in CREATE TRIGGER"));
                    stmt.update_of_columns->emplace_back(col_token.literal);
                } while (match(TokenType::COMMA));
            }
        } else if (match(TokenType::DELETE)) {
            stmt.events.push_back(TriggerEvent::DELETE);
        } else if (match(TokenType::TRUNCATE)) {
            stmt.events.push_back(TriggerEvent::TRUNCATE);
        } else {
            throw std::runtime_error("Expected trigger event (INSERT, UPDATE, DELETE, TRUNCATE) in CREATE TRIGGER at line " +
                std::to_string(current().line) + ", column " +
                std::to_string(current().column));
        }
    } while (match(TokenType::OR));

    if (match(TokenType::FOR)) {
        if (match(TokenType::EACH)) {
            if (match(TokenType::ROW)) {
                stmt.for_each = TriggerForEach::ROW;
            } else if (match(TokenType::STATEMENT)) {
                stmt.for_each = TriggerForEach::STATEMENT;
            } else {
                throw std::runtime_error("Expected ROW or STATEMENT after EACH in CREATE TRIGGER at line " +
                    std::to_string(current().line) + ", column " +
                    std::to_string(current().column));
            }
        } else {
            throw std::runtime_error("Expected EACH after FOR in CREATE TRIGGER at line " +
                std::to_string(current().line) + ", column " +
                std::to_string(current().column));
        }
    }

    if (match(TokenType::WHEN)) {
        expect(TokenType::LPAREN, errMsg(current(), "Expected '(' after WHEN in CREATE TRIGGER"));
        stmt.when = parse_expression();
        expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after WHEN expression in CREATE TRIGGER"));
    }

    expect(TokenType::ON, errMsg(current(), "Expected ON keyword in CREATE TRIGGER"));
    stmt.table_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected table name in CREATE TRIGGER")).literal;

    expect(TokenType::EXECUTE, errMsg(current(), "Expected EXECUTE keyword in CREATE TRIGGER"));
    expect(TokenType::FUNCTION, errMsg(current(), "Expected FUNCTION keyword in CREATE TRIGGER"));
    stmt.function_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected function name in CREATE TRIGGER")).literal;
    if (match(TokenType::LPAREN)) {
        // Parse function arguments
        if (current().type != TokenType::RPAREN) {
            do {
                stmt.function_args.push_back(parse_expression());
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::RPAREN, errMsg(current(), "Expected ')' after function arguments in CREATE TRIGGER"));
    }
    return stmt;
}

CreateSequenceStmt Parser::parse_create_sequence_stmt() {
    CreateSequenceStmt stmt;

    if (match(TokenType::TEMPORARY)) {
        stmt.temporary = true;
    }

    expect(TokenType::SEQUENCE, errMsg(current(), "Expected SEQUENCE keyword in CREATE SEQUENCE"));
    if (match(TokenType::IF)) {
        expect(TokenType::NOT, errMsg(current(), "Expected NOT after IF in CREATE SEQUENCE"));
        expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after NOT in CREATE SEQUENCE"));
        stmt.if_not_exists = true;
    }

    stmt.sequence_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected sequence name after CREATE SEQUENCE")).literal;

    while (current().type != TokenType::SEMICOLON && current().type != TokenType::EOF_TOKEN) {
        switch (current().type) {
            case TokenType::INCREMENT:{
                advance(); // consume INCREMENT
                expect(TokenType::BY, errMsg(current(), "Expected BY after INCREMENT in CREATE SEQUENCE"));

                const int64_t sign = determine_sign();

                const Token inc_token = expect(TokenType::NUMBER, errMsg(current(), "Expected number after INCREMENT BY in CREATE SEQUENCE"));
                stmt.increment_by = std::stoi(inc_token.literal) * sign;
                break;
            } case TokenType::MINVALUE: {
                advance();
                int64_t sign = determine_sign();

                const Token min_token = expect(TokenType::NUMBER, errMsg(current(), "Expected number after MINVALUE in CREATE SEQUENCE"));
                stmt.min_value = std::stoi(min_token.literal) * sign;
                break;
            } case TokenType::MAXVALUE: {
                advance();
                int64_t sign = determine_sign();

                const Token max_token = expect(TokenType::NUMBER, errMsg(current(), "Expected number after MAXVALUE in CREATE SEQUENCE"));
                stmt.max_value = std::stoi(max_token.literal) * sign;
                break;
            } case TokenType::CYCLE: {
                advance();
                stmt.cycle = true;
                break;
            } case TokenType::START: {
                advance();
                expect(TokenType::WITH, errMsg(current(), "Expected WITH after START in CREATE SEQUENCE"));

                const int64_t sign = determine_sign();

                const Token start_token = expect(TokenType::NUMBER, errMsg(current(), "Expected number after START WITH in CREATE SEQUENCE"));
                stmt.start_value = std::stoi(start_token.literal) * sign;
                break;
            } case TokenType::CACHE: {
                advance();
                const Token cache_token = expect(TokenType::NUMBER, errMsg(current(), "Expected number after CACHE in CREATE SEQUENCE"));
                stmt.cache_size = std::stoi(cache_token.literal);
                break;
            } case TokenType::NO: {
                advance();
                if (match(TokenType::CYCLE)) stmt.cycle = false;
                else if (match(TokenType::MINVALUE)) stmt.min_value = std::nullopt;
                else if (match(TokenType::MAXVALUE)) stmt.max_value = std::nullopt;
                else throw std::runtime_error("Expected CYCLE, MINVALUE, or MAXVALUE after NO in CREATE SEQUENCE at line " +
                    std::to_string(current().line) + ", column " +
                    std::to_string(current().column));
                break;
            } case TokenType::OWNED: {
                advance();
                expect(TokenType::BY, errMsg(current(), "Expected BY after OWNED in CREATE SEQUENCE"));
                if (match(TokenType::NONE)) {
                    stmt.owner = std::nullopt;
                } else {
                    const Token table_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected table name after OWNED BY in CREATE SEQUENCE"));
                    expect(TokenType::DOT, errMsg(current(), "Expected '.' between table and column name in OWNED BY in CREATE SEQUENCE"));
                    const Token column_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name after '.' in OWNED BY in CREATE SEQUENCE"));
                    stmt.owner = std::make_pair(table_token.literal, column_token.literal);
                }
                break;
            } default: {
                throw std::runtime_error("Unknown option in CREATE SEQUENCE at line " +
                std::to_string(current().line) + ", column " +
                std::to_string(current().column));
                break;
            }
        }
    }
    return stmt;
}

CreateSchemaStmt Parser::parse_create_schema_stmt() {
    CreateSchemaStmt stmt;

    expect(TokenType::SCHEMA, errMsg(current(), "Expected SCHEMA keyword after CREATE"));

    if (match(TokenType::IF)) {
        expect(TokenType::NOT, errMsg(current(), "Expected NOT after IF in CREATE SCHEMA"));
        expect(TokenType::EXISTS, errMsg(current(), "Expected EXISTS after NOT in CREATE SCHEMA"));
        stmt.if_not_exists = true;
    }

    if (match(TokenType::AUTHORIZATION)) {
        const Token owner_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected owner name after AUTHORIZATION in CREATE SCHEMA"));
        stmt.authorization = owner_token.literal;
    }

    stmt.schema_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected schema name after CREATE SCHEMA")).literal;
    // TODO: Parse schema elements (tables, views, etc.) if needed
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
