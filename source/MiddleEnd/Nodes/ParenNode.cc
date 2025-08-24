#include "ParenNode.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../ParseExpression.hh"

namespace ParenNodeContainer {

    std::unique_ptr<ParenNode> ParseParent(Parser& parser) {
        parser.consume(TokenType::Delimiter, "(");

        auto node = std::make_unique<ParenNode>();
        node->inner = Main::ParseExpression(parser, 0, {")"});
        node->type = NodeType::Paren;

        parser.consume(TokenType::Delimiter, ")");

        return node;
    }

}
