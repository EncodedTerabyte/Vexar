#pragma once
#include "../AST.hh"
#include "../../Token.hh"
#include "../Parser.hh"
#include <memory>
#include <map>
#include <string>

namespace BlockNodeContainer {
    std::unique_ptr<BlockNode> ParseBlock(Parser& parser);
}
