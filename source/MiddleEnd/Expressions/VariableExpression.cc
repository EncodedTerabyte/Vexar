#include "VariableExpression.hh"
#include "../ParseExpression.hh"
#include "../Expressions/StringExpression.hh"
#include "../Expressions/ArrayExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

namespace VariableExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        parser.advance();
        Token ident = parser.peek();
        if (ident.type != TokenType::Identifier) {
            Write("Parser", "Expected identifier at line " + std::to_string(ident.line) +
                  ", column " + std::to_string(ident.column), 2, true, true, "");
            return nullptr;
        }
        parser.advance();

        auto node = std::make_unique<VariableNode>();
        node->type = NodeType::Variable;
        node->token = ident;
        node->name = ident.value;
        node->varType.name = "auto";

        const Token& nextTok = parser.peek();
        if (nextTok.type == TokenType::Delimiter && nextTok.value == ":") {
            parser.advance();
            Token typeTok = parser.peek();
            if (typeTok.type != TokenType::Identifier) {
                Write("Parser", "Expected type name at line " + std::to_string(typeTok.line) +
                      ", column " + std::to_string(typeTok.column), 2, true, true, "");
                return nullptr;
            }
            parser.advance();
            node->varType.name = typeTok.value;

            std::string fullTypeName = typeTok.value;
            
            while (parser.peek().type == TokenType::Delimiter && parser.peek().value == "[") {
                parser.advance();
                
                // Check if this is an empty bracket [] (type declaration) or has content (sized array)
                if (parser.peek().type == TokenType::Delimiter && parser.peek().value == "]") {
                    // Empty brackets - this is just a type declaration like int[]
                    parser.advance();
                    fullTypeName += "[]";
                } else {
                    // Has content - this is a sized array declaration like int[5]
                    auto dimExpr = Main::ParseExpression(parser, 0, {"]"});
                    if (!dimExpr) {
                        Write("Parser", "Expected dimension expression at line " +
                            std::to_string(parser.peek().line) + ", column " +
                            std::to_string(parser.peek().column), 2, true, true, "");
                        return nullptr;
                    }
                    
                    if (parser.peek().type != TokenType::Delimiter || parser.peek().value != "]") {
                        Write("Parser", "Expected closing ']' at line " +
                            std::to_string(parser.peek().line) + ", column " +
                            std::to_string(parser.peek().column), 2, true, true, "");
                        return nullptr;
                    }
                    parser.advance();
                    
                    // For sized arrays, store the dimension expression
                    if (!node->arrayExpression) {
                        node->arrayExpression = std::move(dimExpr);
                    } else if (node->arrayExpression->type == NodeType::Array) {
                        auto* arrayNode = static_cast<ArrayNode*>(node->arrayExpression.get());
                        arrayNode->elements.push_back(std::move(dimExpr));
                    } else {
                        auto arrayNode = std::make_unique<ArrayNode>();
                        arrayNode->type = NodeType::Array;
                        arrayNode->elements.push_back(std::move(node->arrayExpression));
                        arrayNode->elements.push_back(std::move(dimExpr));
                        node->arrayExpression = std::move(arrayNode);
                    }
                }
            }
            
            // Update the type name to include array brackets
            node->varType.name = fullTypeName;
        }

        const Token& assignTok = parser.peek();
        if (assignTok.type == TokenType::Operator && assignTok.value == "=") {
            parser.advance();
            
            if (parser.peek().type == TokenType::Delimiter && parser.peek().value == "{") {
                node->value = ArrayExpression::Parse(parser, node->varType.name);
            } else {
                node->value = Main::ParseExpression(parser, 0, {";", "\n"});
            }
            
            if (!node->value) {
                Write("Parser", "Invalid assignment at line " +
                      std::to_string(assignTok.line) + ", column " +
                      std::to_string(assignTok.column), 2, true, true, "");
                return nullptr;
            }
        }

        return node;
    }
}