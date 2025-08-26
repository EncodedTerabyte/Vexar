#include "VariableExpression.hh"
#include "../ParseExpression.hh"
#include "../Expressions/StringExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

namespace VariableExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        parser.advance();
        Token ident = parser.peek();
        if (ident.type != TokenType::Identifier) {
            Write("Parser", "Expected identifier at line " + std::to_string(ident.line) +
                  ", column " + std::to_string(ident.column), 2, true, true, "");
            return nullptr;
        }
        parser.advance();

        auto node = std::make_unique<VariableNode>();
        node->type = NodeType::Variable;
        node->token = ident;
        node->name = ident.value;
        node->varType.name = "auto";

        const Token& nextTok = parser.peek();
        if (nextTok.type == TokenType::Delimiter && nextTok.value == ":") {
            parser.advance();
            Token typeTok = parser.peek();
            if (typeTok.type != TokenType::Identifier) {
                Write("Parser", "Expected type name at line " + std::to_string(typeTok.line) +
                      ", column " + std::to_string(typeTok.column), 2, true, true, "");
                return nullptr;
            }
            parser.advance();
            node->varType.name = typeTok.value;

            if (parser.peek().type == TokenType::Delimiter && parser.peek().value == "[") {
                parser.advance();
                node->arrayExpression = Main::ParseExpression(parser, 0, {"]"});
                if (!parser.check(TokenType::Delimiter, "]")) {
                    Write("Parser", "Expected closing ']' at line " +
                        std::to_string(parser.peek().line) + ", column " +
                        std::to_string(parser.peek().column), 2, true, true, "");
                    return nullptr;
                }
                parser.consume(TokenType::Delimiter, "]");
            }
        }

        const Token& assignTok = parser.peek();
        if (assignTok.type == TokenType::Operator && assignTok.value == "=") {
            parser.advance();
            node->value = Main::ParseExpression(parser, 0, {";", "\n"});
            if (!node->value) {
                Write("Parser", "Invalid assignment at line " +
                      std::to_string(assignTok.line) + ", column " +
                      std::to_string(assignTok.column), 2, true, true, "");
                return nullptr;
            }
        }

        return node;
    }
}
