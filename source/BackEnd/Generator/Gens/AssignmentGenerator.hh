#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateAssignment(AssignmentOpNode* Assign, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods);