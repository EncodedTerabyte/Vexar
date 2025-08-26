#include "CallGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateCall(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods, BuiltinSymbols& BuiltIns) {
    if (!Expr) {
        Write("Function Call", "Null ASTNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);

    auto* FuncCallNode = static_cast<FunctionCallNode*>(Expr.get());
    if (!FuncCallNode) {
        Write("Function Call", "Failed to cast ASTNode to FunctionCallNode" + Location, 2, true, true, "");
        return nullptr;
    }
    
    auto BuiltinIt = BuiltIns.find(FuncCallNode->name);
    if (BuiltinIt != BuiltIns.end()) {
        llvm::Value* Result = BuiltinIt->second(FuncCallNode->arguments, Builder, SymbolStack, Methods);
        if (!Result) {
            Write("Function Call", "Failed to execute builtin function: " + FuncCallNode->name + Location, 2, true, true, "");
            return nullptr;
        }
        return Result;
    }
    
    auto it = Methods.find(FuncCallNode->name);
    if (it == Methods.end()) {
        Write("Function Call", "Function not found: " + FuncCallNode->name + Location, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Function* Function = it->second;
    if (!Function) {
        Write("Function Call", "Invalid function pointer for: " + FuncCallNode->name + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::FunctionType* FuncType = Function->getFunctionType();
    
    std::vector<llvm::Value*> Args;
    for (size_t i = 0; i < FuncCallNode->arguments.size(); ++i) {
        llvm::Value* ArgValue = GenerateExpression(FuncCallNode->arguments[i], Builder, SymbolStack, Methods);
        if (!ArgValue) {
            Write("Function Call", "Invalid argument expression for function: " + FuncCallNode->name + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (i < FuncType->getNumParams()) {
            llvm::Type* ExpectedType = FuncType->getParamType(i);
            if (ArgValue->getType() != ExpectedType) {
                if (ExpectedType->isIntegerTy(32) && ArgValue->getType()->isFloatingPointTy()) {
                    ArgValue = Builder.CreateFPToSI(ArgValue, ExpectedType);
                } else if (ExpectedType->isFloatingPointTy() && ArgValue->getType()->isIntegerTy()) {
                    ArgValue = Builder.CreateSIToFP(ArgValue, ExpectedType);
                } else if (ExpectedType->isFloatTy() && ArgValue->getType()->isDoubleTy()) {
                    ArgValue = Builder.CreateFPTrunc(ArgValue, ExpectedType);
                } else if (ExpectedType->isDoubleTy() && ArgValue->getType()->isFloatTy()) {
                    ArgValue = Builder.CreateFPExt(ArgValue, ExpectedType);
                } else {
                    Write("Function Call", "Type mismatch for argument " + std::to_string(i) + " in function: " + FuncCallNode->name + Location, 2, true, true, "");
                    return nullptr;
                }
            }
        }
        Args.push_back(ArgValue);
    }
    
    return Builder.CreateCall(Function, Args);
}