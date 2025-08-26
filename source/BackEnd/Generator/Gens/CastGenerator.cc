#include "CastGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateCast(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
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

    llvm::Value* ExprValue = GenerateExpression(CastNodePtr->expr, Builder, SymbolStack, Methods);
    if (!ExprValue) {
        Write("Cast Generation", "Invalid expression for cast" + Location, 2, true, true, "");
        return nullptr;
    }
    
    llvm::Type* TargetType = nullptr;
    if (CastNodePtr->targetType == "int") {
        TargetType = llvm::Type::getInt32Ty(Builder.getContext());
    } else if (CastNodePtr->targetType == "float") {
        TargetType = llvm::Type::getFloatTy(Builder.getContext());
    } else if (CastNodePtr->targetType == "double") {
        TargetType = llvm::Type::getDoubleTy(Builder.getContext());
    } else if (CastNodePtr->targetType == "char") {
        TargetType = llvm::Type::getInt8Ty(Builder.getContext());
    } else if (CastNodePtr->targetType == "bool") {
        TargetType = llvm::Type::getInt1Ty(Builder.getContext());
    } else if (CastNodePtr->targetType == "string") {
        TargetType = llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0);
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
            llvm::Value* Result = Builder.CreateSExt(ExprValue, TargetType);

            return Result;
        } else if (SourceType->getIntegerBitWidth() > TargetType->getIntegerBitWidth()) {
            llvm::Value* Result = Builder.CreateTrunc(ExprValue, TargetType);

            return Result;
        }
    } else if (SourceType->isIntegerTy() && TargetType->isFloatingPointTy()) {
        llvm::Value* Result = Builder.CreateSIToFP(ExprValue, TargetType);

        return Result;
    } else if (SourceType->isFloatingPointTy() && TargetType->isIntegerTy()) {
        llvm::Value* Result = Builder.CreateFPToSI(ExprValue, TargetType);

        return Result;
    } else if (SourceType->isFloatingPointTy() && TargetType->isFloatingPointTy()) {
        if (SourceType->isFloatTy() && TargetType->isDoubleTy()) {
            llvm::Value* Result = Builder.CreateFPExt(ExprValue, TargetType);

            return Result;
        } else if (SourceType->isDoubleTy() && TargetType->isFloatTy()) {
            llvm::Value* Result = Builder.CreateFPTrunc(ExprValue, TargetType);

            return Result;
        }
    } else if (SourceType->isIntegerTy(8) && TargetType->isPointerTy()) {
        llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
        if (!mallocFunc) {
            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                {llvm::Type::getInt64Ty(Builder.getContext())},
                false
            );
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", 
                                               Builder.GetInsertBlock()->getParent()->getParent());
        }
        llvm::Value* charPtr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 2)});
        if (!charPtr) {
            Write("Cast Generation", "Failed to allocate memory for char to string cast" + Location, 2, true, true, "");
            return nullptr;
        }
        Builder.CreateStore(ExprValue, charPtr);
        llvm::Value* nullPtr = Builder.CreateGEP(llvm::Type::getInt8Ty(Builder.getContext()), charPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1));
        Builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), nullPtr);

        return charPtr;
    } else if (SourceType->isPointerTy() && TargetType->isIntegerTy()) {
        llvm::Value* firstChar = Builder.CreateLoad(llvm::Type::getInt8Ty(Builder.getContext()), ExprValue);
        if (TargetType->isIntegerTy(32)) {
            llvm::Value* Result = Builder.CreateSExt(firstChar, TargetType);

            return Result;
        } else if (TargetType->isIntegerTy(8)) {

            return firstChar;
        } else {
            llvm::Value* Result = Builder.CreateSExt(firstChar, TargetType);

            return Result;
        }
    } else if (SourceType->isPointerTy() && TargetType->isFloatingPointTy()) {
        llvm::Function* strtodFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("strtod");
        if (!strtodFunc) {
            llvm::FunctionType* strtodType = llvm::FunctionType::get(
                llvm::Type::getDoubleTy(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                 llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 0)},
                false
            );
            strtodFunc = llvm::Function::Create(strtodType, llvm::Function::ExternalLinkage, "strtod", 
                                               Builder.GetInsertBlock()->getParent()->getParent());
        }
        llvm::Value* nullPtr = llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 0));
        llvm::Value* doubleResult = Builder.CreateCall(strtodFunc, {ExprValue, nullPtr});
        
        if (TargetType->isFloatTy()) {
            llvm::Value* Result = Builder.CreateFPTrunc(doubleResult, TargetType);

            return Result;
        } else {

            return doubleResult;
        }
    } else if (SourceType->isIntegerTy() && TargetType->isPointerTy()) {
        llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
        if (!mallocFunc) {
            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                {llvm::Type::getInt64Ty(Builder.getContext())},
                false
            );
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", 
                                               Builder.GetInsertBlock()->getParent()->getParent());
        }

        llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
        if (!sprintfFunc) {
            llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                 llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", 
                                                Builder.GetInsertBlock()->getParent()->getParent());
        }

        llvm::Value* bufferPtr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
        if (!bufferPtr) {
            Write("Cast Generation", "Failed to allocate memory for integer to string cast" + Location, 2, true, true, "");
            return nullptr;
        }
        llvm::Value* formatStr = Builder.CreateGlobalString("%d", "int_to_str_fmt");
        llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
        Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, ExprValue});

        return bufferPtr;
    } else if (SourceType->isFloatingPointTy() && TargetType->isPointerTy()) {
        llvm::Function* mallocFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
        if (!mallocFunc) {
            llvm::FunctionType* mallocType = llvm::FunctionType::get(
                llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0),
                {llvm::Type::getInt64Ty(Builder.getContext())},
                false
            );
            mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", 
                                               Builder.GetInsertBlock()->getParent()->getParent());
        }

        llvm::Function* sprintfFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("sprintf");
        if (!sprintfFunc) {
            llvm::FunctionType* sprintfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 
                 llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                true
            );
            sprintfFunc = llvm::Function::Create(sprintfType, llvm::Function::ExternalLinkage, "sprintf", 
                                                Builder.GetInsertBlock()->getParent()->getParent());
        }

        llvm::Value* bufferPtr = Builder.CreateCall(mallocFunc, {llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 32)});
        if (!bufferPtr) {
            Write("Cast Generation", "Failed to allocate memory for floating-point to string cast" + Location, 2, true, true, "");
            return nullptr;
        }
        llvm::Value* formatStr = Builder.CreateGlobalString("%.6f", "float_to_str_fmt");
        llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
        
        if (ExprValue->getType()->isFloatTy()) {
            ExprValue = Builder.CreateFPExt(ExprValue, Builder.getDoubleTy());
        }
        
        Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, ExprValue});

        return bufferPtr;
    }
    
    Write("Cast Generation", "Unsupported cast from source type to target type" + Location, 2, true, true, "");
    return nullptr;
}