#pragma once
#include "../AST.hh"
#include "../Parser.hh"
#include <memory>

namespace StringExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser, int precedence = 0, const std::set<std::string>& stopTokens = {}) ;
}
