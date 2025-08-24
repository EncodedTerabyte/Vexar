#include "WhileStatementExpression.hh"
#include "../Nodes/BlockNode.hh"
#include "../Nodes/ConditionNode.hh"
#include "../ParseExpression.hh"

namespace WhileStatementExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        parser.advance();

        auto whileNode = std::make_unique<WhileNode>();
        whileNode->type = NodeType::While;

        whileNode->condition = std::unique_ptr<ConditionNode>(
            static_cast<ConditionNode*>(ConditionNodeContainer::ParseCondition(parser).release())
        );

        whileNode->block = std::unique_ptr<BlockNode>(
            static_cast<BlockNode*>(BlockNodeContainer::ParseBlock(parser).release())
        );

        return whileNode;
    }
}
