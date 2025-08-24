#include "IfStatementExpression.hh"
#include "../Nodes/BlockNode.hh"
#include "../Nodes/ConditionNode.hh"
#include "../ParseExpression.hh"

namespace IfStatementExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        parser.advance();

        auto ifNode = std::make_unique<IfNode>();
        ifNode->type = NodeType::If;

        size_t elseIfCounter = 0;
        
        IfNode::Branch mainBranch;
        mainBranch.type = IfNode::BranchType::MainIf;
        mainBranch.condition = std::unique_ptr<ConditionNode>(
            static_cast<ConditionNode*>(ConditionNodeContainer::ParseCondition(parser).release())
        );
        mainBranch.block = std::unique_ptr<BlockNode>(
            static_cast<BlockNode*>(BlockNodeContainer::ParseBlock(parser).release())
        );
        ifNode->branches.push_back(std::move(mainBranch));

        while (parser.peek().value == "else") {
            parser.advance();
            if (parser.peek().value == "if") {
                parser.advance();
                IfNode::Branch elseIfBranch;
                elseIfBranch.type = IfNode::BranchType::ElseIf;
                elseIfBranch.order = ++elseIfCounter;
                elseIfBranch.condition = std::unique_ptr<ConditionNode>(
                    static_cast<ConditionNode*>(ConditionNodeContainer::ParseCondition(parser).release())
                );
                elseIfBranch.block = std::unique_ptr<BlockNode>(
                    static_cast<BlockNode*>(BlockNodeContainer::ParseBlock(parser).release())
                );
                ifNode->branches.push_back(std::move(elseIfBranch));
            } else {
                ifNode->elseBlock = std::unique_ptr<BlockNode>(
                    static_cast<BlockNode*>(BlockNodeContainer::ParseBlock(parser).release())
                );
                break;
            }
        }

        return ifNode;
    }
}
