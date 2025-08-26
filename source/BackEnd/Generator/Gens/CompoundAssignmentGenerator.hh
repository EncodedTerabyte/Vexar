#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateCompoundAssignment(CompoundAssignmentOpNode*, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods);