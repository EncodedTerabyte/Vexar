#include "AssignmentExpression.hh"
#include "../ParseExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

namespace AssignmentExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        auto left = std::make_unique<IdentifierNode>();
        left->type = NodeType::Identifier;
        left->token = parser.peek(-1);
        left->name = left->token.value;

        const Token& tok = parser.peek();
        if (tok.type == TokenType::Operator && tok.value == "=") {
            parser.advance();
            auto right = Main::ParseExpression(parser);
            if (!right) {
                Write("Parser", "Expected expression after '=' for assignment to '" +
                      left->name + "' at line " + std::to_string(tok.line) +
                      ", column " + std::to_string(tok.column),
                      2, true, true, "");
                return nullptr;
            }

            auto assignNode = std::make_unique<AssignmentOpNode>();
            assignNode->type = NodeType::Assignment;
            assignNode->token = tok;
            assignNode->left = std::move(left);
            assignNode->right = std::move(right);
            return assignNode;
        }
        
        Write("Parser", "Expected '=' after identifier '" + left->name +
              "' at line " + std::to_string(tok.line) +
              ", column " + std::to_string(tok.column),
              2, true, true, "");
        return nullptr;
    }
}
