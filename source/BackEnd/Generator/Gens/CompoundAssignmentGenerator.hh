#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateCompoundAssignment(CompoundAssignmentOpNode* CompoundAssign, AeroIR* IR, FunctionSymbols& Methods);