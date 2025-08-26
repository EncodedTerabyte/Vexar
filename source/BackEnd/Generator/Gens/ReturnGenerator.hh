#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateReturn(const ReturnNode* Ret, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods);