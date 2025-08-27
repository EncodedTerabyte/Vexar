#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"
#include <stack>

extern std::stack<llvm::BasicBlock*> LoopExitStack;

llvm::Value* GenerateBreak(const BreakNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods);