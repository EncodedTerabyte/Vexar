#include "BreakGenerator.hh"

std::stack<llvm::BasicBlock*> LoopExitStack;

llvm::Value* GenerateBreak(const BreakNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Break Generator", "Null BreakNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    if (LoopExitStack.empty()) {
        Write("Break Generator", "Break statement outside of loop" + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::BasicBlock* LoopExit = LoopExitStack.top();
    Builder.CreateBr(LoopExit);

    return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
}