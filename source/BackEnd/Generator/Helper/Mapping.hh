#pragma once

#include "../LLVMHeader.hh"
#define FunctionSymbols std::unordered_map<std::string, llvm::Function*> 
#define AllocaSymbols std::unordered_map<std::string, llvm::AllocaInst*>