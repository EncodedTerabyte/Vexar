#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Function* GenerateFunction(FunctionNode* Node, AeroIR* IR, FunctionSymbols& Methods);