#pragma once
#include "../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../Token.hh"

#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <set>

class Parser {
public:
    Parser(const std::vector<Token>& t);

    const Token& peek(int offset = 0) const;
    const Token& peekNext() const;
    const Token& advance();
    bool match(int tokenType);
    const Token& consume(int tokenType, const std::string& expectedValue = "");
    bool check(int tokenType, const std::string& expectedValue = "") const;
    const Token& expect(int tokenType, const std::string& expectedValue = "");
    bool lookahead(int offset, int tokenType, const std::string& expectedValue = "") const;
    bool isAtEnd() const;

private:
    std::vector<Token> tokens;
    size_t index = 0;
};