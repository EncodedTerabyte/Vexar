#include "CastGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateCast(const std::unique_ptr<ASTNode>& Expr, AeroIR* IR, FunctionSymbols& Methods) {
    if (!Expr) {
        Write("Cast Generation", "Null ASTNode pointer", 2, true, true, "");
        return nullptr;
    }

    std::string Location = " at line " + std::to_string(Expr->token.line) + ", column " + std::to_string(Expr->token.column);

    auto* CastNodePtr = static_cast<CastNode*>(Expr.get());
    if (!CastNodePtr) {
        Write("Cast Generation", "Failed to cast ASTNode to CastNode" + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::Value* ExprValue = GenerateExpression(CastNodePtr->expr, IR, Methods);
    if (!ExprValue) {
        Write("Cast Generation", "Invalid expression for cast" + Location, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Type* TargetType = nullptr;
    if (CastNodePtr->targetType == "int") {
        TargetType = IR->int_t();
    } else if (CastNodePtr->targetType == "float") {
        TargetType = IR->float_t();
    } else if (CastNodePtr->targetType == "double") {
        TargetType = IR->double_t();
    } else if (CastNodePtr->targetType == "char") {
        TargetType = IR->char_t();
    } else if (CastNodePtr->targetType == "bool") {
        TargetType = IR->bool_t();
    } else if (CastNodePtr->targetType == "string") {
        TargetType = IR->string_t();
    } else {
        Write("Cast Generation", "Unsupported target type: " + CastNodePtr->targetType + Location, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Type* SourceType = ExprValue->getType();
    
    if (SourceType == TargetType) {
        if (Expr->token.line != 0 && Expr->token.column != 0)
            Write("Cast Generation", "No cast needed, source and target types are identical" + Location, 1, true, true, "");
        return ExprValue;
    }
    
    if (SourceType->isIntegerTy() && TargetType->isIntegerTy()) {
        if (SourceType->getIntegerBitWidth() < TargetType->getIntegerBitWidth()) {
            return IR->intCast(ExprValue, TargetType);
        } else if (SourceType->getIntegerBitWidth() > TargetType->getIntegerBitWidth()) {
            return IR->intCast(ExprValue, TargetType);
        }
    } else if (SourceType->isIntegerTy() && TargetType->isFloatingPointTy()) {
        return IR->getBuilder()->CreateSIToFP(ExprValue, TargetType);
    } else if (SourceType->isFloatingPointTy() && TargetType->isIntegerTy()) {
        return IR->getBuilder()->CreateFPToSI(ExprValue, TargetType);
    } else if (SourceType->isFloatingPointTy() && TargetType->isFloatingPointTy()) {
        if (SourceType->isFloatTy() && TargetType->isDoubleTy()) {
            return IR->floatCast(ExprValue, TargetType);
        } else if (SourceType->isDoubleTy() && TargetType->isFloatTy()) {
            return IR->floatCast(ExprValue, TargetType);
        }
    } else if (SourceType->isIntegerTy(8) && TargetType->isPointerTy()) {
        llvm::Value* charPtr = IR->malloc(IR->i8(), IR->constI64(2));
        if (!charPtr) {
            Write("Cast Generation", "Failed to allocate memory for char to string cast" + Location, 2, true, true, "");
            return nullptr;
        }
        IR->store(ExprValue, charPtr);
        llvm::Value* nullPtr = IR->getBuilder()->CreateGEP(IR->i8(), charPtr, IR->constI64(1));
        IR->store(IR->constI8(0), nullPtr);
        return charPtr;
    } else if (SourceType->isPointerTy() && TargetType->isIntegerTy()) {
        llvm::Value* firstChar = IR->load(ExprValue);
        if (TargetType->isIntegerTy(32)) {
            return IR->intCast(firstChar, TargetType);
        } else if (TargetType->isIntegerTy(8)) {
            return firstChar;
        } else {
            return IR->intCast(firstChar, TargetType);
        }
    } else if (SourceType->isPointerTy() && TargetType->isFloatingPointTy()) {
        llvm::Function* strtodFunc = IR->getRegisteredBuiltin("strtod");
        if (!strtodFunc) {
            std::vector<llvm::Type*> strtodArgs = {IR->string_t(), IR->ptr(IR->string_t())};
            strtodFunc = IR->createBuiltinFunction("strtod", IR->double_t(), strtodArgs);
        }
        llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(IR->ptr(IR->string_t())));
        llvm::Value* doubleResult = IR->call(strtodFunc, {ExprValue, nullPtr});
        
        if (TargetType->isFloatTy()) {
            return IR->floatCast(doubleResult, TargetType);
        } else {
            return doubleResult;
        }
    } else if (SourceType->isIntegerTy() && TargetType->isPointerTy()) {
        llvm::Value* bufferPtr = IR->malloc(IR->i8(), IR->constI64(32));
        if (!bufferPtr) {
            Write("Cast Generation", "Failed to allocate memory for integer to string cast" + Location, 2, true, true, "");
            return nullptr;
        }

        llvm::Function* sprintfFunc = IR->getRegisteredBuiltin("sprintf");
        if (!sprintfFunc) {
            std::vector<llvm::Type*> sprintfArgs = {IR->string_t(), IR->string_t()};
            sprintfFunc = IR->createBuiltinFunction("sprintf", IR->i32(), sprintfArgs);
        }

        llvm::Value* formatStr = IR->constString("%d");
        IR->call(sprintfFunc, {bufferPtr, formatStr, ExprValue});
        return bufferPtr;
    } else if (SourceType->isFloatingPointTy() && TargetType->isPointerTy()) {
        llvm::Value* bufferPtr = IR->malloc(IR->i8(), IR->constI64(32));
        if (!bufferPtr) {
            Write("Cast Generation", "Failed to allocate memory for floating-point to string cast" + Location, 2, true, true, "");
            return nullptr;
        }

        llvm::Function* sprintfFunc = IR->getRegisteredBuiltin("sprintf");
        if (!sprintfFunc) {
            std::vector<llvm::Type*> sprintfArgs = {IR->string_t(), IR->string_t()};
            sprintfFunc = IR->createBuiltinFunction("sprintf", IR->i32(), sprintfArgs);
        }

        llvm::Value* formatStr = IR->constString("%.6f");
        
        if (ExprValue->getType()->isFloatTy()) {
            ExprValue = IR->floatCast(ExprValue, IR->double_t());
        }
        
        IR->call(sprintfFunc, {bufferPtr, formatStr, ExprValue});
        return bufferPtr;
    }
    
    Write("Cast Generation", "Unsupported cast from source type to target type" + Location, 2, true, true, "");
    return nullptr;
}