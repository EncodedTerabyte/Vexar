#include "Parser.hh"

Parser::Parser(const std::vector<Token>& t) : tokens(t) {}

const Token& Parser::peek(int offset) const {
    int idx = static_cast<int>(index) + offset;
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(tokens.size())) idx = static_cast<int>(tokens.size() - 1);
    return tokens[idx];
}

const Token& Parser::advance() {
    if (index < tokens.size()) return tokens[index++];
    return tokens.back();
}

bool Parser::match(int tokenType) {
    if (peek().type == tokenType) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::consume(int tokenType, const std::string& expectedValue) {
    const Token& tok = advance();
    if (tok.type != tokenType || (!expectedValue.empty() && tok.value != expectedValue)) {
        Write("Parser", "Unexpected token: " + tok.value, 2, true);
    }
    return tok;
}

bool Parser::isAtEnd() const {
    return index >= tokens.size() || tokens[index].type == TokenType::EndOfFile;
}
