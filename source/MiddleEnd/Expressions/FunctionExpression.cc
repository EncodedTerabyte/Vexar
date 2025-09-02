#include "FunctionExpression.hh"
#include "../Nodes/BlockNode.hh"
#include "../ParseExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"

namespace FunctionExpression {

    std::unique_ptr<ASTNode> Parse(Parser& parser) {
        Token inline_q = parser.peek(-1);

        parser.advance();

        Token nameTok = parser.peek();
        if (nameTok.type != TokenType::Identifier) {
            Write("Parser", "Expected function name, got '" + nameTok.value +
                  "' at line " + std::to_string(nameTok.line) +
                  ", column " + std::to_string(nameTok.column), 2, true, true, "");
            return nullptr;
        }
        parser.advance();

        auto funcNode = std::make_unique<FunctionNode>();
        funcNode->type = NodeType::Function;
        funcNode->name = nameTok.value;

        funcNode->isInlined = false;
        funcNode->alwaysInline = false;

        if (inline_q.value == "inline") {
            funcNode->isInlined = true;
        } else if (inline_q.value == "always_inline") {
            funcNode->alwaysInline = true;
        }

        if (parser.peek().value == "(") {
            parser.advance();
            while (parser.peek().value != ")" && parser.peek().type != TokenType::EndOfFile) {
                Token paramName = parser.peek();
                if (paramName.type != TokenType::Identifier) {
                    Write("Parser", "Expected parameter name, got '" + paramName.value +
                          "' at line " + std::to_string(paramName.line) +
                          ", column " + std::to_string(paramName.column), 2, true, true, "");
                    return nullptr;
                }
                parser.advance();

                if (parser.peek().value != ":") {
                    Write("Parser", "Expected ':' after parameter '" + paramName.value +
                          "' at line " + std::to_string(parser.peek().line) +
                          ", column " + std::to_string(parser.peek().column), 2, true, true, "");
                    return nullptr;
                }
                parser.advance();

                Token paramType = parser.peek();
                if (paramType.type != TokenType::Identifier) {
                    Write("Parser", "Expected type for parameter '" + paramName.value +
                          "', got '" + paramType.value + "' at line " +
                          std::to_string(paramType.line) + ", column " +
                          std::to_string(paramType.column), 2, true, true, "");
                    return nullptr;
                }
                
                std::string typeString = paramType.value;
                parser.advance();

                int dimensions = 0;
                while (parser.peek().value == "[") {
                    if (parser.peekNext().value == "]") {
                        parser.advance();
                        parser.advance();
                        typeString += "[]";
                        dimensions++;
                        if (dimensions > 2) {
                            Write("Parser", "Arrays support maximum 2 dimensions, got " + std::to_string(dimensions) + 
                                  " at line " + std::to_string(parser.peek(-1).line), 2, true, true, "");
                            return nullptr;
                        }
                    } else {
                        break;
                    }
                }

                funcNode->params.push_back({paramName.value, typeString, dimensions});

                if (parser.peek().value == ",") {
                    parser.advance();
                } else if (parser.peek().value != ")") {
                    Write("Parser", "Expected ',' or ')' in parameter list, got '" +
                          parser.peek().value + "' at line " +
                          std::to_string(parser.peek().line) + ", column " +
                          std::to_string(parser.peek().column), 2, true, true, "");
                    return nullptr;
                }
            }

            if (parser.peek().value == ")") {
                parser.advance();
            } else {
                Write("Parser", "Unclosed parameter list for function '" + funcNode->name +
                      "' starting at line " + std::to_string(nameTok.line) +
                      ", column " + std::to_string(nameTok.column), 2, true, true, "");
                return nullptr;
            }
        }

        if (parser.peek().value == ":") {
            parser.advance();
            Token returnType = parser.peek();
            if (returnType.type != TokenType::Identifier) {
                Write("Parser", "Expected return type, got '" + returnType.value +
                      "' at line " + std::to_string(returnType.line) + ", column " +
                      std::to_string(returnType.column), 2, true, true, "");
                return nullptr;
            }
            
            std::string returnTypeString = returnType.value;
            parser.advance();
            
            int returnDimensions = 0;
            while (parser.peek().value == "[") {
                if (parser.peekNext().value == "]") {
                    parser.advance();
                    parser.advance();
                    returnTypeString += "[]";
                    returnDimensions++;
                    if (returnDimensions > 2) {
                        Write("Parser", "Return type arrays support maximum 2 dimensions, got " + std::to_string(returnDimensions) + 
                              " at line " + std::to_string(parser.peek(-1).line), 2, true, true, "");
                        return nullptr;
                    }
                } else {
                    break;
                }
            }
            
            funcNode->returnType = returnTypeString;
        }

        funcNode->body = std::unique_ptr<BlockNode>(
            static_cast<BlockNode*>(BlockNodeContainer::ParseBlock(parser).release())
        );
        if (!funcNode->body) {
            Write("Parser", "Missing function body for '" + funcNode->name +
                  "' at line " + std::to_string(nameTok.line) +
                  ", column " + std::to_string(nameTok.column), 2, true, true, "");
            return nullptr;
        }

        return funcNode;
    }

}