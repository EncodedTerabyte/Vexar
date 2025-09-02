#include "UnaryOpGenerator.hh" 
#include "ExpressionGenerator.hh" 
 
llvm::Value* GenerateUnaryOp(const std::unique_ptr<ASTNode>& Expr, AeroIR* IR, FunctionSymbols& Methods) {
    auto* UnaryNode = static_cast<UnaryOpNode*>(Expr.get());
    if (!UnaryNode) {
        return nullptr;
    }
    
    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);
    
    if (UnaryNode->op == "++") {
        if (!UnaryNode->operand || UnaryNode->operand->type != NodeType::Identifier) {
            Write("Unary Op Generation", "Increment operator requires identifier operand" + Location, 2, true, true, "");
            return nullptr;
        }
        
        auto* IdentNode = static_cast<IdentifierNode*>(UnaryNode->operand.get());
        llvm::Value* VarPtr = IR->getVar(IdentNode->name);
        if (!VarPtr) {
            Write("Unary Op Generation", "Undefined variable: " + IdentNode->name + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* CurrentValue = IR->load(VarPtr);
        llvm::Value* One = nullptr;
        llvm::Value* NewValue = nullptr;
        
        if (CurrentValue->getType()->isFloatingPointTy()) {
            One = IR->constF64(1.0);
            if (CurrentValue->getType()->isFloatTy()) {
                One = IR->constF32(1.0f);
            }
            NewValue = IR->add(CurrentValue, One);
        } else if (CurrentValue->getType()->isIntegerTy()) {
            One = IR->constI32(1);
            if (CurrentValue->getType()->isIntegerTy(64)) {
                One = IR->constI64(1);
            }
            NewValue = IR->add(CurrentValue, One);
        } else {
            Write("Unary Op Generation", "Increment operator not supported for this type" + Location, 2, true, true, "");
            return nullptr;
        }
        
        IR->store(NewValue, VarPtr);
        return NewValue;
    }
    
    if (UnaryNode->op == "--") {
        if (!UnaryNode->operand || UnaryNode->operand->type != NodeType::Identifier) {
            Write("Unary Op Generation", "Decrement operator requires identifier operand" + Location, 2, true, true, "");
            return nullptr;
        }
        
        auto* IdentNode = static_cast<IdentifierNode*>(UnaryNode->operand.get());
        llvm::Value* VarPtr = IR->getVar(IdentNode->name);
        if (!VarPtr) {
            Write("Unary Op Generation", "Undefined variable: " + IdentNode->name + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* CurrentValue = IR->load(VarPtr);
        llvm::Value* One = nullptr;
        llvm::Value* NewValue = nullptr;
        
        if (CurrentValue->getType()->isFloatingPointTy()) {
            One = IR->constF64(1.0);
            if (CurrentValue->getType()->isFloatTy()) {
                One = IR->constF32(1.0f);
            }
            NewValue = IR->sub(CurrentValue, One);
        } else if (CurrentValue->getType()->isIntegerTy()) {
            One = IR->constI32(1);
            if (CurrentValue->getType()->isIntegerTy(64)) {
                One = IR->constI64(1);
            }
            NewValue = IR->sub(CurrentValue, One);
        } else {
            Write("Unary Op Generation", "Decrement operator not supported for this type" + Location, 2, true, true, "");
            return nullptr;
        }
        
        IR->store(NewValue, VarPtr);
        return NewValue;
    }
    
    if (UnaryNode->op == "-") {
        llvm::Value* Operand = GenerateExpression(UnaryNode->operand, IR, Methods);
        if (!Operand) {
            Write("Unary Op Generation", "Invalid operand for unary minus" + Location, 2, true, true, "");
            return nullptr;
        }
        
        return IR->neg(Operand);
    }
    
    if (UnaryNode->op == "!") {
        llvm::Value* Operand = GenerateExpression(UnaryNode->operand, IR, Methods);
        if (!Operand) {
            Write("Unary Op Generation", "Invalid operand for logical not" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (Operand->getType()->isIntegerTy(1)) {
            return IR->not_(Operand);
        } else {
            if (Operand->getType()->isFloatingPointTy()) {
                llvm::Value* Zero = IR->constF64(0.0);
                if (Operand->getType()->isFloatTy()) {
                    Zero = IR->constF32(0.0f);
                }
                return IR->eq(Operand, Zero);
            } else if (Operand->getType()->isIntegerTy()) {
                llvm::Value* Zero = IR->constI32(0);
                if (Operand->getType()->isIntegerTy(64)) {
                    Zero = IR->constI64(0);
                }
                return IR->eq(Operand, Zero);
            } else {
                Write("Unary Op Generation", "Logical not not supported for this type" + Location, 2, true, true, "");
                return nullptr;
            }
        }
    }
    
    Write("Unary Op Generation", "Unsupported unary operator: " + UnaryNode->op + Location, 2, true, true, "");
    return nullptr;
}