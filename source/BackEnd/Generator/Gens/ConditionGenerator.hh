#pragma once

#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* ResolveIdentifier(const std::string& name, AeroIR* IR);
llvm::Value* GenerateCondition(ConditionNode* Node, AeroIR* IR, FunctionSymbols& Methods);
llvm::Value* GenerateConditionExpression(const std::unique_ptr<ASTNode>& expr, AeroIR* IR, FunctionSymbols& Methods);