#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

void GenerateBlock(const std::unique_ptr<BlockNode>& Node, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods);