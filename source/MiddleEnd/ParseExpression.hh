#pragma once
#include "../Token.hh"
#include "AST.hh"
#include "Expressions/VariableExpression.hh"
#include "Parser.hh"

// language features
#include "Expressions/NumberExpression.hh"
#include "Expressions/IdentifierExpression.hh"
#include "Expressions/AssignmentExpression.hh"
#include "Expressions/StringExpression.hh"
#include "Expressions/ArrayExpression.hh"

// keywords
#include "Expressions/VariableExpression.hh"
#include "Expressions/IfStatementExpression.hh"
#include "Expressions/WhileStatementExpression.hh"
#include "Expressions/ForStatementExpression.hh"
#include "Expressions/ReturnExpression.hh"
#include "Expressions/FunctionExpression.hh"
#include "Expressions/InlineExpression.hh"
// nodes
#include "Nodes/BlockNode.hh"
#include "Nodes/ConditionNode.hh"
//#include "Nodes/ParenNode.hh"

#include <memory>

namespace Main {
    std::unique_ptr<ASTNode> ParseExpression(Parser& parser, int precedence = 0, const std::set<std::string>& stopTokens = {});
}