#include "WhileStatementExpression.hh"
#include "../Nodes/BlockNode.hh"
#include "../Nodes/ConditionNode.hh"
#include "../ParseExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

namespace WhileStatementExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        Token whileTok = parser.advance();

        auto whileNode = std::make_unique<WhileNode>();
        whileNode->type = NodeType::While;

        auto cond = ConditionNodeContainer::ParseCondition(parser);
        if (!cond) {
            Write("Parser", "Expected condition after 'while' at line " +
                  std::to_string(whileTok.line) + ", column " +
                  std::to_string(whileTok.column), 2, true, true, "");
            return nullptr;
        }
        whileNode->condition = std::unique_ptr<ConditionNode>(
            static_cast<ConditionNode*>(cond.release())
        );

        auto block = BlockNodeContainer::ParseBlock(parser);
        if (!block) {
            Write("Parser", "Expected block after 'while' condition at line " +
                  std::to_string(whileTok.line) + ", column " +
                  std::to_string(whileTok.column), 2, true, true, "");
            return nullptr;
        }
        whileNode->block = std::unique_ptr<BlockNode>(
            static_cast<BlockNode*>(block.release())
        );

        return whileNode;
    }
}
