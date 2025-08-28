#include "VariableExpression.hh"
#include "../ParseExpression.hh"
#include "IdentifierExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include <set>

namespace StringExpression {
    std::unique_ptr<ASTNode> ParseSingle(Parser& parser, const std::set<std::string>& stopTokens = {}) {
        const Token& tok = parser.peek();
        if (!stopTokens.empty() && stopTokens.count(tok.value)) return nullptr;

        if (tok.type == TokenType::String) {
            parser.advance();
            auto node = std::make_unique<StringNode>();
            node->type = NodeType::String;
            node->token = tok;
            node->value = tok.value;
            return node;
        }
        else if (tok.type == TokenType::Character) {
            if (tok.value.empty()) {
                Write("Parser", "Empty character literal at line " +
                      std::to_string(tok.line) + ", column " +
                      std::to_string(tok.column), 2, true, true, "");
                return nullptr;
            }
            parser.advance();
            auto node = std::make_unique<CharacterNode>();
            node->type = NodeType::Character;
            node->token = tok;
            node->value = tok.value[0];
            return node;
        }
        else if (tok.type == TokenType::Identifier) {
            return IdentifierExpression::Parse(parser);
        }
        else if (tok.type == TokenType::Delimiter && tok.value == "(") {
            parser.advance();
            auto inner = NumberExpression::Parse(parser, 0, {")"});
            if (parser.peek().type != TokenType::Delimiter || parser.peek().value != ")") {
                Write("Parser", "Expected ')' at line " +
                      std::to_string(parser.peek().line) + ", column " +
                      std::to_string(parser.peek().column), 2, true, true, "");
                return nullptr;
            }
            parser.advance();
            auto parenNode = std::make_unique<ParenNode>();
            parenNode->type = NodeType::Paren;
            parenNode->inner = std::move(inner);
            return parenNode;
        }

        Write("Parser", "Unexpected token '" + tok.value + "' at line " +
              std::to_string(tok.line) + ", column " + std::to_string(tok.column),
              2, true, true, "");
        return nullptr;
    }

    std::unique_ptr<ASTNode> Parse(Parser& parser, int precedence, const std::set<std::string>& stopTokens) {
        auto left = ParseSingle(parser, stopTokens);
        if (!left) return nullptr;

        while (parser.peek().type == TokenType::Operator && parser.peek().value == "+") {
            if (!stopTokens.empty() && stopTokens.count(parser.peek().value)) break;
            parser.advance();
            auto right = ParseSingle(parser, stopTokens);
            if (!right) {
                Write("Parser", "Invalid string concatenation at line " +
                      std::to_string(parser.peek().line) + ", column " +
                      std::to_string(parser.peek().column), 2, true, true, "");
                return nullptr;
            }

            auto binOp = std::make_unique<BinaryOpNode>();
            binOp->type = NodeType::BinaryOp;
            binOp->op = "+";
            binOp->left = std::move(left);
            binOp->right = std::move(right);

            left = std::move(binOp);
        }

        return left;
    }
}