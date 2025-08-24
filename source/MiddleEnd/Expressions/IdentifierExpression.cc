#include "IdentifierExpression.hh"
#include "AssignmentExpression.hh"
#include "../ParseExpression.hh"

namespace IdentifierExpression {

    std::unique_ptr<IdentifierNode> Create(Parser& parser) {
        Token tok = parser.advance();
        auto node = std::make_unique<IdentifierNode>();
        node->type = NodeType::Identifier;
        node->token = tok;
        node->name = tok.value;
        return node;
    }

    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        auto node = Create(parser);
        const Token& nextTok = parser.peek();

        if (nextTok.value == "(") {
            parser.advance();
            auto callNode = std::make_unique<FunctionCallNode>();
            callNode->name = node->name;
            callNode->type = NodeType::FunctionCall;

            while (parser.peek().value != ")") {
                callNode->arguments.push_back(Main::ParseExpression(parser, 0, {",", ")"}));
                if (parser.peek().value == ",") parser.advance();
            }

            parser.advance();
            return callNode;
        }

        if (nextTok.type == TokenType::Operator) {
            if (nextTok.value == "++" || nextTok.value == "--") {
                Token opTok = parser.advance();
                auto unaryNode = std::make_unique<UnaryOpNode>();
                unaryNode->type = NodeType::UnaryOp;
                unaryNode->token = opTok;
                unaryNode->operand = std::move(node);
                unaryNode->op = opTok.value;
                return unaryNode;
            } else if (nextTok.value == "+=" || nextTok.value == "-=" ||
                       nextTok.value == "*=" || nextTok.value == "/=" ||
                       nextTok.value == "%=" || nextTok.value == "^=") {
                Token opTok = parser.advance();
                std::set<std::string> stopTokens = {";", "\n", ")"};
                auto right = NumberExpression::ParseExpression(parser, stopTokens);
                auto compAssignNode = std::make_unique<CompoundAssignmentOpNode>();
                compAssignNode->type = NodeType::Assignment;
                compAssignNode->token = opTok;
                compAssignNode->left = std::move(node);
                compAssignNode->right = std::move(right);
                compAssignNode->op = opTok.value;
                return compAssignNode;
            }
        }

        return node;
    }

}
