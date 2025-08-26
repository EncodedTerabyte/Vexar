#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

#include "DefaultSymbols.hh"
#include "Memory/Malloc.hh"

const uint64_t HEAP_ARRAY_THRESHOLD = 1024;

llvm::Value* GenerateArrayExpression(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods);