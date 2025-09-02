#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateAssignment(AssignmentOpNode* Assign, AeroIR* IR, FunctionSymbols& Methods);