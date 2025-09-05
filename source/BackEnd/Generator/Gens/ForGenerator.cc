#include "ForGenerator.hh"
#include "BlockGenerator.hh"
#include "ConditionGenerator.hh"
#include "BreakGenerator.hh"
#include "ExpressionGenerator.hh"
#include "VariableGenerator.hh"

llvm::Value* GenerateFor(ForNode* Node, AeroIR* IR, FunctionSymbols& Methods) {
    if (!Node) {
        Write("For Generator", "Null ForNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::Function* CurrentFunction = IR->getBuilder()->GetInsertBlock()->getParent();
    if (!CurrentFunction) {
        Write("For Generator", "No current function for for loop" + Location, 2, true, true, "");
        return nullptr;
    }

    IR->pushScope();

    llvm::BasicBlock* ForHeader = IR->createBlock("for.header");
    llvm::BasicBlock* ForBody = IR->createBlock("for.body");
    llvm::BasicBlock* ForIncrement = IR->createBlock("for.increment");
    llvm::BasicBlock* ForExit = IR->createBlock("for.exit");

    if (Node->init) {
        if (Node->init->type == NodeType::Variable) {
            auto* VarNode = static_cast<VariableNode*>(Node->init.get());
            GenerateVariable(VarNode, IR, Methods);
        } else {
            GenerateExpression(Node->init, IR, Methods);
        }
    }

    IR->branch(ForHeader);
    IR->setInsertPoint(ForHeader);

    if (Node->condition) {
        llvm::Value* Condition = GenerateCondition(Node->condition.get(), IR, Methods);
        if (!Condition) {
            Write("For Generator", "Failed to generate for condition" + Location, 2, true, true, "");
            IR->popScope();
            return nullptr;
        }
        IR->condBranch(Condition, ForBody, ForExit);
    } else {
        IR->branch(ForBody);
    }

    IR->setInsertPoint(ForBody);
    LoopExitStack.push(ForExit);
    GenerateBlock(Node->body, IR, Methods);
    LoopExitStack.pop();

    if (!IR->getBuilder()->GetInsertBlock()->getTerminator()) {
        IR->branch(ForIncrement);
    }

    IR->setInsertPoint(ForIncrement);
    if (Node->increment) {
        GenerateExpression(Node->increment, IR, Methods);
    }
    IR->branch(ForHeader);

    IR->setInsertPoint(ForExit);
    IR->popScope();
    
    return IR->constI32(0);
}