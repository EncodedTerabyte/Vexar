#pragma once
#include "../AST.hh"
#include "../Parser.hh"
#include <memory>

namespace VariableExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser);
}
