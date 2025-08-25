#pragma once

#include "../LLVMHeader.hh"
#define FunctionSymbols std::unordered_map<std::string, llvm::Function*> 
#define AllocaSymbols std::unordered_map<std::string, llvm::AllocaInst*>

using ScopeStack = std::vector<AllocaSymbols>;

void PushScope(ScopeStack& Stack);
void PopScope(ScopeStack& Stack);
llvm::AllocaInst* FindInScopes(ScopeStack& Stack, const std::string& Name);