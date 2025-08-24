#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Function* GenerateFunction(FunctionNode* Node, llvm::Module* Module, AllocaSymbols& AllocaMap);