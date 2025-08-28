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
            return nullptr;
        }
    }
    
    llvm::Function* CurrentFunc = Builder.GetInsertBlock()->getParent();
    if (!CurrentFunc) {
        Write("Return Generator", "Invalid current function for return statement" + StmtLocation, 2, true, true, "");
        return nullptr;
    }

    llvm::Type* ReturnType = CurrentFunc->getReturnType();

    if (Value) {
        if (Value->getType() != ReturnType) {
            if (ReturnType->isIntegerTy(1) && Value->getType()->isIntegerTy()) {
                Value = Builder.CreateICmpNE(Value, llvm::ConstantInt::get(Value->getType(), 0), "tobool");
            }
            else if (ReturnType->isIntegerTy(1) && Value->getType()->isFloatingPointTy()) {
                Value = Builder.CreateFCmpONE(Value, llvm::ConstantFP::get(Value->getType(), 0.0), "tobool");
            }
            else if (ReturnType->isIntegerTy() && Value->getType()->isIntegerTy(1)) {
                Value = Builder.CreateZExt(Value, ReturnType, "frombool");
            }
            else if (ReturnType->isIntegerTy() && Value->getType()->isIntegerTy() && 
                     ReturnType->getIntegerBitWidth() != Value->getType()->getIntegerBitWidth()) {
                if (ReturnType->getIntegerBitWidth() > Value->getType()->getIntegerBitWidth()) {
                    Value = Builder.CreateSExt(Value, ReturnType, "sext");
                } else {
                    Value = Builder.CreateTrunc(Value, ReturnType, "trunc");
                }
            }
            else if (ReturnType->isFloatingPointTy() && Value->getType()->isIntegerTy()) {
                Value = Builder.CreateSIToFP(Value, ReturnType, "sitofp");
            }
            else if (ReturnType->isIntegerTy() && Value->getType()->isFloatingPointTy()) {
                Value = Builder.CreateFPToSI(Value, ReturnType, "fptosi");
            }
            else if (ReturnType->isFloatingPointTy() && Value->getType()->isFloatingPointTy()) {
                if (ReturnType->isDoubleTy() && Value->getType()->isFloatTy()) {
                    Value = Builder.CreateFPExt(Value, ReturnType, "fpext");
                } else if (ReturnType->isFloatTy() && Value->getType()->isDoubleTy()) {
                    Value = Builder.CreateFPTrunc(Value, ReturnType, "fptrunc");
                }
            }
            else {
                Write("Return Generator", "Unsupported type conversion" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        }
        
        llvm::ReturnInst* RetInst = Builder.CreateRet(Value);
        if (!RetInst) {
            Write("Return Generator", "Failed to create return instruction" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        return Value;
    } else {
        if (ReturnType->isVoidTy()) {
            llvm::ReturnInst* RetInst = Builder.CreateRetVoid();
            if (!RetInst) {
                Write("Return Generator", "Failed to create void return instruction" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        } else {
            llvm::Value* defaultValue;
            if (ReturnType->isIntegerTy(1)) {
                defaultValue = llvm::ConstantInt::get(ReturnType, false);
            } else if (ReturnType->isIntegerTy()) {
                defaultValue = llvm::ConstantInt::get(ReturnType, 0);
            } else if (ReturnType->isFloatingPointTy()) {
                defaultValue = llvm::ConstantFP::get(ReturnType, 0.0);
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
        return nullptr;
    }
}