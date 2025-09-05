#include "BreakGenerator.hh"

std::stack<llvm::BasicBlock*> LoopExitStack;

llvm::Value* GenerateBreak(const BreakNode* Node, AeroIR* IR, FunctionSymbols& Methods) {
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
    return IR->branch(LoopExit);
}