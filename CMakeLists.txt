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
        src/parser/lexer.h
        src/parser/lexer.cpp
        tests/unit/lexer_test.cpp
        tests/test_main.cpp
        src/parser/parser.h
        src/parser/parser.cpp
        src/parser/ast.h
        src/parser/ast.cpp
)

target_link_libraries(fluxo_db PRIVATE gtest gtest_main)
target_include_directories(fluxo_db PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(fluxo_db_tests tests/test_main.cpp)
target_link_libraries(fluxo_db_tests PRIVATE fluxo_db gtest gtest_main)
add_test(NAME FluxoTests COMMAND fluxo_db_tests)