#include "ForStatementExpression.hh"
#include "../Nodes/BlockNode.hh"
#include "../Nodes/ConditionNode.hh"
#include "../ParseExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

namespace ForStatementExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        Token forTok = parser.advance();

        auto forNode = std::make_unique<ForNode>();
        forNode->type = NodeType::For;

        if (parser.peek().value != "(") {
            Write("Parser", "Expected '(' after 'for' at line " +
                  std::to_string(forTok.line) + ", column " +
                  std::to_string(forTok.column), 2, true, true, "");
            return nullptr;
        }
        parser.advance();

        if (parser.peek().value != ";") {
            forNode->init = Main::ParseExpression(parser);
            if (!forNode->init) {
                Write("Parser", "Failed to parse init expression in for loop at line " +
                      std::to_string(forTok.line) + ", column " +
                      std::to_string(forTok.column), 2, true, true, "");
                return nullptr;
            }
        }

        if (parser.peek().value != ";") {
            Write("Parser", "Expected ';' after for init at line " +
                  std::to_string(parser.peek().line) + ", column " +
                  std::to_string(parser.peek().column), 2, true, true, "");
            return nullptr;
        }
        parser.advance();

        if (parser.peek().value != ";") {
            auto cond = ConditionNodeContainer::ParseCondition(parser);
            if (!cond) {
                Write("Parser", "Failed to parse condition in for loop at line " +
                      std::to_string(forTok.line) + ", column " +
                      std::to_string(forTok.column), 2, true, true, "");
                return nullptr;
            }
            forNode->condition = std::unique_ptr<ConditionNode>(
                static_cast<ConditionNode*>(cond.release())
            );
        }

        if (parser.peek().value != ";") {
            Write("Parser", "Expected ';' after for condition at line " +
                  std::to_string(parser.peek().line) + ", column " +
                  std::to_string(parser.peek().column), 2, true, true, "");
            return nullptr;
        }
        parser.advance();

        if (parser.peek().value != ")") {
            forNode->increment = Main::ParseExpression(parser);
            if (!forNode->increment) {
                Write("Parser", "Failed to parse increment expression in for loop at line " +
                      std::to_string(forTok.line) + ", column " +
                      std::to_string(forTok.column), 2, true, true, "");
                return nullptr;
            }
        }

        if (parser.peek().value != ")") {
            Write("Parser", "Expected ')' after for increment at line " +
                  std::to_string(parser.peek().line) + ", column " +
                  std::to_string(parser.peek().column), 2, true, true, "");
            return nullptr;
        }
        parser.advance();

        auto block = BlockNodeContainer::ParseBlock(parser);
        if (!block) {
            Write("Parser", "Expected block after for statement at line " +
                  std::to_string(forTok.line) + ", column " +
                  std::to_string(forTok.column), 2, true, true, "");
            return nullptr;
        }
        forNode->body = std::unique_ptr<BlockNode>(
            static_cast<BlockNode*>(block.release())
        );

        return forNode;
    }
}