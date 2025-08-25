#include "NumberExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "IdentifierExpression.hh"
#include "StringExpression.hh"
#include "../Nodes/ParenNode.hh"
#include <map>
#include <set>
#include <memory>
#include <string>

int GetPrecedence(const Token& tok) {
   static std::map<std::string, int> prec = {
       {"=", 1}, {":=", 1}, {"==", 2}, {"!=", 2},
       {"<", 2}, {"<=", 2}, {">", 2}, {">=", 2},
       {"+", 3}, {"-", 3},
       {"*", 4}, {"/", 4}, {"%", 4},
       {"^", 5}
   };
   if (tok.type == TokenType::Operator && prec.count(tok.value))
       return prec[tok.value];
   return -1;
}

bool IsRightAssociative(const Token& tok) {
   return tok.type == TokenType::Operator && tok.value == "^";
}

namespace NumberExpression {
   std::unique_ptr<ASTNode> ParsePrimary(Parser& parser, const std::set<std::string>& stopTokens, bool& hasNumbers, bool& hasStrings) {
       const Token& tok = parser.peek();

       if (stopTokens.count(tok.value)) return nullptr;

       if (tok.type == TokenType::Number || tok.type == TokenType::Float) {
           hasNumbers = true;
           parser.advance();
           auto node = std::make_unique<NumberNode>();
           node->type = NodeType::Number;
           node->token = tok;
           node->value = std::stod(tok.value);
           return node;
       }

       if (tok.type == TokenType::String) {
           hasStrings = true;
           parser.advance();
           auto node = std::make_unique<StringNode>();
           node->type = NodeType::String;
           node->token = tok;
           node->value = tok.value;
           return node;
       }

       if (tok.type == TokenType::Character) {
           hasStrings = true;
           parser.advance();
           auto node = std::make_unique<CharacterNode>();
           node->type = NodeType::Character;
           node->token = tok;
           node->value = tok.value[0];
           return node;
       }

       if (tok.type == TokenType::Identifier) {
           return IdentifierExpression::Parse(parser);
       }

       if (tok.type == TokenType::Delimiter && tok.value == "(") {
           parser.advance();
           auto inner = ParseBinary(parser, 0, stopTokens, hasNumbers, hasStrings);
           if (parser.peek().type != TokenType::Delimiter || parser.peek().value != ")") {
               Write("Number Expression", "Expected ')'", 2, true);
           }
           parser.advance();
           auto parenNode = std::make_unique<ParenNode>();
           parenNode->type = NodeType::Paren;
           parenNode->inner = std::move(inner);
           return parenNode;
       }

       Write("Number Expression", "Unexpected token: " + tok.value, 2, true);
       return nullptr;
   }

   std::unique_ptr<ASTNode> ParseBinary(Parser& parser, int precedence, const std::set<std::string>& stopTokens, bool& hasNumbers, bool& hasStrings) {
       auto left = ParseUnary(parser, stopTokens, hasNumbers, hasStrings);
       if (!left) return nullptr;

       while (true) {
           const Token& tok = parser.peek();
           if (stopTokens.count(tok.value) || tok.value == ")") break;
           int tokPrec = GetPrecedence(tok);
           if (tokPrec < precedence) break;

           if (hasNumbers && hasStrings) {
               Write("Number Expression", "Mixed string and numeric expressions are not allowed", 2, true);
               return nullptr;
           }

           parser.advance();
           int nextPrec = IsRightAssociative(tok) ? tokPrec : tokPrec + 1;
           auto right = ParseBinary(parser, nextPrec, stopTokens, hasNumbers, hasStrings);
           
           if (hasNumbers && hasStrings) {
               Write("Number Expression", "Mixed string and numeric expressions are not allowed", 2, true);
               return nullptr;
           }

           auto binNode = std::make_unique<BinaryOpNode>();
           binNode->type = NodeType::BinaryOp;
           binNode->token = tok;
           binNode->op = tok.value;
           binNode->left = std::move(left);
           binNode->right = std::move(right);
           left = std::move(binNode);
       }

       return left;
   }

   std::unique_ptr<ASTNode> ParseUnary(Parser& parser, const std::set<std::string>& stopTokens, bool& hasNumbers, bool& hasStrings) {
       const Token& tok = parser.peek();
       if (tok.type == TokenType::Operator && (tok.value == "-" || tok.value == "+" || tok.value == "!" || tok.value == "~")) {
           parser.advance();
           auto node = std::make_unique<UnaryOpNode>();
           node->type = NodeType::UnaryOp;
           node->token = tok;
           node->op = tok.value;
           node->operand = ParseUnary(parser, stopTokens, hasNumbers, hasStrings);
           return node;
       }
       return ParsePrimary(parser, stopTokens, hasNumbers, hasStrings);
   }
   
   std::unique_ptr<ASTNode> Parse(Parser& parser, int precedence, const std::set<std::string>& stopTokens) {
       bool hasNumbers = false;
       bool hasStrings = false;
       return ParseBinary(parser, precedence, stopTokens, hasNumbers, hasStrings);
   }

   std::unique_ptr<ASTNode> ParseExpression(Parser& parser, const std::set<std::string>& stopTokens) {
       return Parse(parser, 0, stopTokens);
   }
}