#include "GarbageCollector.hh"
#include "ExpressionGenerator.hh"

GarbageCollector gc;

void GarbageCollector::Initialize(llvm::Module* module) {
    llvm::LLVMContext& ctx = module->getContext();
    
    llvm::FunctionType* registerType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(ctx),
        {llvm::PointerType::get(llvm::Type::getInt8Ty(ctx), 0), 
         llvm::Type::getInt64Ty(ctx), 
         llvm::Type::getInt32Ty(ctx)},
        false
    );
    gcRegisterFunc = llvm::Function::Create(registerType, llvm::Function::ExternalLinkage, "gc_register", module);
    
    llvm::FunctionType* collectType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(ctx), {}, false
    );
    gcCollectFunc = llvm::Function::Create(collectType, llvm::Function::ExternalLinkage, "gc_collect", module);
    
    llvm::FunctionType* freeDeepType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(ctx),
        {llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(ctx), 0), 0),
         llvm::Type::getInt32Ty(ctx)},
        false
    );
    freeDeepFunc = llvm::Function::Create(freeDeepType, llvm::Function::ExternalLinkage, "free_ptr_array", module);
    
    llvm::FunctionType* cleanupType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(ctx), {}, false
    );
    gcCleanupFunc = llvm::Function::Create(cleanupType, llvm::Function::ExternalLinkage, "gc_cleanup", module);
}

llvm::Value* GarbageCollector::GenerateTrackedMalloc(llvm::IRBuilder<>& builder, llvm::Value* size) {
    llvm::Function* mallocFunc = builder.GetInsertBlock()->getParent()->getParent()->getFunction("malloc");
    if (!mallocFunc) {
        llvm::FunctionType* mallocType = llvm::FunctionType::get(
            llvm::PointerType::get(llvm::Type::getInt8Ty(builder.getContext()), 0),
            {llvm::Type::getInt64Ty(builder.getContext())},
            false
        );
        mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, "malloc", 
            builder.GetInsertBlock()->getParent()->getParent());
    }
    
    llvm::Value* ptr = builder.CreateCall(mallocFunc, {size});
    llvm::Value* dimensions = llvm::ConstantInt::get(builder.getInt32Ty(), 2);
    builder.CreateCall(gcRegisterFunc, {ptr, size, dimensions});
    
    return ptr;
}

llvm::Value* GarbageCollector::GenerateGetSize(llvm::IRBuilder<>& builder, llvm::Value* ptr) {
    llvm::Function* getSizeFunc = builder.GetInsertBlock()->getParent()->getParent()->getFunction("gc_get_size");
    if (!getSizeFunc) {
        llvm::FunctionType* getSizeType = llvm::FunctionType::get(
            builder.getInt64Ty(),
            {llvm::PointerType::get(builder.getInt8Ty(), 0)},
            false
        );
        getSizeFunc = llvm::Function::Create(getSizeType, llvm::Function::ExternalLinkage, "gc_get_size", 
            builder.GetInsertBlock()->getParent()->getParent());
    }
    
    llvm::Value* size = builder.CreateCall(getSizeFunc, {ptr});
    return builder.CreateTrunc(size, builder.getInt32Ty());
}

void GarbageCollector::GenerateGCCall(llvm::IRBuilder<>& builder) {
    builder.CreateCall(gcCollectFunc, {});
}

void GarbageCollector::GenerateScopeCleanup(llvm::IRBuilder<>& builder) {
    builder.CreateCall(gcCollectFunc, {});
}

void GarbageCollector::GenerateGCCleanup(llvm::IRBuilder<>& builder) {
    builder.CreateCall(gcCleanupFunc, {});
}

