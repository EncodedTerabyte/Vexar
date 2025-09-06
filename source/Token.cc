#include "Token.hh"

std::set<std::string> Keywords = {"var", "if", "while", "func", "ret", "inline", "always_inline", "break", "for", "foreach"};

std::set<std::string> Operators = {
    "+", "-", "*", "/", "%",     // arithmetic
    "&&", "||",                  // logical
    "~", "!",                    // unary
    "=", "==", "!=", ":=",       // assignment / equality
    "<", "<=", ">", ">=",        // relational
    "++", "--",                  // increment / decrement
    "+=", "-=", "*=", "/=" ,     // compount operators
    "<<", ">>", "|", "&", "^",    // bitwise operators
    "$", "#", "%"
};

std::set<char> Delimiters = {
    '(', ')', '{', '}', '[', ']', ',', ';', ':', '.', '\'', '"', '`'
};

std::ostream& operator<<(std::ostream& os, const Token& tok) {
    os << "Token(Type: " << tok.type
       << ", Value: \"" << tok.value << "\""
       << ", Line: " << tok.line
       << ", Column: " << tok.column << ")";
    return os;
}