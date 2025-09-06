#include "ParseExpression.hh"
#include "../Miscellaneous/LoggerHandler/LoggerFile.hh"

std::unique_ptr<ASTNode> Main::ParseExpression(Parser& parser, int precedence, const std::set<std::string>& stopTokens) {
    const Token& tok = parser.peek();

    if (stopTokens.count(tok.value)) {
        return nullptr;
    }
    
    if (tok.value == "break") {
        auto Node = std::make_unique<BreakNode>();
        Node->token = tok;
        parser.advance();
        return Node; 
    }

    if (tok.type == TokenType::Number ||
        tok.type == TokenType::Float ||
        tok.type == TokenType::String ||
        tok.type == TokenType::Character ||
        (tok.type == TokenType::Delimiter && tok.value == "(") ||
        (tok.type == TokenType::Operator && 
         (tok.value == "-" || tok.value == "+" || tok.value == "!" || tok.value == "~"))) {
        return NumberExpression::Parse(parser, precedence, stopTokens);
    }

    if ((tok.value == "true" || tok.value == "false")) {
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
    if (tok.value == "for") {
        return ForStatementExpression::Parse(parser);
    }
    if (tok.value == "ret" || tok.value == "return") {
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
    if (tok.type == TokenType::Keyword && (tok.value == "inline" || tok.value == "always_inline")) {
        return InlineExpression::Parse(parser);
    }
    if (tok.value == ";") {
        auto Node = std::make_unique<SemiColonNode>();
        Node->type = NodeType::SemiColon;
        Node->line = tok.line;
        Node->column = tok.column;
        parser.advance();
        return Node;
    }
    
    
    if (tok.type == TokenType::Identifier) {
        return IdentifierExpression::Parse(parser);
    }

    Write("Parser",
          "Unexpected token '" + tok.value + "' at line " +
          std::to_string(tok.line) + ", column " + std::to_string(tok.column),
          2, true, true, "");

    return nullptr;
}