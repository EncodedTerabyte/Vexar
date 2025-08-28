#include "ForGenerator.hh"
#include "BlockGenerator.hh"
#include "ConditionGenerator.hh"
#include "BreakGenerator.hh"
#include "ExpressionGenerator.hh"
#include "VariableGenerator.hh"

llvm::Value* GenerateFor(ForNode* Node, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
    if (!Node) {
        Write("For Generator", "Null ForNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::Function* CurrentFunction = Builder.GetInsertBlock()->getParent();
    if (!CurrentFunction) {
        Write("For Generator", "No current function for for loop" + Location, 2, true, true, "");
        return nullptr;
    }

    PushScope(SymbolStack);

    llvm::BasicBlock* ForHeader = llvm::BasicBlock::Create(Builder.getContext(), "for.header", CurrentFunction);
    llvm::BasicBlock* ForBody = llvm::BasicBlock::Create(Builder.getContext(), "for.body", CurrentFunction);
    llvm::BasicBlock* ForIncrement = llvm::BasicBlock::Create(Builder.getContext(), "for.increment", CurrentFunction);
    llvm::BasicBlock* ForExit = llvm::BasicBlock::Create(Builder.getContext(), "for.exit", CurrentFunction);

    if (Node->init) {
        if (Node->init->type == NodeType::Variable) {
            auto* VarNode = static_cast<VariableNode*>(Node->init.get());
            GenerateVariable(VarNode, Builder, SymbolStack, Methods);
        } else {
            GenerateExpression(Node->init, Builder, SymbolStack, Methods);
        }
    }

    Builder.CreateBr(ForHeader);
    Builder.SetInsertPoint(ForHeader);

    if (Node->condition) {
        llvm::Value* Condition = GenerateCondition(Node->condition.get(), Builder, SymbolStack, Methods);
        if (!Condition) {
            Write("For Generator", "Failed to generate for condition" + Location, 2, true, true, "");
            PopScope(SymbolStack);
            return nullptr;
        }
        Builder.CreateCondBr(Condition, ForBody, ForExit);
    } else {
        Builder.CreateBr(ForBody);
    }

    Builder.SetInsertPoint(ForBody);
    LoopExitStack.push(ForExit);
    GenerateBlock(Node->body, Builder, SymbolStack, Methods);
    LoopExitStack.pop();

    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateBr(ForIncrement);
    }

    Builder.SetInsertPoint(ForIncrement);
    if (Node->increment) {
        GenerateExpression(Node->increment, Builder, SymbolStack, Methods);
    }
    Builder.CreateBr(ForHeader);

    Builder.SetInsertPoint(ForExit);
    PopScope(SymbolStack);
    
    return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
}