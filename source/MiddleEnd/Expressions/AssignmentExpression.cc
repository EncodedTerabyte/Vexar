#include "AssignmentExpression.hh"
#include "../ParseExpression.hh"

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

            auto assignNode = std::make_unique<AssignmentOpNode>();
            assignNode->type = NodeType::Assignment;
            assignNode->token = tok;
            assignNode->left = std::move(left);
            assignNode->right = std::move(right);
            return assignNode;
        }

        return left;
    }
}
