#include "IfGenerator.hh"
#include "BlockGenerator.hh"
#include "ConditionGenerator.hh"

llvm::Value* GenerateIf(IfNode* Node, llvm::IRBuilder<>& Builder, AllocaSymbols& AllocaMap) {
    if (!Node || Node->branches.empty()) return nullptr;

    llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();
    std::vector<llvm::BasicBlock*> conditionBBs;
    std::vector<llvm::BasicBlock*> bodyBBs;
    llvm::BasicBlock* elseBB = nullptr;
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(Builder.getContext(), "ifcont", currentFunc);

    for (size_t i = 0; i < Node->branches.size(); ++i) {
        conditionBBs.push_back(llvm::BasicBlock::Create(Builder.getContext(), "cond" + std::to_string(i), currentFunc));
        bodyBBs.push_back(llvm::BasicBlock::Create(Builder.getContext(), "body" + std::to_string(i), currentFunc));
    }

    if (Node->elseBlock) {
        elseBB = llvm::BasicBlock::Create(Builder.getContext(), "else", currentFunc);
    }

    Builder.CreateBr(conditionBBs[0]);

    for (size_t i = 0; i < Node->branches.size(); ++i) {
        const auto& branch = Node->branches[i];
        Builder.SetInsertPoint(conditionBBs[i]);
        
        llvm::Value* condValue = GenerateCondition(branch.condition.get(), Builder, AllocaMap);
        if (!condValue) return nullptr;

        llvm::BasicBlock* nextBlock = nullptr;
        if (i + 1 < Node->branches.size()) {
            nextBlock = conditionBBs[i + 1];
        } else if (elseBB) {
            nextBlock = elseBB;
        } else {
            nextBlock = mergeBB;
        }

        Builder.CreateCondBr(condValue, bodyBBs[i], nextBlock);

        Builder.SetInsertPoint(bodyBBs[i]);
        GenerateBlock(branch.block, Builder, AllocaMap);
        Builder.CreateBr(mergeBB);
    }

    if (elseBB) {
        Builder.SetInsertPoint(elseBB);
        GenerateBlock(Node->elseBlock, Builder, AllocaMap);
        Builder.CreateBr(mergeBB);
    }

    Builder.SetInsertPoint(mergeBB);

    return nullptr;
}