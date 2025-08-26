#include "IfStatementExpression.hh"
#include "../Nodes/BlockNode.hh"
#include "../Nodes/ConditionNode.hh"
#include "../ParseExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

namespace IfStatementExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        const Token& ifTok = parser.peek();
        parser.advance();

        auto ifNode = std::make_unique<IfNode>();
        ifNode->type = NodeType::If;

        size_t elseIfCounter = 0;

        IfNode::Branch mainBranch;
        mainBranch.type = IfNode::BranchType::MainIf;

        auto cond = ConditionNodeContainer::ParseCondition(parser);
        if (!cond) {
            Write("Parser", "Invalid condition in if-statement at line " +
                  std::to_string(ifTok.line) + ", column " +
                  std::to_string(ifTok.column), 2, true, true, "");
            return nullptr;
        }
        mainBranch.condition = std::unique_ptr<ConditionNode>(
            static_cast<ConditionNode*>(cond.release())
        );

        auto blk = BlockNodeContainer::ParseBlock(parser);
        if (!blk) {
            Write("Parser", "Missing block in if-statement at line " +
                  std::to_string(ifTok.line) + ", column " +
                  std::to_string(ifTok.column), 2, true, true, "");
            return nullptr;
        }
        mainBranch.block = std::unique_ptr<BlockNode>(
            static_cast<BlockNode*>(blk.release())
        );

        ifNode->branches.push_back(std::move(mainBranch));

        while (parser.peek().value == "else") {
            const Token& elseTok = parser.peek();
            parser.advance();
            if (parser.peek().value == "if") {
                parser.advance();

                IfNode::Branch elseIfBranch;
                elseIfBranch.type = IfNode::BranchType::ElseIf;
                elseIfBranch.order = ++elseIfCounter;

                auto elseIfCond = ConditionNodeContainer::ParseCondition(parser);
                if (!elseIfCond) {
                    Write("Parser", "Invalid condition in else-if at line " +
                          std::to_string(elseTok.line) + ", column " +
                          std::to_string(elseTok.column), 2, true, true, "");
                    return nullptr;
                }
                elseIfBranch.condition = std::unique_ptr<ConditionNode>(
                    static_cast<ConditionNode*>(elseIfCond.release())
                );

                auto elseIfBlk = BlockNodeContainer::ParseBlock(parser);
                if (!elseIfBlk) {
                    Write("Parser", "Missing block in else-if at line " +
                          std::to_string(elseTok.line) + ", column " +
                          std::to_string(elseTok.column), 2, true, true, "");
                    return nullptr;
                }
                elseIfBranch.block = std::unique_ptr<BlockNode>(
                    static_cast<BlockNode*>(elseIfBlk.release())
                );

                ifNode->branches.push_back(std::move(elseIfBranch));
            } else {
                auto elseBlk = BlockNodeContainer::ParseBlock(parser);
                if (!elseBlk) {
                    Write("Parser", "Missing block in else-statement at line " +
                          std::to_string(elseTok.line) + ", column " +
                          std::to_string(elseTok.column), 2, true, true, "");
                    return nullptr;
                }
                ifNode->elseBlock = std::unique_ptr<BlockNode>(
                    static_cast<BlockNode*>(elseBlk.release())
                );
                break;
            }
        }

        return ifNode;
    }
}
