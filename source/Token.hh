#pragma once
#include <iostream>
#include <string>

#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <set>

struct TokenType {
    static constexpr int Identifier   = 0;
    static constexpr int Number       = 1;
    static constexpr int Float        = 2;
    static constexpr int String       = 3;
    static constexpr int Character    = 4;
    static constexpr int Keyword      = 5;
    static constexpr int Operator     = 6;
    static constexpr int Delimiter    = 7;
    static constexpr int EndOfFile    = 8;
};

struct Token {
    int type;
    std::string value;
    int line;
    int column;
};

std::ostream& operator<<(std::ostream& os, const Token& tok);

const std::set<std::string> Keywords = {"var", "if", "while", "func", "ret", "true", "false"};

const std::set<std::string> Operators = {
    "+", "-", "*", "/", "%",     // arithmetic
    "&", "&&", "|", "||", "^",   // bitwise & logical
    "~", "!",                    // unary
    "=", "==", "!=", ":=",       // assignment / equality
    "<", "<=", ">", ">=",        // relational
    "++", "--",                   // increment / decrement
    "+=", "-=", "*=", "/=" // Compount Operators
};

const std::set<char> Delimiters = {
    '(', ')', '{', '}', '[', ']', ',', ';', ':', '.', '\'', '"', '`'
};
