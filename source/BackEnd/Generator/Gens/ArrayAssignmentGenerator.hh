#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateArrayAssignment(ArrayAssignmentNode* ArrayAssign, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods);