//
// Created by mikai on 03.01.2026.
//

#ifndef FLUXO_DB_EXECUTOR_H
#define FLUXO_DB_EXECUTOR_H
#pragma once
#include "../catalog/catalog/catalog.h"
#include "../../src/parser/parser.h"

class Executor {
    Catalog& catalog;
public:
    explicit Executor(Catalog& catalog_) : catalog(catalog_) {}

    void execute(const Statement& stmt);
private:
    void handle_create(const CreateStmt& stmt);
};

#endif //FLUXO_DB_EXECUTOR_H