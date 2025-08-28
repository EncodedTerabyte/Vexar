#pragma once
#include "../AST.hh"
#include "../../Token.hh"
#include "../Parser.hh"
#include <memory>
#include <map>
#include <string>

namespace ForStatementExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser);
}
