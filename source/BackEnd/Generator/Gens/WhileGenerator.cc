#include "WhileGenerator.hh"
#include "BlockGenerator.hh"
#include "ConditionGenerator.hh"
#include "BreakGenerator.hh"

llvm::Value* GenerateWhile(WhileNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Node) {
        Write("While Generator", "Null WhileNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::Function* CurrentFunction = Builder.GetInsertBlock()->getParent();
    if (!CurrentFunction) {
        Write("While Generator", "No current function for while loop" + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::BasicBlock* LoopHeader = llvm::BasicBlock::Create(Builder.getContext(), "while.header", CurrentFunction);
    llvm::BasicBlock* LoopBody = llvm::BasicBlock::Create(Builder.getContext(), "while.body", CurrentFunction);
    llvm::BasicBlock* LoopExit = llvm::BasicBlock::Create(Builder.getContext(), "while.exit", CurrentFunction);

    Builder.CreateBr(LoopHeader);
    Builder.SetInsertPoint(LoopHeader);

    llvm::Value* Condition = GenerateCondition(Node->condition.get(), Builder, SymbolStack, Methods);
    if (!Condition) {
        Write("While Generator", "Failed to generate while condition" + Location, 2, true, true, "");
        return nullptr;
    }

    Builder.CreateCondBr(Condition, LoopBody, LoopExit);

    Builder.SetInsertPoint(LoopBody);
    LoopExitStack.push(LoopExit);
    GenerateBlock(Node->block, Builder, SymbolStack, Methods);
    LoopExitStack.pop();

    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateBr(LoopHeader);
    }

    Builder.SetInsertPoint(LoopExit);
    return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
}