cmake_minimum_required(VERSION 4.1)
project(fluxo_db)

set(CMAKE_CXX_STANDARD 26)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_library(fluxo_db SHARED library.cpp
        tools/repl/repl.h
        src/lexer/lexer.h
        src/lexer/lexer.cpp
        tests/unit/lexer_test.cpp
        tests/test_main.cpp
        src/parser/parser.h
        src/ast/ast.cpp
        tests/unit/parser_test.cpp
        src/ast/ast_statements.h
        src/ast/ast_expr.h
        src/parser/parser_core.cpp
        src/parser/parser_expr.cpp
        src/parser/parser_select.cpp
        src/parser/parser_insert.cpp
        src/parser/parser_alter.cpp
        src/parser/parser_drop.cpp
        src/parser/parser_create.cpp
        src/catalog/schema/schema.h
        src/catalog/catalog/catalog.h
        src/catalog/catalog/catalog.cpp
        src/execution/execution.cpp
        src/execution/executor.h
)

target_link_libraries(fluxo_db PRIVATE gtest gtest_main)
target_include_directories(fluxo_db PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(fluxo_db_tests tests/test_main.cpp)
target_link_libraries(fluxo_db_tests PRIVATE fluxo_db gtest gtest_main)
add_test(NAME FluxoTests COMMAND fluxo_db_tests)