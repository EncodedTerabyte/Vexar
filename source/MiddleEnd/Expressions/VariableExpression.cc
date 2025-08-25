#include "VariableExpression.hh"
#include "../ParseExpression.hh"
#include "../Expressions/StringExpression.hh"

namespace VariableExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        parser.advance();
        Token ident = parser.peek();
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
            parser.advance();
            node->varType.name = typeTok.value;

            if (parser.peek().type == TokenType::Delimiter && parser.peek().value == "[") {
                parser.advance(); // consume [

                node->arrayExpression = Main::ParseExpression(parser, 0, {"]"});
                parser.consume(TokenType::Delimiter, "]");
            }

        }

        const Token& assignTok = parser.peek();
        if (assignTok.type == TokenType::Operator && assignTok.value == "=") {
            parser.advance();
            node->value = Main::ParseExpression(parser, 0, {";", "\n"});
        }

        return node;
    }
}
