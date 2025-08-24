#pragma once
#include "../AST.hh"
#include "../../Token.hh"
#include "../Parser.hh"
#include <memory>

namespace ConditionNodeContainer {
    std::unique_ptr<ConditionNode> ParseCondition(Parser& parser);
}
