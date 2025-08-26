#include "Parser.hh"
#include "../Miscellaneous/LoggerHandler/LoggerFile.hh"

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
        Write(
            "Parser",
            "Unexpected token '" + tok.value + 
            "' at line " + std::to_string(tok.line) + 
            ", column " + std::to_string(tok.column) +
            (expectedValue.empty() ? "" : " (expected '" + expectedValue + "')"),
            2,
            true
        );
    }
    return tok;
}

bool Parser::check(int tokenType, const std::string& expectedValue) const {
    const Token& tok = peek();
    if (tok.type != tokenType) return false;
    if (!expectedValue.empty() && tok.value != expectedValue) return false;
    return true;
}

const Token& Parser::expect(int tokenType, const std::string& expectedValue) {
    if (check(tokenType, expectedValue)) {
        return advance();
    }
    const Token& tok = peek();
    Write(
        "Parser",
        "Expected '" + expectedValue + 
        "' but got '" + tok.value + 
        "' at line " + std::to_string(tok.line) + 
        ", column " + std::to_string(tok.column),
        2,
        true
    );
    return tok;
}

bool Parser::lookahead(int offset, int tokenType, const std::string& expectedValue) const {
    const Token& tok = peek(offset);
    if (tok.type != tokenType) return false;
    if (!expectedValue.empty() && tok.value != expectedValue) return false;
    return true;
}

bool Parser::isAtEnd() const {
    return index >= tokens.size() || tokens[index].type == TokenType::EndOfFile;
}
