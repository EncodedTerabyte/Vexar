#include "IfGenerator.hh"
#include "BlockGenerator.hh"
#include "ConditionGenerator.hh"

llvm::Value* GenerateIf(IfNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods) {
    if (!Node || Node->branches.empty()) return nullptr;

    llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();
    
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(Builder.getContext(), "ifcont");
    
    std::vector<llvm::BasicBlock*> conditionBBs;
    std::vector<llvm::BasicBlock*> bodyBBs;
    
    for (size_t i = 0; i < Node->branches.size(); ++i) {
        conditionBBs.push_back(llvm::BasicBlock::Create(Builder.getContext(), "cond" + std::to_string(i), currentFunc));
        bodyBBs.push_back(llvm::BasicBlock::Create(Builder.getContext(), "body" + std::to_string(i), currentFunc));
    }

    llvm::BasicBlock* elseBB = nullptr;
    if (Node->elseBlock) {
        elseBB = llvm::BasicBlock::Create(Builder.getContext(), "else", currentFunc);
    }

    Builder.CreateBr(conditionBBs[0]);

    bool mergeNeeded = false;

    for (size_t i = 0; i < Node->branches.size(); ++i) {
        const auto& branch = Node->branches[i];
        
        Builder.SetInsertPoint(conditionBBs[i]);
        llvm::Value* condValue = GenerateCondition(branch.condition.get(), Builder, AllocaMap, Methods);
        if (!condValue) {
            mergeBB->deleteValue();
            return nullptr;
        }

        llvm::BasicBlock* nextCondition = (i + 1 < Node->branches.size()) ? conditionBBs[i + 1] : 
                                         (elseBB ? elseBB : mergeBB);

        Builder.CreateCondBr(condValue, bodyBBs[i], nextCondition);

        Builder.SetInsertPoint(bodyBBs[i]);
        PushScope(AllocaMap);
        GenerateBlock(branch.block, Builder, AllocaMap, Methods);
        PopScope(AllocaMap);
        
        if (!Builder.GetInsertBlock()->getTerminator()) {
            Builder.CreateBr(mergeBB);
            mergeNeeded = true;
        }
    }

    if (elseBB) {
        Builder.SetInsertPoint(elseBB);
        PushScope(AllocaMap);
        GenerateBlock(Node->elseBlock, Builder, AllocaMap, Methods);
        PopScope(AllocaMap);
        
        if (!Builder.GetInsertBlock()->getTerminator()) {
            Builder.CreateBr(mergeBB);
            mergeNeeded = true;
        }
    } else {
        mergeNeeded = true;
    }

    if (mergeNeeded) {
        mergeBB->insertInto(currentFunc);
        Builder.SetInsertPoint(mergeBB);
    } else {
        mergeBB->deleteValue();
    }

    return nullptr;
}