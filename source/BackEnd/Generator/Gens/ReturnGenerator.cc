#include "ExpressionGenerator.hh"
#include "ReturnGenerator.hh"

llvm::Value* GenerateReturn(const ReturnNode* Ret, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) { 
    if (!Ret) {
        Write("Return Generator", "Null ReturnNode pointer", 2, true, true, "");
        return nullptr;
    }
    
    std::string StmtLocation = " at line " + std::to_string(Ret->token.line) + ", column " + std::to_string(Ret->token.column);
    Write("Return Generator", "Processing return statement" + StmtLocation, 0, true, true, "");
    
    llvm::Value* Value = nullptr;
    std::string typeName = "unknown";
    
    if (Ret->value) {
        Write("Return Generator", "Generating return expression" + StmtLocation, 0, true, true, "");
        Value = GenerateExpression(Ret->value, Builder, SymbolStack, Methods);
        if (!Value) {
            Write("Return Generator", "Failed to generate return expression" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        
        if (Value->getType()->isIntegerTy(1)) typeName = "i1 (bool)";
        else if (Value->getType()->isIntegerTy(32)) typeName = "i32 (int)";
        else if (Value->getType()->isFloatTy()) typeName = "float";
        else if (Value->getType()->isDoubleTy()) typeName = "double";
        
        Write("Return Generator", "Generated return value of type: " + typeName + StmtLocation, 0, true, true, "");
    }
    
    llvm::Function* CurrentFunc = Builder.GetInsertBlock()->getParent();
    if (!CurrentFunc) {
        Write("Return Generator", "Invalid current function for return statement" + StmtLocation, 2, true, true, "");
        return nullptr;
    }

    llvm::Type* ReturnType = CurrentFunc->getReturnType();
    std::string returnTypeName = "unknown";
    if (ReturnType->isIntegerTy(1)) returnTypeName = "i1 (bool)";
    else if (ReturnType->isIntegerTy(32)) returnTypeName = "i32 (int)";
    else if (ReturnType->isFloatTy()) returnTypeName = "float";
    else if (ReturnType->isDoubleTy()) returnTypeName = "double";
    else if (ReturnType->isVoidTy()) returnTypeName = "void";
    
    Write("Return Generator", "Function expects return type: " + returnTypeName + StmtLocation, 0, true, true, "");

    if (Value) {
        if (Value->getType() != ReturnType) {
            Write("Return Generator", "Type mismatch: converting from " + typeName + " to " + returnTypeName + StmtLocation, 1, true, true, "");
            
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
                Write("Return Generator", "Unsupported type conversion from " + typeName + " to " + returnTypeName + StmtLocation, 2, true, true, "");
                return nullptr;
            }
        }
        
        Write("Return Generator", "Creating return instruction" + StmtLocation, 0, true, true, "");
        llvm::ReturnInst* RetInst = Builder.CreateRet(Value);
        if (!RetInst) {
            Write("Return Generator", "Failed to create return instruction" + StmtLocation, 2, true, true, "");
            return nullptr;
        }
        Write("Return Generator", "Successfully created return instruction" + StmtLocation, 0, true, true, "");
        return Value;
    } else {
        Write("Return Generator", "Creating default return" + StmtLocation, 0, true, true, "");
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