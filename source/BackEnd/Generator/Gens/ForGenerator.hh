#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateFor(ForNode* Node, AeroIR* IR, FunctionSymbols& Methods);