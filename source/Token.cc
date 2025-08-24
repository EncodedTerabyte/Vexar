#include "Token.hh"

std::ostream& operator<<(std::ostream& os, const Token& tok) {
    os << "Token(Type: " << tok.type
       << ", Value: \"" << tok.value << "\""
       << ", Line: " << tok.line
       << ", Column: " << tok.column << ")";
    return os;
}