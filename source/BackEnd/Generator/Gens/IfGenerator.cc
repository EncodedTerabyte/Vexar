#include "IfGenerator.hh"
#include "BlockGenerator.hh"
#include "ConditionGenerator.hh"

llvm::Value* GenerateIf(IfNode* Node, AeroIR* IR, FunctionSymbols& Methods) {
    if (!Node) {
        Write("If Generation", "Null IfNode provided", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    if (Node->branches.empty()) {
        Write("If Generation", "Empty branches in IfNode" + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::Function* currentFunc = IR->getBuilder()->GetInsertBlock()->getParent();
    if (!currentFunc) {
        Write("If Generation", "Invalid current function" + Location, 2, true, true, "");
        return nullptr;
    }
    
    llvm::BasicBlock* mergeBB = IR->createBlock("ifcont");
    
    std::vector<llvm::BasicBlock*> conditionBBs;
    std::vector<llvm::BasicBlock*> bodyBBs;
    
    for (size_t i = 0; i < Node->branches.size(); ++i) {
        conditionBBs.push_back(IR->createBlock("cond" + std::to_string(i)));
        bodyBBs.push_back(IR->createBlock("body" + std::to_string(i)));
    }

    llvm::BasicBlock* elseBB = nullptr;
    if (Node->elseBlock) {
        elseBB = IR->createBlock("else");
    }

    IR->branch(conditionBBs[0]);

    bool mergeNeeded = false;

    for (size_t i = 0; i < Node->branches.size(); ++i) {
        const auto& branch = Node->branches[i];
        
        IR->setInsertPoint(conditionBBs[i]);
        llvm::Value* condValue = GenerateCondition(branch.condition.get(), IR, Methods);
        if (!condValue) {
            Write("If Generation", "Invalid condition for branch " + std::to_string(i) + Location, 2, true, true, "");
            return nullptr;
        }

        llvm::BasicBlock* nextCondition = (i + 1 < Node->branches.size()) ? conditionBBs[i + 1] : 
                                         (elseBB ? elseBB : mergeBB);

        IR->condBranch(condValue, bodyBBs[i], nextCondition);

        IR->setInsertPoint(bodyBBs[i]);
        IR->pushScope();
        GenerateBlock(branch.block, IR, Methods);
        IR->popScope();
        
        if (!IR->getBuilder()->GetInsertBlock()->getTerminator()) {
            IR->branch(mergeBB);
            mergeNeeded = true;
        }
    }

    if (elseBB) {
        IR->setInsertPoint(elseBB);
        IR->pushScope();
        GenerateBlock(Node->elseBlock, IR, Methods);
        IR->popScope();
        
        if (!IR->getBuilder()->GetInsertBlock()->getTerminator()) {
            IR->branch(mergeBB);
            mergeNeeded = true;
        }
    } else {
        mergeNeeded = true;
    }

    if (mergeNeeded) {
        IR->setInsertPoint(mergeBB);
    } else {
        mergeBB->eraseFromParent();
    }

    return nullptr;
}