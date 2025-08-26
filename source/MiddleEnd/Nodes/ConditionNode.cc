#include "ConditionNode.hh"
#include "../../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../ParseExpression.hh"
#include "ParenNode.hh"
#include <map>
#include <memory>
#include <string>

static int GetConditionPrecedence(const Token& tok) {
    static std::map<std::string, int> prec = {
        {"||", 1}, {"&&", 2},
        {"==", 3}, {"!=", 3},
        {"<", 4}, {"<=", 4}, {">", 4}, {">=", 4}
    };
    if (tok.type == TokenType::Operator && prec.count(tok.value))
        return prec.at(tok.value);
    return -1;
}

static bool IsRightAssociativeCondition(const Token& tok) {
    return false;
}

static std::unique_ptr<ASTNode> ParseConditionBinary(Parser& parser, int precedence) {
    std::unique_ptr<ASTNode> left;
    const Token& tok = parser.peek();

    if (tok.type == TokenType::Delimiter && tok.value == "(") {
        parser.advance();
        left = ParseConditionBinary(parser, 0);
        if (parser.peek().type != TokenType::Delimiter || parser.peek().value != ")") {
            Write("Condition Expression",
                  "Expected ')' at line " + std::to_string(tok.line) +
                  ", column " + std::to_string(tok.column),
                  2, true);
        } else {
            parser.advance();
        }
    } else if (tok.type == TokenType::Keyword && (tok.value == "true" || tok.value == "false")) {
        parser.advance();
        left = std::make_unique<BooleanNode>(tok.value == "true");
    } else if (tok.type == TokenType::Identifier || tok.type == TokenType::Number || tok.type == TokenType::Float ||
               (tok.type == TokenType::Delimiter && tok.value == "(") ||
               (tok.type == TokenType::Operator && (tok.value == "-" || tok.value == "+" || tok.value == "!" || tok.value == "~"))) {
        left = Main::ParseExpression(parser, 0, {")"});
    } else {
        Write("Condition Expression",
              "Unexpected token '" + tok.value + "' at line " +
              std::to_string(tok.line) + ", column " +
              std::to_string(tok.column),
              2, true);
        parser.advance();
        return nullptr;
    }

    while (true) {
        const Token& nextTok = parser.peek();
        int tokPrec = GetConditionPrecedence(nextTok);
        if (tokPrec < precedence) break;

        parser.advance();
        int nextPrec = IsRightAssociativeCondition(nextTok) ? tokPrec : tokPrec + 1;
        auto right = ParseConditionBinary(parser, nextPrec);

        if (!right) {
            Write("Condition Expression",
                  "Missing right-hand side of operator '" + nextTok.value +
                  "' at line " + std::to_string(nextTok.line) +
                  ", column " + std::to_string(nextTok.column),
                  2, true);
            return nullptr;
        }

        auto binNode = std::make_unique<BinaryOpNode>();
        binNode->type = NodeType::BinaryOp;
        binNode->token = nextTok;
        binNode->op = nextTok.value;
        binNode->left = std::move(left);
        binNode->right = std::move(right);

        left = std::move(binNode);
    }

    return left;
}

namespace ConditionNodeContainer {
    std::unique_ptr<ConditionNode> ParseCondition(Parser& parser) {
        auto expr = ParseConditionBinary(parser, 0);
        if (!expr) {
            const Token& badTok = parser.peek();
            Write("Condition Expression",
                  "Invalid condition at line " + std::to_string(badTok.line) +
                  ", column " + std::to_string(badTok.column),
                  2, true);
            return nullptr;
        }
        auto condNode = std::make_unique<ConditionNode>();
        condNode->type = NodeType::Condition;
        condNode->expression = std::move(expr);
        return condNode;
    }
}
