//
// Created by mikai on 03.01.2026.
//

#ifndef FLUXO_DB_CATALOG_H
#define FLUXO_DB_CATALOG_H

#pragma once

#include "../../ast/ast_statements.h"
#include "../schema/schema.h"

class Catalog {
public:
    bool create_table(const CreateTableStmt &stmt);
    bool create_sequence(const CreateSequenceStmt &stmt);
    [[nodiscard]] std::optional<TableInfo> get_table(const std::string &table_name) const;
private:
    std::unordered_map<std::string, TableInfo> tables;
    std::unordered_map<std::string, SequenceInfo> sequences;
};

#endif //FLUXO_DB_CATALOG_H