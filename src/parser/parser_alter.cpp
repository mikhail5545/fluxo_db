//
// Created by mikai on 03.01.2026.
//

#include "parser.h"

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