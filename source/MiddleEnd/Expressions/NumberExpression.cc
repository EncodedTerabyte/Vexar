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
       {"=", 1}, {":=", 1}, 
       {"||", 2}, {"or", 2}, {"&&", 3}, {"and", 3},
       {"==", 4}, {"!=", 4},
       {"<", 5}, {"<=", 5}, {">", 5}, {">=", 5},
       {"+", 6}, {"-", 6},
       {"*", 7}, {"/", 7}, {"%", 7},
       {"^", 8}
   };
   if ((tok.type == TokenType::Operator || tok.type == TokenType::Keyword) && prec.count(tok.value))
       return prec[tok.value];
   return -1;
}

bool IsRightAssociative(const Token& tok) {
   return tok.type == TokenType::Operator && tok.value == "^";
}

bool IsTypeName(const std::string& name) {
    static std::set<std::string> types = {
        "int", "uint", "float", "double", "char", "bool", "string", "void"
    };
    return types.count(name) > 0;
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
           try {
               node->value = std::stod(tok.value);
           } catch (...) {
               Write("Parser", "Invalid numeric literal '" + tok.value + "' at line " +
                     std::to_string(tok.line) + ", column " + std::to_string(tok.column),
                     2, true, true, "");
               return nullptr;
           }
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
           if (tok.value.empty()) {
               Write("Parser", "Empty character literal at line " +
                     std::to_string(tok.line) + ", column " + std::to_string(tok.column),
                     2, true, true, "");
               return nullptr;
           }
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
           const Token& nextTok = parser.peek();
           
           if (nextTok.type == TokenType::Identifier && IsTypeName(nextTok.value)) {
               std::string typeName = nextTok.value;
               parser.advance();
               
               if (parser.peek().type == TokenType::Delimiter && parser.peek().value == ")") {
                   parser.advance();
                   auto expr = ParsePrimary(parser, stopTokens, hasNumbers, hasStrings);
                   if (!expr) {
                       Write("Parser", "Invalid cast expression at line " +
                             std::to_string(nextTok.line) + ", column " +
                             std::to_string(nextTok.column), 2, true, true, "");
                       return nullptr;
                   }
                   auto castNode = std::make_unique<CastNode>();
                   castNode->type = NodeType::Cast;
                   castNode->targetType = typeName;
                   castNode->expr = std::move(expr);
                   return castNode;
               } else {
                   auto identNode = std::make_unique<IdentifierNode>();
                   identNode->type = NodeType::Identifier;
                   identNode->name = typeName;
                   auto parenNode = std::make_unique<ParenNode>();
                   parenNode->type = NodeType::Paren;
                   parenNode->inner = std::move(identNode);
                   return parenNode;
               }
           } else {
               std::set<std::string> innerStop = {")"};
               auto inner = ParseBinary(parser, 0, innerStop, hasNumbers, hasStrings);
               if (!inner) {
                   Write("Parser", "Invalid parenthesized expression at line " +
                         std::to_string(nextTok.line) + ", column " +
                         std::to_string(nextTok.column), 2, true, true, "");
                   return nullptr;
               }
               if (parser.peek().type == TokenType::Delimiter && parser.peek().value == ")") {
                   parser.advance();
               } else {
                   Write("Parser", "Expected ')' at line " +
                         std::to_string(parser.peek().line) + ", column " +
                         std::to_string(parser.peek().column), 2, true, true, "");
                   return nullptr;
               }
               auto parenNode = std::make_unique<ParenNode>();
               parenNode->type = NodeType::Paren;
               parenNode->inner = std::move(inner);
               return parenNode;
           }
       }

       Write("Parser", "Unexpected token '" + tok.value + "' at line " +
             std::to_string(tok.line) + ", column " + std::to_string(tok.column),
             2, true, true, "");
       return nullptr;
   }

   std::unique_ptr<ASTNode> ParseBinary(Parser& parser, int precedence, const std::set<std::string>& stopTokens, bool& hasNumbers, bool& hasStrings) {
       auto left = ParseUnary(parser, stopTokens, hasNumbers, hasStrings);
       if (!left) return nullptr;

       while (true) {
           const Token& tok = parser.peek();
           if (stopTokens.count(tok.value)) break;
           int tokPrec = GetPrecedence(tok);
           if (tokPrec < precedence) break;

           if (hasNumbers && hasStrings) {
               Write("Parser", "Type error: cannot mix numbers and strings in binary expression at line " +
                     std::to_string(tok.line) + ", column " + std::to_string(tok.column),
                     2, true, true, "");
               return nullptr;
           }

           parser.advance();
           int nextPrec = IsRightAssociative(tok) ? tokPrec : tokPrec + 1;
           auto right = ParseBinary(parser, nextPrec, stopTokens, hasNumbers, hasStrings);
           
           if (!right) {
               Write("Parser", "Invalid right-hand side in binary expression at line " +
                     std::to_string(tok.line) + ", column " + std::to_string(tok.column),
                     2, true, true, "");
               return nullptr;
           }
           
           if (hasNumbers && hasStrings) {
               Write("Parser", "Type error: cannot mix numbers and strings in binary expression at line " +
                     std::to_string(tok.line) + ", column " + std::to_string(tok.column),
                     2, true, true, "");
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
       
       if (tok.type == TokenType::Operator && tok.value == "-") {
           if (parser.peekNext().type == TokenType::Number || parser.peekNext().type == TokenType::Float) {
               parser.advance();
               Token numTok = parser.advance();
               
               hasNumbers = true;
               auto node = std::make_unique<NumberNode>();
               node->type = NodeType::Number;
               node->token = numTok;
               try {
                   node->value = -std::stod(numTok.value);
               } catch (...) {
                   Write("Parser", "Invalid numeric literal '-" + numTok.value + "' at line " +
                         std::to_string(tok.line) + ", column " + std::to_string(tok.column),
                         2, true, true, "");
                   return nullptr;
               }
               return node;
           }
       }
       
       if (tok.type == TokenType::Operator && (tok.value == "-" || tok.value == "+" || tok.value == "!" || tok.value == "~")) {
           parser.advance();
           auto node = std::make_unique<UnaryOpNode>();
           node->type = NodeType::UnaryOp;
           node->token = tok;
           node->op = tok.value;
           node->operand = ParseUnary(parser, stopTokens, hasNumbers, hasStrings);
           if (!node->operand) {
               Write("Parser", "Invalid operand for unary operator '" + tok.value +
                     "' at line " + std::to_string(tok.line) + ", column " +
                     std::to_string(tok.column), 2, true, true, "");
               return nullptr;
           }
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