#include "UnaryAssignmentGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateReturn(const ReturnNode* Ret, AeroIR* IR, FunctionSymbols& Methods) { 
    if (!Ret) {
        Write("Return Generator", "Null ReturnNode pointer", 2, true, true, "");
        return nullptr;
    }
    
    std::string StmtLocation = " at line " + std::to_string(Ret->token.line) + ", column " + std::to_string(Ret->token.column);
    
    llvm::Value* Value = nullptr;
    
    if (Ret->value) {
        Value = GenerateExpression(Ret->value, IR, Methods);
        if (!Value) {
            return nullptr;
        }
    }
    
    llvm::Function* CurrentFunc = IR->getBuilder()->GetInsertBlock()->getParent();
    if (!CurrentFunc) {
        Write("Return Generator", "Invalid current function for return statement" + StmtLocation, 2, true, true, "");
        return nullptr;
    }

    llvm::Type* ReturnType = CurrentFunc->getReturnType();

    if (Value) {
        if (Value->getType() != ReturnType) {
            if (ReturnType->isIntegerTy(1) && Value->getType()->isIntegerTy()) {
                Value = IR->getBuilder()->CreateICmpNE(Value, llvm::ConstantInt::get(Value->getType(), 0), "tobool");
            }
            else if (ReturnType->isIntegerTy(1) && Value->getType()->isFloatingPointTy()) {
                Value = IR->getBuilder()->CreateFCmpONE(Value, llvm::ConstantFP::get(Value->getType(), 0.0), "tobool");
            }
            else if (ReturnType->isIntegerTy() && Value->getType()->isIntegerTy(1)) {
                Value = IR->getBuilder()->CreateZExt(Value, ReturnType, "frombool");
            }
            else if (ReturnType->isIntegerTy() && Value->getType()->isIntegerTy() && 
                     ReturnType->getIntegerBitWidth() != Value->getType()->getIntegerBitWidth()) {
                if (ReturnType->getIntegerBitWidth() > Value->getType()->getIntegerBitWidth()) {
                    Value = IR->getBuilder()->CreateSExt(Value, ReturnType, "sext");
                } else {
                    Value = IR->getBuilder()->CreateTrunc(Value, ReturnType, "trunc");
                }
            }
            else if (ReturnType->isFloatingPointTy() && Value->getType()->isIntegerTy()) {
                Value = IR->getBuilder()->CreateSIToFP(Value, ReturnType, "sitofp");
            }
            else if (ReturnType->isIntegerTy() && Value->getType()->isFloatingPointTy()) {
                Value = IR->getBuilder()->CreateFPToSI(Value, ReturnType, "fptosi");
            }
            else if (ReturnType->isFloatingPointTy() && Value->getType()->isFloatingPointTy()) {
                if (ReturnType->isDoubleTy() && Value->getType()->isFloatTy()) {
                    Value = IR->getBuilder()->CreateFPExt(Value, ReturnType, "fpext");
                } else if (ReturnType->isFloatTy() && Value->getType()->isDoubleTy()) {
                    Value = IR->getBuilder()->CreateFPTrunc(Value, ReturnType, "fptrunc");
                }
            }
            else {
                Write("Return Generator", "Unsupported type conversion" + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        }
        
        return IR->ret(Value);
    } else {
        if (ReturnType->isVoidTy()) {
            return IR->ret(nullptr);
        } else {
            llvm::Value* defaultValue;
            if (ReturnType->isIntegerTy(1)) {
                defaultValue = IR->constBool(false);
            } else if (ReturnType->isIntegerTy()) {
                defaultValue = llvm::ConstantInt::get(ReturnType, 0);
            } else if (ReturnType->isFloatingPointTy()) {
                defaultValue = llvm::ConstantFP::get(ReturnType, 0.0);
            } else if (ReturnType->isPointerTy()) {
                defaultValue = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ReturnType));
            } else {
                defaultValue = llvm::UndefValue::get(ReturnType);
            }
            return IR->ret(defaultValue);
        }
    }
}
