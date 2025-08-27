#include "ConditionNode.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../Expressions/NumberExpression.hh"
#include "ParenNode.hh"
#include <map>
#include <memory>
#include <string>

namespace ConditionNodeContainer {
    std::unique_ptr<ConditionNode> ParseCondition(Parser& parser) {
        std::set<std::string> stopTokens = {")", "{", "&&", "||"};
        auto expr = NumberExpression::ParseExpression(parser, stopTokens);
        if (!expr) {
            const Token& badTok = parser.peek();
            Write("Condition Expression",
                  "Invalid condition at line " + std::to_string(badTok.line) +
                  ", column " + std::to_string(badTok.column),
                  2, true, true, "");
            return nullptr;
        }
        auto condNode = std::make_unique<ConditionNode>();
        condNode->type = NodeType::Condition;
        condNode->token = parser.peek(-1);
        condNode->expression = std::move(expr);
        return condNode;
    }
}