#include "BlockNode.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../ParseExpression.hh"

namespace BlockNodeContainer {
    std::unique_ptr<BlockNode> ParseBlock(Parser& parser) {
        const Token& openTok = parser.peek();
        if (openTok.type != TokenType::Delimiter || openTok.value != "{") {
            Write("Block Expression",
                  "Expected '{' at line " + std::to_string(openTok.line) +
                  ", column " + std::to_string(openTok.column),
                  2, true);
            return nullptr;
        }
        parser.advance();

        auto block = std::make_unique<BlockNode>();
        block->type = NodeType::Block;

        while (!parser.isAtEnd() && parser.peek().value != "}") {
            auto stmt = Main::ParseExpression(parser);
            if (!stmt) {
                Write("Block Expression",
                      "Invalid statement inside block at line " +
                      std::to_string(parser.peek().line) + ", column " +
                      std::to_string(parser.peek().column),
                      2, true);
                parser.advance(); 
                continue;
            }
            block->statements.push_back(std::move(stmt));
        }

        const Token& closeTok = parser.peek();
        if (closeTok.type != TokenType::Delimiter || closeTok.value != "}") {
            Write("Block Expression",
                  "Expected '}' to match '{' at line " +
                  std::to_string(openTok.line) + ", but got '" +
                  closeTok.value + "' at line " +
                  std::to_string(closeTok.line) + ", column " +
                  std::to_string(closeTok.column),
                  2, true);
            return nullptr;
        }
        parser.advance();

        return block;
    }
}
