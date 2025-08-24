#pragma once
#include "../AST.hh"
#include "../../Token.hh"
#include "../Parser.hh"
#include <memory>
#include <map>
#include <string>

int GetPrecedence(const Token& tok);
bool IsRightAssociative(const Token& tok);

namespace NumberExpression {
    std::unique_ptr<ASTNode> ParseBinary(Parser& parser, int precedence = 0, const std::set<std::string>& stopTokens = {});
    std::unique_ptr<ASTNode> Parse(Parser& parser, int precedence = 0, const std::set<std::string>& stopTokens = {});
    std::unique_ptr<ASTNode> ParsePrimary(Parser& parser, const std::set<std::string>& stopTokens);
    std::unique_ptr<ASTNode> ParseUnary(Parser& parser, const std::set<std::string>& stopTokens);
    std::unique_ptr<ASTNode> ParseExpression(Parser& parser, const std::set<std::string>& stopTokens);
}
