#include "BlockNode.hh"

#include "../ParseExpression.hh"

namespace BlockNodeContainer {
    std::unique_ptr<BlockNode> ParseBlock(Parser& parser) {
        parser.consume(TokenType::Delimiter, "{");
        auto block = std::make_unique<BlockNode>();
        block->type = NodeType::Block;
        
        while (!parser.isAtEnd() && parser.peek().value != "}") {
            block->statements.push_back(Main::ParseExpression(parser));
        }
        
        parser.consume(TokenType::Delimiter, "}");
        return block;
    }
}
