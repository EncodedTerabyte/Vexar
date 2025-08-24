#include "ReturnExpression.hh"
#include "../ParseExpression.hh"
#include "../Nodes/ParenNode.hh"

namespace ReturnExpression {

    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        parser.advance();

        auto node = std::make_unique<ReturnNode>();
        node->type = NodeType::Return;

        const Token& next = parser.peek();
        if (!(next.type == TokenType::Delimiter &&
             (next.value == ";" || next.value == "}" || next.value == "\n"))) {

            if (next.type == TokenType::Delimiter && next.value == "(") {
                node->value = ParenNodeContainer::ParseParent(parser);
            } else {
                std::set<std::string> stopTokens = {";", "}", "\n"};
                node->value = Main::ParseExpression(parser, 0, stopTokens);
            }
        }

        return node;
    }
}
