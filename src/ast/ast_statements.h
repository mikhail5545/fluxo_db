//
// Created by mikai on 01.01.2026.
//

#ifndef FLUXO_DB_AST_STATEMENTS_H
#define FLUXO_DB_AST_STATEMENTS_H

#include <string>
#include <variant>
#include <vector>
#include <optional>

#include "ast_expr.h"

struct AddColumnAction {
    ColumnDef column_def;
    bool if_not_exists = false;
};

struct AddConstraintAction {
    std::string column_name;
    bool not_null = false;
    bool unique = false;
    bool primary_key = false;
};

struct DropColumnAction {
    std::string column_name;
    bool if_exists = false;
    bool cascade = false;
};

struct DropConstraintAction {
    std::string constraint_name;
    bool if_exists = false;
    bool cascade = false;
};

struct AlterColumnTypeAction {
    std::string column_name;
    DataType new_type;
    Expr using_expr; // Optional USING expression
    std::string collation; // Optional collation
};

struct AlterColumnDefaultAction {
    std::string column_name;
    Expr default_expr; // The new default value;
    bool is_drop = false; // true if DROP DEFAULT
};

struct AlterColumnNotNullAction {
    std::string column_name;
    bool set_not_null = true; // true for SET NOT NULL, false for DROP NOT NULL
};

struct RenameColumnAction {
    std::string old_name;
    std::string new_name;
};

struct RenameTableAction {
    std::string new_name;
};

struct RenameConstraintAction {
    std::string old_name;
    std::string new_name;
};

struct SetSchemaAction {
    std::string schema_name;
};

struct OwnerToAction {
    std::string new_owner;
};

using AddAction = std::variant<AddColumnAction, AddConstraintAction>;
using DropAction = std::variant<DropColumnAction, DropConstraintAction>;
using AlterColumnAction = std::variant<AlterColumnTypeAction, AlterColumnDefaultAction, AlterColumnNotNullAction>;
using RenameAction = std::variant<RenameColumnAction, RenameTableAction, RenameConstraintAction>;

// Variant to hold any alter action
using AlterAction = std::variant<
    AddAction,
    DropAction,
    AlterColumnAction,
    RenameAction,
    SetSchemaAction,
    OwnerToAction
>;

// --- Create Statements ---

struct CreateCollationStmt {
    std::string collation_name;
    std::string locale;
    bool if_not_exists = false;
    bool deterministic = true;
    std::optional<std::string> provider; // e.g., "icu", "libc"
    std::optional<std::string> version; // e.g., "57.1"
    std::optional<std::string> rules; // Custom collation rules
    std::optional<std::string> existing_collation_name; // For "FROM" clause (copying existing collation)
};

struct CreateDatabaseStmt {
    std::string name;
    bool if_not_exists = false;
    std::string user_name = "DEFAULT"; // Owner of the database
    std::string encoding = "UTF-8";
    std::string tablespace_name = "fx_default";
    bool allow_conn = true;
    int64_t conn_limit = -1; // -1 means no limit
};

enum class OrderDirection {
    ASC,
    DESC
};
struct IndexElem {
    std::optional<std::string> name; // Column name
    std::optional<Expr> expr;        // Expression (e.g., lower(col))
    std::optional<std::string> collation;
    std::optional<std::string> op_class; // e.g., varchar_pattern_ops
    OrderDirection ordering = OrderDirection::ASC;
    std::optional<bool> nulls_first; // Optional to distinguish default behavior
};

struct CreateIndexStmt {
    std::string index_name;
    std::string table_name;
    bool unique = false;
    bool if_not_exists = false;
    bool concurrently = false;
    bool only = false;

    std::optional<std::string> method; // e.g., "btree", "hash"

    std::vector<IndexElem> params;

    std::optional<Expr> where; // Partial index condition
    std::optional<std::string> tablespace;
};

enum class TriggerEvent {
    INSERT,
    UPDATE,
    DELETE,
    TRUNCATE
};

enum class TriggerTiming {
    BEFORE,
    AFTER,
    INSTEAD_OF
};

enum class TriggerForEach {
    ROW,
    STATEMENT
};

struct CreateTriggerStmt {
    std::string trigger_name;
    std::string table_name;
    TriggerTiming timing; // BEFORE, AFTER, INSTEAD OF
    std::vector<TriggerEvent> events; // INSERT, UPDATE, DELETE, TRUNCATE
    std::optional<std::vector<std::string>> update_of_columns; // For UPDATE OF
    std::string function_name;
    std::vector<Expr> function_args;
    TriggerForEach for_each = TriggerForEach::STATEMENT; // Default to statement
    std::optional<Expr> when; // Optional WHEN condition
};

