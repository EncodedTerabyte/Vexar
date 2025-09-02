#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateUnaryAssignment(UnaryOpNode* UnaryOp, AeroIR* IR, FunctionSymbols& Methods);