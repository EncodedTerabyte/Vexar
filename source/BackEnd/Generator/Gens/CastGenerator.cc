#include "CastGenerator.hh"
#include "ExpressionGenerator.hh"

llvm::Value* GenerateCast(const std::unique_ptr<ASTNode>& Expr, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) {
       auto* CastNodePtr = static_cast<CastNode*>(Expr.get());
       llvm::Value* ExprValue = GenerateExpression(CastNodePtr->expr, Builder, SymbolStack, Methods);
       
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
           exit(1);
       }
       
       llvm::Type* SourceType = ExprValue->getType();
       
       if (SourceType == TargetType) {
           return ExprValue;
       }
       
       if (SourceType->isIntegerTy() && TargetType->isIntegerTy()) {
           if (SourceType->getIntegerBitWidth() < TargetType->getIntegerBitWidth()) {
               return Builder.CreateSExt(ExprValue, TargetType);
           } else if (SourceType->getIntegerBitWidth() > TargetType->getIntegerBitWidth()) {
               return Builder.CreateTrunc(ExprValue, TargetType);
           }
       } else if (SourceType->isIntegerTy() && TargetType->isFloatingPointTy()) {
           return Builder.CreateSIToFP(ExprValue, TargetType);
       } else if (SourceType->isFloatingPointTy() && TargetType->isIntegerTy()) {
           return Builder.CreateFPToSI(ExprValue, TargetType);
       } else if (SourceType->isFloatingPointTy() && TargetType->isFloatingPointTy()) {
           if (SourceType->isFloatTy() && TargetType->isDoubleTy()) {
               return Builder.CreateFPExt(ExprValue, TargetType);
           } else if (SourceType->isDoubleTy() && TargetType->isFloatTy()) {
               return Builder.CreateFPTrunc(ExprValue, TargetType);
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
           Builder.CreateStore(ExprValue, charPtr);
           llvm::Value* nullPtr = Builder.CreateGEP(llvm::Type::getInt8Ty(Builder.getContext()), charPtr, llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 1));
           Builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), nullPtr);
           return charPtr;
       } else if (SourceType->isPointerTy() && TargetType->isIntegerTy()) {
           llvm::Value* firstChar = Builder.CreateLoad(llvm::Type::getInt8Ty(Builder.getContext()), ExprValue);
           if (TargetType->isIntegerTy(32)) {
               return Builder.CreateSExt(firstChar, TargetType);
           } else if (TargetType->isIntegerTy(8)) {
               return firstChar;
           } else {
               return Builder.CreateSExt(firstChar, TargetType);
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
               return Builder.CreateFPTrunc(doubleResult, TargetType);
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
           llvm::Value* formatStr = Builder.CreateGlobalString("%.6f", "float_to_str_fmt");
           llvm::Value* formatPtr = Builder.CreatePointerCast(formatStr, llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0));
           
           if (ExprValue->getType()->isFloatTy()) {
               ExprValue = Builder.CreateFPExt(ExprValue, Builder.getDoubleTy());
           }
           
           Builder.CreateCall(sprintfFunc, {bufferPtr, formatPtr, ExprValue});
           return bufferPtr;
       }
}