#include "NumberExpression.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "IdentifierExpression.hh"
#include "../Nodes/ParenNode.hh"
#include <map>
#include <set>
#include <memory>
#include <string>

int GetPrecedence(const Token& tok) {
    static std::map<std::string, int> prec = {
        {"=", 1}, {":=", 1}, {"==", 2}, {"!=", 2},
        {"<", 2}, {"<=", 2}, {">", 2}, {">=", 2},
        {"+", 3}, {"-", 3},
        {"*", 4}, {"/", 4}, {"%", 4},
        {"^", 5}
    };
    if (tok.type == TokenType::Operator && prec.count(tok.value))
        return prec[tok.value];
    return -1;
}

bool IsRightAssociative(const Token& tok) {
    return tok.type == TokenType::Operator && tok.value == "^";
}

namespace NumberExpression {
    std::unique_ptr<ASTNode> ParsePrimary(Parser& parser, const std::set<std::string>& stopTokens) {
        const Token& tok = parser.peek();

        if (stopTokens.count(tok.value)) return nullptr;

        if (tok.type == TokenType::Number || tok.type == TokenType::Float) {
            parser.advance();
            auto node = std::make_unique<NumberNode>();
            node->type = NodeType::Number;
            node->token = tok;
            node->value = std::stod(tok.value);
            return node;
        }

        if (tok.type == TokenType::Identifier) {
            return IdentifierExpression::Parse(parser);
        }

        if (tok.type == TokenType::Delimiter && tok.value == "(") {
            parser.advance();
            auto inner = ParseBinary(parser, 0, stopTokens); // fully parse inside
            if (parser.peek().type != TokenType::Delimiter || parser.peek().value != ")") {
                Write("Number Expression", "Expected ')'", 2, true);
            }
            parser.advance();
            auto parenNode = std::make_unique<ParenNode>();
            parenNode->type = NodeType::Paren;
            parenNode->inner = std::move(inner);
            return parenNode;
        }

        Write("Number Expression", "Unexpected token: " + tok.value, 2, true);
        return nullptr;
    }

    std::unique_ptr<ASTNode> ParseBinary(Parser& parser, int precedence, const std::set<std::string>& stopTokens) {
        auto left = ParseUnary(parser, stopTokens);
        if (!left) return nullptr;

        while (true) {
            const Token& tok = parser.peek();
            if (stopTokens.count(tok.value) || tok.value == ")") break;
            int tokPrec = GetPrecedence(tok);
            if (tokPrec < precedence) break;

            parser.advance();
            int nextPrec = IsRightAssociative(tok) ? tokPrec : tokPrec + 1;
            auto right = ParseBinary(parser, nextPrec, stopTokens);
            auto binNode = std::make_unique<BinaryOpNode>();
            binNode->type = NodeType::BinaryOp;
            binNode->token = tok;
            binNode->op = tok.value;
            binNode->left = std::move(left);
            binNode->right = std::move(right);
            left = std::move(binNode);
        }

        return left;
    }


    std::unique_ptr<ASTNode> ParseUnary(Parser& parser, const std::set<std::string>& stopTokens) {
        const Token& tok = parser.peek();
        if (tok.type == TokenType::Operator && (tok.value == "-" || tok.value == "+" || tok.value == "!" || tok.value == "~")) {
            parser.advance();
            auto node = std::make_unique<UnaryOpNode>();
            node->type = NodeType::UnaryOp;
            node->token = tok;
            node->op = tok.value;
            node->operand = ParseUnary(parser, stopTokens);
            return node;
        }
        return ParsePrimary(parser, stopTokens);
    }
    
    std::unique_ptr<ASTNode> Parse(Parser& parser, int precedence, const std::set<std::string>& stopTokens) {
        return ParseBinary(parser, precedence, stopTokens);
    }

    std::unique_ptr<ASTNode> ParseExpression(Parser& parser, const std::set<std::string>& stopTokens) {
        return Parse(parser, 0, stopTokens);
    }

}