llvm::Value* GenerateGCMultiDimensionalArray(ArrayNode* ArrayPtr, llvm::IRBuilder<>& Builder, 
    ScopeStack& SymbolStack, FunctionSymbols& Methods, GarbageCollector& gc) {
    
    if (!ArrayPtr || ArrayPtr->elements.empty()) return nullptr;
    
    if (ArrayPtr->elements[0]->type != NodeType::Array) return nullptr;
    
    size_t outerSize = ArrayPtr->elements.size();
    auto* firstInner = static_cast<ArrayNode*>(ArrayPtr->elements[0].get());
    size_t innerSize = firstInner->elements.size();
    
    llvm::Value* outerArraySize = llvm::ConstantInt::get(Builder.getInt64Ty(), outerSize * 8);
    llvm::Value* outerArray = gc.GenerateTrackedMalloc(Builder, outerArraySize);
    llvm::Value* outerArrayPtr = Builder.CreatePointerCast(outerArray, 
        llvm::PointerType::get(llvm::PointerType::get(Builder.getInt32Ty(), 0), 0));
    
    for (size_t i = 0; i < outerSize; ++i) {
        llvm::Value* innerArraySize = llvm::ConstantInt::get(Builder.getInt64Ty(), innerSize * 4);
        llvm::Value* innerArray = gc.GenerateTrackedMalloc(Builder, innerArraySize);
        llvm::Value* innerArrayPtr = Builder.CreatePointerCast(innerArray, 
            llvm::PointerType::get(Builder.getInt32Ty(), 0));
        
        if (i < ArrayPtr->elements.size()) {
            auto* innerArrayNode = static_cast<ArrayNode*>(ArrayPtr->elements[i].get());
            for (size_t j = 0; j < innerSize && j < innerArrayNode->elements.size(); ++j) {
                llvm::Value* elementValue = GenerateExpression(innerArrayNode->elements[j], Builder, SymbolStack, Methods);
                if (elementValue) {
                    llvm::Value* elementPtr = Builder.CreateInBoundsGEP(Builder.getInt32Ty(), innerArrayPtr, 
                        llvm::ConstantInt::get(Builder.getInt32Ty(), j));
                    Builder.CreateStore(elementValue, elementPtr);
                }
            }
        }
        
        llvm::Value* outerElementPtr = Builder.CreateInBoundsGEP(
            llvm::PointerType::get(Builder.getInt32Ty(), 0), outerArrayPtr, 
            llvm::ConstantInt::get(Builder.getInt32Ty(), i));
        Builder.CreateStore(innerArrayPtr, outerElementPtr);
    }
    
    return outerArrayPtr;
}

void InitializeGCBuiltins(BuiltinSymbols& Builtins, GarbageCollector& gc) {
    Builtins["gc_collect"] = [&gc](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        gc.GenerateGCCall(Builder);
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };
    
    Builtins["free_deep"] = [&gc](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for free_deep function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* ptrValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        if (!ptrValue || !ptrValue->getType()->isPointerTy()) {
            Write("Expression Generation", "Invalid pointer argument for free_deep function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Function* freePtrArrayFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("free_ptr_array");
        llvm::Function* freeFunc = Builder.GetInsertBlock()->getParent()->getParent()->getFunction("free");
        
        if (!freePtrArrayFunc) {
            llvm::FunctionType* freePtrArrayType = llvm::FunctionType::get(
                llvm::Type::getVoidTy(Builder.getContext()),
                {llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 0),
                 llvm::Type::getInt32Ty(Builder.getContext())},
                false
            );
            freePtrArrayFunc = llvm::Function::Create(freePtrArrayType, llvm::Function::ExternalLinkage, "free_ptr_array", 
                Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        if (!freeFunc) {
            llvm::FunctionType* freeType = llvm::FunctionType::get(
                llvm::Type::getVoidTy(Builder.getContext()),
                {llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0)},
                false
            );
            freeFunc = llvm::Function::Create(freeType, llvm::Function::ExternalLinkage, "free", 
                Builder.GetInsertBlock()->getParent()->getParent());
        }
        
        llvm::Value* ptrArrayPtr = Builder.CreatePointerCast(ptrValue, 
            llvm::PointerType::get(llvm::PointerType::get(llvm::Type::getInt8Ty(Builder.getContext()), 0), 0));
        
        llvm::Value* arraySize = llvm::ConstantInt::get(Builder.getInt32Ty(), 2);
        Builder.CreateCall(freePtrArrayFunc, {ptrArrayPtr, arraySize});
        Builder.CreateCall(freeFunc, {ptrValue});
        
        return llvm::ConstantInt::get(Builder.getInt32Ty(), 0);
    };
    
    Builtins["malloc_tracked"] = [&gc](const std::vector<std::unique_ptr<ASTNode>>& args, llvm::IRBuilder<>& Builder, ScopeStack& SymbolStack, FunctionSymbols& Methods) -> llvm::Value* {
        if (args.empty()) {
            Write("Expression Generation", "Empty arguments for malloc_tracked function", 2, true, true, "");
            return nullptr;
        }
        
        llvm::Value* sizeValue = GenerateExpression(args[0], Builder, SymbolStack, Methods);
        if (!sizeValue || !sizeValue->getType()->isIntegerTy()) {
            Write("Expression Generation", "Invalid size argument for malloc_tracked", 2, true, true, "");
            return nullptr;
        }
        
        if (sizeValue->getType() != Builder.getInt64Ty()) {
            sizeValue = Builder.CreateZExt(sizeValue, Builder.getInt64Ty());
        }
        
        return gc.GenerateTrackedMalloc(Builder, sizeValue);
    };
}