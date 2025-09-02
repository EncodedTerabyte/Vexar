#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

using BuiltinHandler = std::function<llvm::Value*(const std::vector<std::unique_ptr<ASTNode>>&, AeroIR*, FunctionSymbols&)>;
#define BuiltinSymbols std::unordered_map<std::string, BuiltinHandler>

void InitializeBuiltinSymbols(BuiltinSymbols& Builtins);