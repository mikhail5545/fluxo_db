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

#include "lexer.h"

Lexer::Lexer(const std::string &input) : input(input) {
    readChar();
}

void Lexer::readChar() {
    if (readPosition >= input.length()) {
        ch = 0; // ASCII NUL implies EOF
    } else {
        ch = input[readPosition];
    }
    position = readPosition;
    readPosition++;
    column++;
}

void Lexer::skipWhitespace() {
    while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
        if (ch == '\n') {
            line++;
            column = 0;
        }
        readChar();
    }
}

std::string Lexer::readIdentifier() {
    const size_t startPosition = position;
    // Allow alphanumeric and underscore
    while (isalpha(ch) || ch == '_' || isdigit(ch)) {
        readChar();
    }
    // Return the substring representing the identifier
    return input.substr(startPosition, position - startPosition);
}

std::string Lexer::readNumber() {
    const size_t startPosition = position;
    while (isdigit(ch) || ch == '.') {
        readChar();
    }
    return input.substr(startPosition, position - startPosition);
}

// Helper to distinguish between keywords and identifiers
TokenType Lexer::lookupIdent(const std::string& ident) {
    // Postgres keywords are case-insensitive. Convert to UPPER for lookup
    std::string upperIdent = ident;
    for (auto & c : upperIdent) c = toupper(c);

    if (keywords.contains(upperIdent)) {
        return keywords[upperIdent];
    }
    return TokenType::IDENTIFIER;
}

std::string Lexer::readString() {
    const size_t startPosition = position + 1; // Skip opening quote
    readChar(); // Move past the opening quote

    while (ch != '\'' && ch != 0) {
        readChar();
    }

    // Capture the string literal
    std::string str = input.substr(startPosition, position - startPosition);

    if (ch == '\'') {
        readChar(); // Consume the closing quote
    }
    return str;
}

Token Lexer::NextToken() {
    Token tok;
    skipWhitespace();

    tok.line = line;
    tok.column = column;

    switch (ch) {
        case ',':
            tok = {TokenType::COMMA, std::string(1, ch), line, column};
            break;
        case ';':
            tok = {TokenType::SEMICOLON, std::string(1, ch), line, column};
            break;
        case '*':
            tok = {TokenType::ASTERISK, std::string(1, ch), line, column};
            break;
        case '.':
            tok = {TokenType::DOT, std::string(1, ch), line, column};
            break;
        case '=':
            tok = {TokenType::EQUALS, std::string(1, ch), line, column};
            break;
        case '(':
            tok = {TokenType::LPAREN, std::string(1, ch), line, column};
            break;
        case ')':
            tok = {TokenType::RPAREN, std::string(1, ch), line, column};
            break;
        case '+':
            tok = {TokenType::PLUS, std::string(1, ch), line, column};
            break;
        case '-':
            tok = {TokenType::MINUS, std::string(1, ch), line, column};
            break;
        case '%':
            tok = {TokenType::PERCENT, std::string(1, ch), line, column};
            break;
        case '^':
            tok = {TokenType::CARET, std::string(1, ch), line, column};
            break;
        case '\'':
            tok.type = TokenType::STRING;
            tok.literal = readString();
            tok.line = line;
            tok.column = column;
            return tok;
        case 0:
            tok = {TokenType::EOF_TOKEN, std::string(1, ch), line, column};
            break;
        default:
            if (isalpha(ch) || ch == '_') {
                // It's a keyword or identifier
                const std::string ident = readIdentifier();
                const TokenType type = lookupIdent(ident);
                return {type, ident, line, column - static_cast<int>(ident.length())};
            } else if (isdigit(ch)) {
                const std::string number = readNumber();
                return {TokenType::NUMBER, number, line, column - static_cast<int>(number.length())};
            } else {
                tok = {TokenType::ILLEGAL, std::string(1, ch), line, column};
            }
    }

    readChar();
    return tok;
}