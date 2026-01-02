//
// Created by mikai on 03.01.2026.
//

#include "parser.h"
#include <algorithm>

CreateStmt Parser::parse_create_stmt() {
    // We have already consumed CREATE keyword
    // Peek ahead to skip optional modifiers and find the object type
    size_t offset = 0;
    TokenType type = peek(offset).type;

    while (type == TokenType::TEMPORARY ||
        type == TokenType::UNIQUE ||
        type == TokenType::OR ||
        type == TokenType::REPLACE ||
        type == TokenType::CONCURRENTLY) {
        offset++;
        type = peek(offset).type;
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
        case TokenType::VIEW:
            return parse_create_view_stmt();
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

CreateViewStmt Parser::parse_create_view_stmt() {
    CreateViewStmt stmt;

    if (match(TokenType::OR)) {
        expect(TokenType::REPLACE, errMsg(current(), "Expected REPLACE after OR in CREATE VIEW"));
        stmt.or_replace = true;
    }

    if (match(TokenType::TEMPORARY)) {
        stmt.temporary = true;
    }

    if (match(TokenType::RECURSIVE)) {
        stmt.recursive = true;
    }

    expect(TokenType::VIEW, errMsg(current(), "Expected VIEW keyword in CREATE VIEW"));
    stmt.view_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected view name in CREATE VIEW")).literal;

    if (match(TokenType::LPAREN)) {
        // Parse optional column list
        do {
            const Token col_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected column name in view column list"));
            stmt.columns.push_back(col_token.literal);
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::AS, errMsg(current(), "Expected AS keyword in CREATE VIEW"));
    stmt.select_stmt = parse_select_stmt();

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

    stmt.schema_name = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected schema name after CREATE SCHEMA")).literal;

    if (match(TokenType::AUTHORIZATION)) {
        const Token owner_token = expect(TokenType::IDENTIFIER, errMsg(current(), "Expected owner name after AUTHORIZATION in CREATE SCHEMA"));
        stmt.authorization = owner_token.literal;
    }

    while (current().type != TokenType::SEMICOLON && current().type != TokenType::EOF_TOKEN) {
        SchemaElement element;
        switch (current().type) {
            case TokenType::TABLE:
                element = parse_create_table_stmt();
                break;
            case TokenType::INDEX:
                element = parse_create_index_stmt();
                break;
            case TokenType::VIEW:
                element = parse_create_view_stmt();
                break;
            case TokenType::SEQUENCE:
                element = parse_create_sequence_stmt();
                break;
            case TokenType::TRIGGER:
                element = parse_create_trigger_stmt();
                break;
            default:
                throw std::runtime_error("Unknown schema element type in CREATE SCHEMA at line " +
                    std::to_string(current().line) + ", column " +
                    std::to_string(current().column));
        }
        if (!stmt.schema_elements.has_value()) {
            stmt.schema_elements = std::vector<SchemaElement>{};
        }
        stmt.schema_elements->push_back(std::move(element));
    }

    return stmt;
}
