#pragma once
#include "../AST.hh"
#include "../../Token.hh"
#include "../Parser.hh"
#include <memory>
#include <map>
#include <string>

namespace ParenNodeContainer {
    std::unique_ptr<ParenNode> ParseParent(Parser& parser);
}