struct CreateSequenceStmt {
    std::string sequence_name;
    bool if_not_exists = false;
    bool temporary = false;
    int64_t start_value = 1;
    int64_t increment_by = 1;
    std::optional<int64_t> min_value;
    std::optional<int64_t> max_value;
    bool cycle = false;
    std::optional<int64_t> cache_size;
    std::optional<std::pair<std::string, std::string>> owner; // Owner of the sequence (table_name.column_name | None)
};

struct CreateRoleStmt {
    std::string role_name;
    bool if_not_exists = false;
    bool superuser = false;
    bool createdb = false;
    bool createrole = false;
    bool inherit = true;
    bool login = false;
    std::optional<int64_t> conn_limit; // Connection limit
    std::optional<std::string> valid_until; // Expiration timestamp
    std::optional<std::string> password; // Plaintext password
};

struct SelectStmt {
    std::vector<Expr> projections; // SELECT clause
    std::vector<TableRef> from; // FROM clause
    std::optional<Expr> where; // WHERE clause
    std::optional<Expr> having;
    std::vector<Expr> group_by;
    std::vector<std::pair<Expr, bool>> order_by; // expr, asc
    std::optional<int64_t> limit;
    std::optional<int64_t> offset;
    bool distinct = false;
};

struct CreateViewStmt {
    std::string view_name;
    bool temporary = false;
    bool or_replace = false;
    bool recursive = false;
    std::vector<std::string> columns; // Optional column names
    SelectStmt select_stmt; // The SELECT statement defining the view
};

struct InsertStmt {
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<std::vector<Expr>> values; // Multiple rows support
};

struct TableConstraint {
    enum class Type {
        PRIMARY_KEY,
        FOREIGN_KEY,
        UNIQUE,
        CHECK
    } type;

    std::string name; // Constraint name (optional in SQL, but useful to generate if empty)

    // For PK, UNIQUE, FK
    std::vector<std::string> columns;

    // For FK
    std::optional<std::string> foreign_table;
    std::vector<std::string> foreign_columns;
    // FK Actions
    char fk_match_type = 's'; // s=simple, f=full, p=partial
    char fk_update_action = 'a'; // a=no action, r=restrict, c=cascade, n=set null, d=set default
    char fk_delete_action = 'a';

    // For CHECK
    std::optional<Expr> check_expr;
};

struct CreateTableStmt {
    std::string table_name;
    std::vector<ColumnDef> columns;
    std::vector<TableConstraint> constraints;
    bool if_not_exists = false;
    std::optional<std::string> tablespace;
};

using SchemaElement = std::variant<
    CreateTableStmt,
    CreateIndexStmt,
    CreateViewStmt,
    CreateSequenceStmt,
    CreateTriggerStmt
>;

struct CreateSchemaStmt {
    std::string schema_name;
    bool if_not_exists = false;
    std::optional<std::string> authorization; // Owner of the schema
    std::optional<std::vector<SchemaElement>> schema_elements; // Elements within the schema
};

using CreateStmt = std::variant<
    CreateTableStmt,
    CreateIndexStmt,
    CreateViewStmt,
    CreateSchemaStmt,
    CreateTriggerStmt,
    CreateSequenceStmt,
    CreateDatabaseStmt,
    CreateCollationStmt,
    CreateRoleStmt
>;

// TODO: replace user by role (user is just a role with login privilege)
enum class ObjectType {
    TABLE,
    VIEW,
    INDEX,
    SCHEMA,
    TRIGGER,
    SEQUENCE,
    COLLATION,
    DATABASE,
    USER,
    TYPE
};

struct DropStmt {
    ObjectType object_type;
    std::vector<std::string> names; // Names of objects to drop
    bool if_exists = false;
    bool cascade = false;
    bool restrict = false;
    bool concurrently = false; // Specific to DROP INDEX CONCURRENTLY
};

struct AlterTableStmt {
    std::string table_name;
    bool if_exists = false; // IF EXISTS clause
    std::vector<AlterAction> actions; // List of alter actions
};

using Statement = std::variant<
    SelectStmt,
    InsertStmt,
    CreateStmt,
    DropStmt,
    AlterTableStmt
>;

#endif //FLUXO_DB_AST_STATEMENTS_H