#include "ExpressionGenerator.hh"
#include "ReturnGenerator.hh"

llvm::Value* GenerateReturn(const ReturnNode* Ret, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) { 
    if (!Ret) {
        Write("Return Generator", "Null ReturnNode pointer", 2, true, true, "");
        return nullptr;
    }
    
    std::string StmtLocation = " at line " + std::to_string(Ret->token.line) + ", column " + std::to_string(Ret->token.column);
    
    llvm::Value* Value = nullptr;
    if (Ret->value) {
        Value = GenerateExpression(Ret->value, Builder, SymbolStack, Methods);
        if (!Value) {
            Write("Return Generator", "Invalid return expression" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
    }
    
    llvm::Function* CurrentFunc = Builder.GetInsertBlock()->getParent();
    if (!CurrentFunc) {
        Write("Return Generator", "Invalid current function for return statement" + StmtLocation, 2, true, true, "");
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
            Write("Return Generator", "Type mismatch in return statement" + StmtLocation, 2, true, true, "");
        }
    }
    
    if (Value) {
        llvm::ReturnInst* RetInst = Builder.CreateRet(Value);
        if (!RetInst) {
            Write("Return Generator", "Failed to create return instruction" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
    } else {
        if (ReturnType->isVoidTy()) {
            llvm::ReturnInst* RetInst = Builder.CreateRetVoid();
            if (!RetInst) {
                Write("Return Generator", "Failed to create void return instruction" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        } else {
            llvm::Value* defaultValue;
            if (ReturnType->isFloatingPointTy()) {
                defaultValue = llvm::ConstantFP::get(ReturnType, 0.0);
            } else if (ReturnType->isIntegerTy()) {
                defaultValue = llvm::ConstantInt::get(ReturnType, 0);
            } else if (ReturnType->isPointerTy()) {
                defaultValue = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ReturnType));
            } else {
                defaultValue = llvm::UndefValue::get(ReturnType);
            }
            llvm::ReturnInst* RetInst = Builder.CreateRet(defaultValue);
            if (!RetInst) {
                Write("Return Generator", "Failed to create default return instruction" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        }
    }

    return Value;
}