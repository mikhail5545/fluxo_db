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

#include <gtest/gtest.h>
#include "src/parser/lexer.h"
#include <vector>
#include <string>

// Helper struct for test cases
struct ExpectedToken {
    TokenType type;
    std::string literal;
};

TEST(LexerTest, TextNextTokenBasic) {
    std::string input = "SELECT * FROM users WHERE id = 10;";

    std::vector<ExpectedToken> expectedTokens = {
        {TokenType::SELECT, "SELECT"},
        {TokenType::ASTERISK, "*"},
        {TokenType::FROM, "FROM"},
        {TokenType::IDENTIFIER, "users"},
        {TokenType::WHERE, "WHERE"},
        {TokenType::IDENTIFIER, "id"},
        {TokenType::EQUALS, "="},
        {TokenType::NUMBER, "10"},
        {TokenType::SEMICOLON, ";"},
        {TokenType::EOF_TOKEN, ""}
    };

    Lexer lexer(input);

    for (const auto &[type, literal] : expectedTokens) {
        Token tok = lexer.NextToken();

        EXPECT_EQ(tok.type, type) << "Token type mismatch.";
        // For EOF, the literal might be empty or a null char string
        if (type != TokenType::EOF_TOKEN) {
            EXPECT_EQ(tok.literal, literal) << "Token literal mismatch.";
        }
    }
}

TEST(LexerTest, TestPunctuation) {
    std::string input = ", . ( ) ; * =";

    std::vector<ExpectedToken> expectedTokens = {
        {TokenType::COMMA, ","},
        {TokenType::DOT, "."},
        {TokenType::LPAREN, "("},
        {TokenType::RPAREN, ")"},
        {TokenType::SEMICOLON, ";"},
        {TokenType::ASTERISK, "*"},
        {TokenType::EQUALS, "="},
        {TokenType::EOF_TOKEN, ""}
    };
    Lexer lexer(input);

    for (const auto & [type, literal] : expectedTokens) {
        Token tok = lexer.NextToken();
        std::cout << "Got token: " << tok.literal << " of type " << static_cast<int>(tok.type) << std::endl;
        EXPECT_EQ(tok.type, type) << "Token type mismatch.";
        // For EOF, the literal might be empty or a null char string
        if (type != TokenType::EOF_TOKEN) {
            EXPECT_EQ(tok.literal, literal) << "Token literal mismatch.";
        }
    }
}

TEST(LexerTest, TestIdentifiersAndNumbers) {
    std::string input = "table_name column1 12345 another_table 67890";

    std::vector<ExpectedToken> expectedTokens = {
        {TokenType::IDENTIFIER, "table_name"},
        {TokenType::IDENTIFIER, "column1"},
        {TokenType::NUMBER, "12345"},
        {TokenType::IDENTIFIER, "another_table"},
        {TokenType::NUMBER, "67890"},
        {TokenType::EOF_TOKEN, ""}
    };
    Lexer lexer(input);

    for (const auto & [type, literal] : expectedTokens) {
        Token tok = lexer.NextToken();
        std::cout << "Got token: " << tok.literal << " of type " << static_cast<int>(tok.type) << std::endl;
        EXPECT_EQ(tok.type, type) << "Token type mismatch.";
        // For EOF, the literal might be empty or a null char string
        if (type != TokenType::EOF_TOKEN) {
            EXPECT_EQ(tok.literal, literal) << "Token literal mismatch.";
        }
    }
}

TEST (LexerTest, TestCaseInsensitivity) {
    std::string input = "select FroM";

    std::vector<ExpectedToken> expectedTokens = {
        {TokenType::SELECT, "select"},
        {TokenType::FROM, "FroM"},
        {TokenType::EOF_TOKEN, ""}
    };
    Lexer lexer(input);

    for (const auto & [type, literal] : expectedTokens) {
        Token tok = lexer.NextToken();
        std::cout << "Got token: " << tok.literal << " of type " << static_cast<int>(tok.type) << std::endl;
        EXPECT_EQ(tok.type, type) << "Token type mismatch.";
        // For EOF, the literal might be empty or a null char string
        if (type != TokenType::EOF_TOKEN) {
            EXPECT_EQ(tok.literal, literal) << "Token literal mismatch.";
        }
    }
}