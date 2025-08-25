#pragma once
#include "../Parser.hh"
#include "../AST.hh"
#include <set>
#include <memory>

namespace NumberExpression {
    std::unique_ptr<ASTNode> ParsePrimary(Parser& parser, const std::set<std::string>& stopTokens, bool& hasNumbers, bool& hasStrings);
    std::unique_ptr<ASTNode> ParseBinary(Parser& parser, int precedence, const std::set<std::string>& stopTokens, bool& hasNumbers, bool& hasStrings);
    std::unique_ptr<ASTNode> ParseUnary(Parser& parser, const std::set<std::string>& stopTokens, bool& hasNumbers, bool& hasStrings);
    std::unique_ptr<ASTNode> Parse(Parser& parser, int precedence = 0, const std::set<std::string>& stopTokens = {});
    std::unique_ptr<ASTNode> ParseExpression(Parser& parser, const std::set<std::string>& stopTokens = {});
}