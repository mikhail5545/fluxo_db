//
// Created by mikai on 03.01.2026.
//

#ifndef FLUXO_DB_SCHEMA_H
#define FLUXO_DB_SCHEMA_H

#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "../../ast/ast_statements.h"

struct TableInfo {
    std::string name;
    std::vector<ColumnDef> columns;
    int32_t root_page_id; // Page ID of the root page of the table
    std::vector<TableConstraint> constraints;
};

struct SequenceInfo {
    std::string name;
    int64_t current_value;
    int64_t increment;
    int64_t max_value;
    int64_t min_value;
    bool cycle;
};

#endif //FLUXO_DB_SCHEMA_H