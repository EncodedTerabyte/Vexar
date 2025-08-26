#include "ArrayExpression.hh"
#include "../ParseExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

namespace ArrayExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        const Token& tok = parser.peek();
        if (tok.type != TokenType::Delimiter || tok.value != "{") {
            Write("Parser", "Expected '{' at line " + std::to_string(tok.line) +
                  ", column " + std::to_string(tok.column), 2, true, true, "");
            return nullptr;
        }

        parser.advance();
        auto arrayNode = std::make_unique<ArrayNode>();
        arrayNode->type = NodeType::Array;
        arrayNode->token = tok;

        while (true) {
            if (parser.peek().type == TokenType::Delimiter && parser.peek().value == "}") {
                parser.advance();
                break;
            }

            auto element = Main::ParseExpression(parser, 0, {",", "}"});
            if (!element) {
                Write("Parser", "Invalid array element at line " +
                      std::to_string(parser.peek().line) + ", column " +
                      std::to_string(parser.peek().column), 2, true, true, "");
                return nullptr;
            }
            arrayNode->elements.push_back(std::move(element));

            if (parser.peek().type == TokenType::Delimiter && parser.peek().value == ",") {
                parser.advance();
                continue;
            }

            if (parser.peek().type == TokenType::Delimiter && parser.peek().value == "}") {
                parser.advance();
                break;
            }

            Write("Parser", "Unexpected token '" + parser.peek().value +
                  "' in array at line " + std::to_string(parser.peek().line) +
                  ", column " + std::to_string(parser.peek().column), 2, true, true, "");
            return nullptr;
        }

        return arrayNode;
    }
}
