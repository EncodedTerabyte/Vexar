#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

void GenerateBlock(const std::unique_ptr<BlockNode>& Node, AeroIR* IR, FunctionSymbols& Methods);