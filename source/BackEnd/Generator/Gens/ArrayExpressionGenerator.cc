#include "ArrayExpressionGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateArrayExpression(const std::unique_ptr<ASTNode>& Expr, AeroIR* IR, FunctionSymbols& Methods) {
    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);
    
    if (Expr->type == NodeType::Array) {
        Write("Expression Generation", "Array literals should be handled in variable declarations, not expressions" + Location, 2, true, true, "");
        return nullptr;
    } 
    
    if (Expr->type == NodeType::ArrayAccess) {
        auto* AccessNodePtr = static_cast<ArrayAccessNode*>(Expr.get());
        if (!AccessNodePtr) {
            Write("Expression Generation", "Failed to cast to ArrayAccessNode" + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* arrayPtr = IR->getVar(AccessNodePtr->identifier);
        if (!arrayPtr) {
            Write("Expression Generation", "Undefined array identifier: " + AccessNodePtr->identifier + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr);
        if (!allocaInst) {
            Write("Expression Generation", "Array identifier is not an allocated variable: " + AccessNodePtr->identifier + Location, 2, true, true, "");
            return nullptr;
        }
        
        llvm::Type* allocatedType = allocaInst->getAllocatedType();
        llvm::Value* indexValue = GenerateExpression(AccessNodePtr->expr, IR, Methods);
        
        if (!indexValue || !indexValue->getType()->isIntegerTy()) {
            Write("Expression Generation", "Invalid array index" + Location, 2, true, true, "");
            return nullptr;
        }
        
        if (AccessNodePtr->expr->type == NodeType::Array) {
            ArrayNode* indexArray = static_cast<ArrayNode*>(AccessNodePtr->expr.get());
            if (indexArray && indexArray->elements.size() == 2) {
                llvm::Value* i = GenerateExpression(indexArray->elements[0], IR, Methods);
                llvm::Value* j = GenerateExpression(indexArray->elements[1], IR, Methods);
                
                if (i && j && i->getType()->isIntegerTy() && j->getType()->isIntegerTy()) {
                    llvm::Value* elementPtr = IR->getBuilder()->CreateInBoundsGEP(
                        allocatedType, arrayPtr, {IR->constI32(0), i, j}
                    );
                    llvm::ArrayType* outerArrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
                    if (outerArrayType) {
                        llvm::ArrayType* innerArrayType = llvm::dyn_cast<llvm::ArrayType>(outerArrayType->getElementType());
                        llvm::Type* elementType = innerArrayType ? innerArrayType->getElementType() : IR->i32();
                        return IR->getBuilder()->CreateLoad(elementType, elementPtr);
                    }
                }
            }
        } else if (allocatedType->isPointerTy()) {
            llvm::Value* loadedPtr = IR->load(allocaInst);
            llvm::Value* elementPtr = IR->getBuilder()->CreateInBoundsGEP(IR->i8(), loadedPtr, indexValue);
            return IR->getBuilder()->CreateLoad(IR->i8(), elementPtr);
        } else if (allocatedType->isArrayTy()) {
            llvm::Value* elementPtr = IR->getBuilder()->CreateInBoundsGEP(allocatedType, arrayPtr, {IR->constI32(0), indexValue});
            llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(allocatedType);
            llvm::Type* elementType = arrayType ? arrayType->getElementType() : IR->i8();
            return IR->getBuilder()->CreateLoad(elementType, elementPtr);
        }
    }
    
    return nullptr;
}