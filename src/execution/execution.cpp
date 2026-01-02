//
// Created by mikai on 03.01.2026.
//

#include <iostream>
#include "executor.h"

void Executor::execute(const Statement& stmt) {
    if (std::holds_alternative<CreateStmt>(stmt)) {
        handle_create(std::get<CreateStmt>(stmt));
    }
    else if (std::holds_alternative<SelectStmt>(stmt)) {}
}

void Executor::handle_create(const CreateStmt& stmt) {
    std::visit([this]<typename T0>(T0&& arg) {
        using T = std::decay_t<T0>;

        if constexpr (std::is_same_v<T, CreateTableStmt>) {
            catalog.create_table(arg);
            std::cout << "Table " << arg.table_name << " created successfully.\n";
        } else if constexpr (std::is_same_v<T, CreateSequenceStmt>) {
            catalog.create_sequence(arg);
            std::cout << "Sequence " << arg.sequence_name << " created successfully.\n";
        }
    }, stmt);
}