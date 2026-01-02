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

// Enum for all possible token types
enum class TokenType {
    // Keywords
    SELECT, INSERT, INTO, VALUES, FROM, WHERE, CREATE, TABLE, DROP, DELETE, UPDATE, SET,
    PRIMARY, KEY, NOT, UNIQUE, IF, EXISTS, CASCADE, RESTRICT, ONLY, RENAME, CONSTRAINT, ALTER, ATTACH, DETACH, OWNED,
    FOR, DEFAULT, COLUMN, TO, SCHEMA, OWNER, ADD, TYPE, USING, COLLATE, DATABASE, VIEW, INDEX, TRIGGER, COLLATION, USER,
    SEQUENCE, CONCURRENTLY, FOREIGN, REFERENCES, CHECK, LOCALE, DETERMINISTIC, PROVIDER, RULES, TABLESPACE, ALLOW_CONNECTIONS,
    CONNECTION_LIMIT, ENCODING, ON, ASC, DESC, NULLS, FIRST, LAST, BEFORE, AFTER, INSTEAD, OF, OR, TRUNCATE, EXECUTE,
    FUNCTION, EACH, ROW, STATEMENT, WHEN, AUTHORIZATION, TEMPORARY, INCREMENT, BY, MINVALUE, MAXVALUE, CYCLE, START,
    WITH, NO, CACHE, NONE, ROLE, PASSWORD, LOGIN, NO_LOGIN, SUPERUSER, CONNECTION, LIMIT, VALID, UNTIL, NO_SUPERUSER, CREATE_ROLE,
    NO_CREATE_ROLE, INHERIT, NO_INHERIT, CREATE_DB, NO_CREATE_DB, REPLACE, RECURSIVE, AS, NULL_TYPE,

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
    APOSTROPHE, // '

    // Operators
    PLUS,
    MINUS,
    SLASH,
    PERCENT,
    CARET,
    EOF_TOKEN,

    TRUE,
    FALSE,

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
        {"SET", TokenType::SET},
        {"PRIMARY", TokenType::PRIMARY},
        {"KEY", TokenType::KEY},
        {"NOT", TokenType::NOT},
        {"UNIQUE", TokenType::UNIQUE},
        {"IF", TokenType::IF},
        {"EXISTS", TokenType::EXISTS},
        {"CASCADE", TokenType::CASCADE},
        {"RESTRICT", TokenType::RESTRICT},
        {"ONLY", TokenType::ONLY},
        {"RENAME", TokenType::RENAME},
        {"CONSTRAINT", TokenType::CONSTRAINT},
        {"ALTER", TokenType::ALTER},
        {"ATTACH", TokenType::ATTACH},
        {"DETACH", TokenType::DETACH},
        {"OWNED", TokenType::OWNED},
        {"FOR", TokenType::FOR},
        {"DEFAULT", TokenType::DEFAULT},
        {"COLUMN", TokenType::COLUMN},
        {"TO", TokenType::TO},
        {"SCHEMA", TokenType::SCHEMA},
        {"OWNER", TokenType::OWNER},
        {"ADD", TokenType::ADD},
        {"TYPE", TokenType::TYPE},
        {"USING", TokenType::USING},
        {"COLLATE", TokenType::COLLATE},
        {"DATABASE", TokenType::DATABASE},
        {"VIEW", TokenType::VIEW},
        {"INDEX", TokenType::INDEX},
        {"TRIGGER", TokenType::TRIGGER},
        {"COLLATION", TokenType::COLLATION},
        {"USER", TokenType::USER},
        {"SEQUENCE", TokenType::SEQUENCE},
        {"CONCURRENTLY", TokenType::CONCURRENTLY},
        {"FOREIGN", TokenType::FOREIGN},
        {"CHECK", TokenType::CHECK},
        {"REFERENCES", TokenType::REFERENCES},
        {"LOCALE", TokenType::LOCALE},
        {"DETERMINISTIC", TokenType::DETERMINISTIC},
        {"PROVIDER", TokenType::PROVIDER},
        {"RULES", TokenType::RULES},
        {"TRUE", TokenType::TRUE},
        {"FALSE", TokenType::FALSE},
        {"TABLESPACE", TokenType::TABLESPACE},
        {"ALLOW_CONNECTIONS", TokenType::ALLOW_CONNECTIONS},
        {"CONNECTION_LIMIT", TokenType::CONNECTION_LIMIT},
        {"ENCODING", TokenType::ENCODING},
        {"ON", TokenType::ON},
        {"ASC", TokenType::ASC},
        {"ASCENDING", TokenType::ASC},
        {"DESC", TokenType::DESC},
        {"DESCENDING", TokenType::DESC},
        {"NULLS", TokenType::NULLS},
        {"FIRST", TokenType::FIRST},
        {"LAST", TokenType::LAST},
        {"BEFORE", TokenType::BEFORE},
        {"AFTER", TokenType::AFTER},
        {"INSTEAD", TokenType::INSTEAD},
        {"OF", TokenType::OF},
        {"TRUNCATE", TokenType::TRUNCATE},
        {"OR", TokenType::OR},
        {"EXECUTE", TokenType::EXECUTE},
        {"FUNCTION", TokenType::FUNCTION},
        {"EACH", TokenType::EACH},
        {"ROW", TokenType::ROW},
        {"STATEMENT", TokenType::STATEMENT},
        {"WHEN", TokenType::WHEN},
        {"AUTHORIZATION", TokenType::AUTHORIZATION},
        {"TEMPORARY", TokenType::TEMPORARY},
        {"TEMP", TokenType::TEMPORARY},
        {"INCREMENT", TokenType::INCREMENT},
        {"BY", TokenType::BY},
        {"MINVALUE", TokenType::MINVALUE},
        {"MAXVALUE", TokenType::MAXVALUE},
        {"CYCLE", TokenType::CYCLE},
        {"START", TokenType::START},
        {"WITH", TokenType::WITH},
        {"NO", TokenType::NO},
        {"CACHE", TokenType::CACHE},
        {"NONE", TokenType::NONE},
        {"ROLE", TokenType::ROLE},
        {"PASSWORD", TokenType::PASSWORD},
        {"LOGIN", TokenType::LOGIN},
        {"NOLOGIN", TokenType::NO_LOGIN},
        {"SUPERUSER", TokenType::SUPERUSER},
        {"CONNECTION", TokenType::CONNECTION},
        {"LIMIT", TokenType::LIMIT},
        {"VALID", TokenType::VALID},
        {"UNTIL", TokenType::UNTIL},
        {"NOSUPERUSER", TokenType::NO_SUPERUSER},
        {"CREATEROLE", TokenType::CREATE_ROLE},
        {"NOCREATEROLE", TokenType::NO_CREATE_ROLE},
        {"INHERIT", TokenType::INHERIT},
        {"NOINHERIT", TokenType::NO_INHERIT},
        {"CREATEDB", TokenType::CREATE_DB},
        {"NOCREATEDB", TokenType::NO_CREATE_DB},
        {"REPLACE", TokenType::REPLACE},
        {"RECURSIVE", TokenType::RECURSIVE},
        {"AS", TokenType::AS},
        {"NULL", TokenType::NULL_TYPE},
    };

    void readChar();
    void skipWhitespace();
    std::string readIdentifier();
    std::string readNumber();
    std::string readString();
    TokenType lookupIdent(const std::string& ident);

    friend class Parser;
public:
    explicit Lexer(const std::string &input);

    Token NextToken();
};
#endif //FLUXO_DB_LEXER_H