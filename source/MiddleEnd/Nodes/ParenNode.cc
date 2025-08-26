#include "ParenNode.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../ParseExpression.hh"

namespace ParenNodeContainer {

    std::unique_ptr<ParenNode> ParseParent(Parser& parser) {
        const Token& openTok = parser.peek();
        if (openTok.type != TokenType::Delimiter || openTok.value != "(") {
            Write("Paren Expression",
                  "Expected '(' at line " + std::to_string(openTok.line) +
                  ", column " + std::to_string(openTok.column),
                  2, true);
            return nullptr;
        }
        parser.advance();

        auto node = std::make_unique<ParenNode>();
        node->inner = Main::ParseExpression(parser, 0, {")"});
        node->type = NodeType::Paren;

        const Token& closeTok = parser.peek();
        if (closeTok.type != TokenType::Delimiter || closeTok.value != ")") {
            Write("Paren Expression",
                  "Expected ')' to match '(' at line " +
                  std::to_string(openTok.line) + ", but got '" +
                  closeTok.value + "' at line " +
                  std::to_string(closeTok.line) + ", column " +
                  std::to_string(closeTok.column),
                  2, true);
            return nullptr;
        }
        parser.advance();

        return node;
    }

}
