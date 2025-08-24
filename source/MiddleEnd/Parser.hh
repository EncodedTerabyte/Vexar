#pragma once
#include "../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../Token.hh"

#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <set>

class Parser {
    const std::vector<Token>& tokens;
    size_t index = 0;

public:
    Parser(const std::vector<Token>& t);

    const Token& peek(int offset = 0) const;
    const Token& advance();
    bool match(int tokenType);
    const Token& consume(int tokenType, const std::string& expectedValue = "");
    bool isAtEnd() const;
};
