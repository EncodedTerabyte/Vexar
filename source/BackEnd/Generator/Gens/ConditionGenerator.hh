#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

llvm::Value* GenerateConditionExpression(const std::unique_ptr<ASTNode>& expr, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap);
llvm::Value* GenerateCondition(ConditionNode* Node, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap);