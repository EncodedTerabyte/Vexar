#include "WhileGenerator.hh"
#include "BlockGenerator.hh"
#include "ConditionGenerator.hh"
#include "BreakGenerator.hh"

llvm::Value* GenerateWhile(WhileNode* Node, AeroIR* IR, FunctionSymbols& Methods) {
    if (!Node) {
        Write("While Generator", "Null WhileNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::BasicBlock* LoopHeader = IR->createBlock("while.header");
    llvm::BasicBlock* LoopBody = IR->createBlock("while.body");
    llvm::BasicBlock* LoopExit = IR->createBlock("while.exit");

    IR->branch(LoopHeader);
    IR->setInsertPoint(LoopHeader);

    llvm::Value* Condition = GenerateCondition(Node->condition.get(), IR, Methods);
    if (!Condition) {
        Write("While Generator", "Failed to generate while condition" + Location, 2, true, true, "");
        return nullptr;
    }

    IR->condBranch(Condition, LoopBody, LoopExit);

    IR->setInsertPoint(LoopBody);
    LoopExitStack.push(LoopExit);
    GenerateBlock(Node->block, IR, Methods);
    LoopExitStack.pop();

    llvm::BasicBlock* currentBlock = IR->getBuilder()->GetInsertBlock();
    if (!currentBlock->getTerminator()) {
        IR->branch(LoopHeader);
    }

    IR->setInsertPoint(LoopExit);
    return IR->constI32(0);
}