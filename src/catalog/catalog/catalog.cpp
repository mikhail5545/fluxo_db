//
// Created by mikai on 03.01.2026.
//

#include "catalog.h"

bool Catalog::create_table(const CreateTableStmt &stmt) {
    if (tables.find(stmt.table_name) != tables.end()) {
        if (stmt.if_not_exists) return true;
        throw std::runtime_error("Table " + stmt.table_name + " already exists");
    }

    TableInfo info;
    info.name = stmt.table_name;
    info.columns = stmt.columns;

    tables[stmt.table_name] = info;
    return true;
}

bool Catalog::create_sequence(const CreateSequenceStmt &stmt) {
    SequenceInfo info;
    info.name = stmt.sequence_name;
    info.current_value = stmt.start_value;
    info.increment = stmt.increment_by;
    info.max_value = stmt.max_value.value_or(INT64_MAX);
    info.min_value = stmt.min_value.value_or(1);
    info.cycle = stmt.cycle;
    sequences[stmt.sequence_name] = info;
    return true;
}

std::optional<TableInfo> Catalog::get_table(const std::string &table_name) const {
    if (const auto it = tables.find(table_name); it != tables.end()) {
        return it->second;
    }
    return std::nullopt;
}

