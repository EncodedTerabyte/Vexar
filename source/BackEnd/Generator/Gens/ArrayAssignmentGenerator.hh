#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateArrayAssignment(ArrayAssignmentNode* ArrayAssign, AeroIR* IR, FunctionSymbols& Methods);