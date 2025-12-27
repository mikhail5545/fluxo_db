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

#ifndef FLUXO_DB_LEXER_H
#define FLUXO_DB_LEXER_H
#include <string>
#include <unordered_map>

#endif //FLUXO_DB_LEXER_H

// Enum for all possible token types
enum class TokenType {
    // Keywords
    SELECT, INSERT, INTO, VALUES, FROM, WHERE, CREATE, TABLE, DROP, DELETE, UPDATE, SET,

    // Literals
    IDENTIFIER, // Table names, column names, etc.
    STRING,
    NUMBER,

    // Symbols
    COMMA, // ,
    SEMICOLON, // ;
    ASTERISK, // *
    DOT, // .
    EQUALS, // =
    LPAREN, // (
    RPAREN, // )

    // Operators
    PLUS,
    MINUS,
    SLASH,
    PERCENT,
    CARET,
    EOF_TOKEN,

    ILLEGAL,
    UNKNOWN,
};

struct Token {
    TokenType type;
    std::string literal;
    int line;
    int column;
};

class Lexer {
private:
    std::string input;
    size_t position = 0;
    size_t readPosition = 0;
    char ch = 0;
    int line = 1;
    int column = 0;

    // Map string keywords to enum TokenType
    std::unordered_map<std::string, TokenType> keywords = {
        {"SELECT", TokenType::SELECT},
        {"INSERT", TokenType::INSERT},
        {"INTO", TokenType::INTO},
        {"VALUES", TokenType::VALUES},
        {"FROM", TokenType::FROM},
        {"WHERE", TokenType::WHERE},
        {"CREATE", TokenType::CREATE},
        {"TABLE", TokenType::TABLE},
        {"DROP", TokenType::DROP},
        {"DELETE", TokenType::DELETE},
        {"UPDATE", TokenType::UPDATE},
        {"SET", TokenType::SET}
    };

    void readChar();
    void skipWhitespace();
    std::string readIdentifier();
    std::string readNumber();
    TokenType lookupIdent(const std::string& ident);

    friend class Parser;
public:
    explicit Lexer(std::string input);

    Token NextToken();
};