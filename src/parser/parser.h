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
#include "../lexer/lexer.h"
#include "../ast/ast_expr.h"
#include "../ast/ast_statements.h"
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

    int64_t determine_sign();

    // Parsing methods
    Statement parse_statement();
    SelectStmt parse_select_stmt();
    InsertStmt parse_insert_stmt();
    AlterTableStmt parse_alter_table_stmt();
    AlterAction parse_alter_table_action();
    AddAction parse_add_action();
    DropAction parse_drop_action();
    AlterColumnAction parse_alter_column_action();
    RenameAction parse_rename_action();
    SetSchemaAction parse_set_schema_action();
    OwnerToAction parse_owner_to_action();
    DropStmt parse_drop_stmt();

    CreateStmt parse_create_stmt();

    ColumnDef parse_column_def();
    TableConstraint parse_table_constraint();
    CreateTableStmt parse_create_table_stmt();
    CreateCollationStmt parse_create_collation_stmt();
    CreateDatabaseStmt parse_create_database_stmt();
    CreateIndexStmt parse_create_index_stmt();
    CreateTriggerStmt parse_create_trigger_stmt();
    CreateSchemaStmt parse_create_schema_stmt();
    CreateSequenceStmt parse_create_sequence_stmt();
    CreateRoleStmt parse_create_role_stmt();
    CreateViewStmt parse_create_view_stmt();

    Expression parse_expression(int precedence = 0);
    Expression parse_primary();
public:
    explicit Parser(Lexer &lexer);
    std::vector<Statement> parse();
};

#endif //FLUXO_DB_PARSER_H
