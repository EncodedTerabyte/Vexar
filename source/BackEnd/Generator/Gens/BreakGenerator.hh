#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"
#include <stack>

extern std::stack<llvm::BasicBlock*> LoopExitStack;

llvm::Value* GenerateBreak(const BreakNode* Node, AeroIR* IR, FunctionSymbols& Methods);