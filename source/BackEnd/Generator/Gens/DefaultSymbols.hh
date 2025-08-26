#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

using BuiltinHandler = std::function<llvm::Value*(const std::vector<std::unique_ptr<ASTNode>>&, llvm::IRBuilder<>&, ScopeStack&, FunctionSymbols&)>;
#define BuiltinSymbols std::unordered_map<std::string, BuiltinHandler>

void InitializeBuiltinSymbols(BuiltinSymbols& Builtins);