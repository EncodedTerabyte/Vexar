#include "ArrayExpression.hh"
#include "../ParseExpression.hh"

namespace ArrayExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        const Token& tok = parser.peek();
        if (tok.type != TokenType::Delimiter || tok.value != "{") return nullptr;

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
            if (!element) break;
            arrayNode->elements.push_back(std::move(element));

            if (parser.peek().type == TokenType::Delimiter && parser.peek().value == ",") {
                parser.advance();
                continue;
            }

            if (parser.peek().type == TokenType::Delimiter && parser.peek().value == "}") {
                parser.advance();
                break;
            }

            break;
        }

        return arrayNode;
    }
}
