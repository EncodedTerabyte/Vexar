#include "ReturnExpression.hh"
#include "../ParseExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

namespace ReturnExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        parser.advance();

        auto node = std::make_unique<ReturnNode>();
        node->type = NodeType::Return;

        const Token& next = parser.peek();
        if (!(next.type == TokenType::Delimiter &&
             (next.value == ";" || next.value == "}" || next.value == "\n"))) {
            
            std::set<std::string> stopTokens = {";", "}", "\n"};
            node->value = Main::ParseExpression(parser, 0, stopTokens);
            if (!node->value) {
                Write("Parser", "Invalid return expression at line " +
                      std::to_string(next.line) + ", column " +
                      std::to_string(next.column), 2, true, true, "");
                return nullptr;
            }
        }

        return node;
    }
}
