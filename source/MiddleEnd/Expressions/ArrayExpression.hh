#pragma once
#include "../AST.hh"
#include "../Parser.hh"
#include <memory>

namespace ArrayExpression {
    std::unique_ptr<ASTNode> Parse(Parser& parser);
}
