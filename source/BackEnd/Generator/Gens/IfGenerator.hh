#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

bool ProcessBranches(IfNode* Node, const std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>>& branchBBs,
                    llvm::BasicBlock* elseBB, llvm::BasicBlock* mergeBB, llvm::IRBuilder<>& Builder, 
                    ScopeStack& AllocaMap, FunctionSymbols& Methods);
bool ProcessElseBranch(const std::unique_ptr<BlockNode>& elseBlock, llvm::BasicBlock* elseBB,
                      llvm::BasicBlock* mergeBB, llvm::IRBuilder<>& Builder, 
                      ScopeStack& AllocaMap, FunctionSymbols& Methods);
void FinalizeMergeBlock(llvm::BasicBlock* mergeBB, bool mergeNeeded, llvm::Function* currentFunc, llvm::IRBuilder<>& Builder);
llvm::Value* GenerateIf(IfNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods);