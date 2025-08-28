#include "IdentifierExpression.hh"
#include "AssignmentExpression.hh"
#include "../ParseExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

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
                std::unique_ptr<ASTNode> arg;
                const Token& argTok = parser.peek();
                
                if (argTok.type == TokenType::String) {
                    arg = NumberExpression::ParseExpression(parser, {","});
                } else {
                    arg = Main::ParseExpression(parser, 0, {","});
                }
                
                if (!arg) {
                    Write("Parser", "Invalid function call argument for '" + node->name +
                          "' at line " + std::to_string(nextTok.line) + ", column " +
                          std::to_string(nextTok.column), 2, true, true, "");
                    return nullptr;
                }
                callNode->arguments.push_back(std::move(arg));

                if (parser.peek().value == ",") {
                    parser.advance();
                } else if (parser.peek().value != ")" && parser.peek().type != TokenType::EndOfFile) {
                    Write("Parser", "Expected ',' or ')' in function call '" + node->name +
                          "' at line " + std::to_string(parser.peek().line) +
                          ", column " + std::to_string(parser.peek().column), 2, true, true, "");
                    return nullptr;
                }
            }

            if (parser.peek().value == ")") {
                parser.advance();
            } else {
                Write("Parser", "Unclosed function call parenthesis for '" + node->name +
                      "' starting at line " + std::to_string(nextTok.line) + ", column " +
                      std::to_string(nextTok.column), 2, true, true, "");
                return nullptr;
            }

            return callNode;
        }

        if (nextTok.value == "[") {
            std::vector<std::unique_ptr<ASTNode>> indices;
            
            while (parser.peek().value == "[") {
                parser.advance();
                auto indexExpr = Main::ParseExpression(parser, 0, {});

                if (!indexExpr) {
                    Write("Parser", "Invalid array index expression for '" + node->name +
                          "' at line " + std::to_string(parser.peek().line) + ", column " +
                          std::to_string(parser.peek().column), 2, true, true, "");
                    return nullptr;
                }

                if (parser.peek().value != "]") {
                    Write("Parser", "Unclosed array access for '" + node->name +
                          "' starting at line " + std::to_string(parser.peek().line) +
                          ", column " + std::to_string(parser.peek().column), 2, true, true, "");
                    return nullptr;
                }
                parser.advance();
                
                indices.push_back(std::move(indexExpr));
            }

            if (parser.peek().value == "=") {
                parser.advance();
                auto value = Main::ParseExpression(parser, 0, {";", "\n"});
                if (!value) {
                    Write("Parser", "Invalid array assignment value for '" + node->name +
                          "' at line " + std::to_string(parser.peek().line) +
                          ", column " + std::to_string(parser.peek().column), 2, true, true, "");
                    return nullptr;
                }

                auto arrayAssignNode = std::make_unique<ArrayAssignmentNode>();
                arrayAssignNode->identifier = node->name;
                arrayAssignNode->type = NodeType::ArrayAssignment;
                
                if (indices.size() == 1) {
                    arrayAssignNode->indexExpr = std::move(indices[0]);
                } else {
                    auto arrayNode = std::make_unique<ArrayNode>();
                    arrayNode->type = NodeType::Array;
                    for (auto& idx : indices) {
                        arrayNode->elements.push_back(std::move(idx));
                    }
                    arrayAssignNode->indexExpr = std::move(arrayNode);
                }
                
                arrayAssignNode->value = std::move(value);
                return arrayAssignNode;
            } else {
                auto accessNode = std::make_unique<ArrayAccessNode>();
                accessNode->type = NodeType::ArrayAccess;
                accessNode->identifier = node->name;
                
                if (indices.size() == 1) {
                    accessNode->expr = std::move(indices[0]);
                } else {
                    auto arrayNode = std::make_unique<ArrayNode>();
                    arrayNode->type = NodeType::Array;
                    for (auto& idx : indices) {
                        arrayNode->elements.push_back(std::move(idx));
                    }
                    accessNode->expr = std::move(arrayNode);
                }
                
                return accessNode;
            }
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
                std::set<std::string> stopTokens = {";", "\n"};
                auto right = NumberExpression::ParseExpression(parser, stopTokens);
                if (!right) {
                    Write("Parser", "Invalid right-hand side in compound assignment '" +
                          opTok.value + "' for identifier '" + node->name +
                          "' at line " + std::to_string(opTok.line) +
                          ", column " + std::to_string(opTok.column), 2, true, true, "");
                    return nullptr;
                }

                auto compAssignNode = std::make_unique<CompoundAssignmentOpNode>();
                compAssignNode->type = NodeType::CompoundAssignment;
                compAssignNode->token = opTok;
                compAssignNode->left = std::move(node);
                compAssignNode->right = std::move(right);
                compAssignNode->op = opTok.value;
                return compAssignNode;
            } else if (nextTok.value == "=") {
                Token opTok = parser.advance();
                std::set<std::string> stopTokens = {";", "\n"};
                auto right = NumberExpression::ParseExpression(parser, stopTokens);
                if (!right) {
                    Write("Parser", "Invalid right-hand side in assignment for identifier '" + node->name +
                          "' at line " + std::to_string(opTok.line) +
                          ", column " + std::to_string(opTok.column), 2, true, true, "");
                    return nullptr;
                }

                auto assignNode = std::make_unique<AssignmentOpNode>();
                assignNode->type = NodeType::Assignment;
                assignNode->token = opTok;
                assignNode->left = std::move(node);
                assignNode->right = std::move(right);
                return assignNode;
            }
        }

        return node;
    }

}