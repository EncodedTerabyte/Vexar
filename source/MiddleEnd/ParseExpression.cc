#include "ParseExpression.hh"
#include "AST.hh"
#include "Expressions/FunctionExpression.hh"
#include "Expressions/ReturnExpression.hh"
#include "Expressions/StringExpression.hh"
#include "Nodes/ParenNode.hh"
#include "../Miscellaneous/LoggerHandler/LoggerFile.hh"

std::unique_ptr<ASTNode> Main::ParseExpression(Parser& parser, int precedence, const std::set<std::string>& stopTokens) {
    const Token& tok = parser.peek();

    if (tok.type == TokenType::Number ||
        tok.type == TokenType::Float ||
        tok.type == TokenType::Identifier ||
        (tok.type == TokenType::Delimiter && tok.value == "(") ||
        (tok.type == TokenType::Operator && 
         (tok.value == "-" || tok.value == "+" || tok.value == "!" || tok.value == "~"))) {
        return NumberExpression::Parse(parser, precedence, stopTokens);
    }

    if (tok.value == "var") {
        return VariableExpression::Parse(parser);
    }
    if (tok.value == "if") {
        return IfStatementExpression::Parse(parser);
    }
    if (tok.value == "while") {
        return WhileStatementExpression::Parse(parser);
    }
    if (tok.value == "ret") {
        return ReturnExpression::Parse(parser);
    }
    if (tok.value == "func") {
        return FunctionExpression::Parse(parser);
    }
    if (tok.value == "=") {
        return AssignmentExpression::Parse(parser);
    }
    if (tok.type == TokenType::Delimiter && tok.value == "{") {
        if (parser.peek(-1).value == "=") {
            return ArrayExpression::Parse(parser, "");
        } else {
            return BlockNodeContainer::ParseBlock(parser);
        }
    }
    if (tok.type == TokenType::String || tok.type == TokenType::Character) {
        return StringExpression::Parse(parser, precedence, stopTokens);
    }
    if (tok.type == TokenType::Keyword && (tok.value == "inline" || tok.value == "always_inline")) {
        auto Node = std::make_unique<InlinePreprocessor>();
        Node->line = tok.line;
        Node->column = tok.column;
        Node->type = NodeType::InlinePreProc;
        parser.consume(TokenType::Keyword, tok.value);
        return Node;
    }
    
    Write("Parser",
          "Unexpected token '" + tok.value + "' at line " +
          std::to_string(tok.line) + ", column " + std::to_string(tok.column),
          2, true);

    parser.advance();
    return nullptr;
}
