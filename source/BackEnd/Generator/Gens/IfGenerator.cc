#include "IfGenerator.hh"
#include "BlockGenerator.hh"
#include "ConditionGenerator.hh"

llvm::Value* GenerateIf(IfNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& AllocaMap, FunctionSymbols& Methods) {
    if (!Node) {
        Write("If Generation", "Null IfNode provided", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    if (Node->branches.empty()) {
        Write("If Generation", "Empty branches in IfNode" + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::Function* currentFunc = Builder.GetInsertBlock()->getParent();
    if (!currentFunc) {
        Write("If Generation", "Invalid current function" + Location, 2, true, true, "");
        return nullptr;
    }
    
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(Builder.getContext(), "ifcont", currentFunc);
    
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
            Write("If Generation", "Invalid condition for branch " + std::to_string(i) + Location, 2, true, true, "");
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
        Builder.SetInsertPoint(mergeBB);
    } else {
        mergeBB->eraseFromParent();
    }

    return nullptr;
}