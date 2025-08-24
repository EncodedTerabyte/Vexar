#pragma once
#include "../AST.hh"
#include "../../Token.hh"
#include "../Parser.hh"
#include <memory>
#include <map>
#include <string>

namespace IdentifierExpression {
    std::unique_ptr<IdentifierNode> Create(Parser& parser);
    std::unique_ptr<ASTNode> Parse(Parser& parser);
}
