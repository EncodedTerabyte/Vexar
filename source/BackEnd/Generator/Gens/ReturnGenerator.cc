#include "ExpressionGenerator.hh"
#include "ReturnGenerator.hh"

llvm::Value* GenerateReturn(const ReturnNode* Ret, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) { 
    std::string StmtLocation = " at line " + std::to_string(Ret->token.line) + ", column " + std::to_string(Ret->token.column);

    if (!Ret) {
        Write("Block Generator", "Failed to cast to ReturnNode" + StmtLocation, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Value* Value = nullptr;
    if (Ret->value) {
        Value = GenerateExpression(Ret->value, Builder, SymbolStack, Methods);
        if (!Value) {
            Write("Block Generator", "Invalid return expression" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
    }
    
    llvm::Function* CurrentFunc = Builder.GetInsertBlock()->getParent();
    if (!CurrentFunc) {
        Write("Block Generator", "Invalid current function for return statement" + StmtLocation, 2, true, true, "");
        return nullptr;
    }

    llvm::Type* ReturnType = CurrentFunc->getReturnType();

    if (Value && Value->getType() != ReturnType) {
        if (ReturnType->isIntegerTy() && Value->getType()->isFloatingPointTy()) {
            Value = Builder.CreateFPToSI(Value, ReturnType, "fptosi");
        } else if (ReturnType->isFloatingPointTy() && Value->getType()->isIntegerTy()) {
            Value = Builder.CreateSIToFP(Value, ReturnType, "sitofp");
        } else if (ReturnType->isFloatingPointTy() && Value->getType()->isFloatingPointTy()) {
            if (ReturnType->getTypeID() == llvm::Type::FloatTyID && 
                Value->getType()->getTypeID() == llvm::Type::DoubleTyID) {
                Value = Builder.CreateFPTrunc(Value, ReturnType, "fptrunc");
            } else if (ReturnType->getTypeID() == llvm::Type::DoubleTyID && 
                    Value->getType()->getTypeID() == llvm::Type::FloatTyID) {
                Value = Builder.CreateFPExt(Value, ReturnType, "fpext");
            }
        } else {
            Write("Block Generator", "Type mismatch in return statement" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
    }
    
    if (Value) {
        Builder.CreateRet(Value);
    } else {
        if (ReturnType->isVoidTy()) {
            Builder.CreateRetVoid();
        } else {
            llvm::Value* defaultValue;
            if (ReturnType->isFloatingPointTy()) {
                defaultValue = llvm::ConstantFP::get(ReturnType, 0.0);
            } else if (ReturnType->isIntegerTy()) {
                defaultValue = llvm::ConstantInt::get(ReturnType, 0);
            } else if (ReturnType->isPointerTy()) {
                defaultValue = llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(ReturnType));
            } else {
                defaultValue = llvm::UndefValue::get(ReturnType);
                Write("Block Generator", "Unsupported return type for default value" + StmtLocation, 1, true, true, "");
            }
            Builder.CreateRet(defaultValue);
        }
    }

    return Value;
}