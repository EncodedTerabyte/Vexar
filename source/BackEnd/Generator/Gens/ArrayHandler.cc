#include "ArrayHandler.hh"
#include "ExpressionGenerator.hh"

ArrayHandler::ArrayHandler(llvm::IRBuilder<>& builder) : Builder(builder) {}

llvm::Function* ArrayHandler::GetMallocFunction() {
    llvm::Module* module = Builder.GetInsertBlock()->getParent()->getParent();
    llvm::Function* mallocFunc = module->getFunction("malloc");
    
    if (!mallocFunc) {
        llvm::FunctionType* mallocType = llvm::FunctionType::get(
            llvm::PointerType::get(Builder.getContext(), 0),
            {llvm::Type::getInt64Ty(Builder.getContext())},
            false
        );
        mallocFunc = llvm::Function::Create(mallocType, llvm::Function::ExternalLinkage, 
                                          "malloc", module);
    }
    
    return mallocFunc;
}

llvm::Function* ArrayHandler::GetFreeFunction() {
    llvm::Module* module = Builder.GetInsertBlock()->getParent()->getParent();
    llvm::Function* freeFunc = module->getFunction("free");
    
    if (!freeFunc) {
        llvm::FunctionType* freeType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(Builder.getContext()),
            {llvm::PointerType::get(Builder.getContext(), 0)},
            false
        );
        freeFunc = llvm::Function::Create(freeType, llvm::Function::ExternalLinkage, 
                                        "free", module);
    }
    
    return freeFunc;
}

llvm::Function* ArrayHandler::GetAtExitFunction() {
    llvm::Module* module = Builder.GetInsertBlock()->getParent()->getParent();
    llvm::Function* atexitFunc = module->getFunction("atexit");
    
    if (!atexitFunc) {
        llvm::FunctionType* atexitType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(Builder.getContext()),
            {llvm::PointerType::get(llvm::FunctionType::get(
                llvm::Type::getVoidTy(Builder.getContext()), {}, false), 0)},
            false
        );
        atexitFunc = llvm::Function::Create(atexitType, llvm::Function::ExternalLinkage, 
                                          "atexit", module);
    }
    
    return atexitFunc;
}

bool ArrayHandler::CheckBounds(const ArrayInfo& arrayInfo, llvm::Value* index) {
    if (!index || !arrayInfo.size) {
        return false;
    }
    
    llvm::ConstantInt* constIndex = llvm::dyn_cast<llvm::ConstantInt>(index);
    if (constIndex) {
        llvm::ConstantInt* constSize = llvm::dyn_cast<llvm::ConstantInt>(arrayInfo.size);
        if (constSize) {
            int64_t idx = constIndex->getSExtValue();
            int64_t size = constSize->getSExtValue();
            
            if (idx < 0 || idx >= size) {
                return false;
            }
        }
    }
    
    return true;
}

void ArrayHandler::RegisterArrayForCleanup(llvm::Value* arrayPtr) {
    static bool cleanupGenerated = false;
    
    if (!cleanupGenerated) {
        GenerateCleanupFunction();
        cleanupGenerated = true;
    }
    
    llvm::Module* module = Builder.GetInsertBlock()->getParent()->getParent();
    llvm::GlobalVariable* arrayList = module->getGlobalVariable("__array_cleanup_list");
    
    if (!arrayList) {
        llvm::Type* ptrType = llvm::PointerType::get(Builder.getContext(), 0);
        llvm::ArrayType* listType = llvm::ArrayType::get(ptrType, 1000);
        
        arrayList = new llvm::GlobalVariable(
            *module, listType, false, llvm::GlobalValue::InternalLinkage,
            llvm::ConstantAggregateZero::get(listType), "__array_cleanup_list"
        );
        
        new llvm::GlobalVariable(
            *module, Builder.getInt32Ty(), false, llvm::GlobalValue::InternalLinkage,
            llvm::ConstantInt::get(Builder.getInt32Ty(), 0), "__array_cleanup_count"
        );
    }
    
    llvm::GlobalVariable* countVar = module->getGlobalVariable("__array_cleanup_count");
    llvm::Value* currentCount = Builder.CreateLoad(Builder.getInt32Ty(), countVar);
    
    llvm::ConstantInt* constCount = llvm::dyn_cast<llvm::ConstantInt>(currentCount);
    if (constCount) {
        if (constCount->getSExtValue() >= 1000) {
            return;
        }
    }
    
    llvm::Value* arrayListPtr = Builder.CreateInBoundsGEP(
        arrayList->getType(), arrayList,
        {Builder.getInt32(0), currentCount}
    );
    
    Builder.CreateStore(arrayPtr, arrayListPtr);
    
    llvm::Value* newCount = Builder.CreateAdd(currentCount, Builder.getInt32(1));
    Builder.CreateStore(newCount, countVar);
}

llvm::StructType* ArrayHandler::CreateArrayStructType(llvm::Type* elementType) {
    if (!elementType) {
        return nullptr;
    }
    
    return llvm::StructType::create(Builder.getContext(), 
        {llvm::PointerType::get(elementType, 0), Builder.getInt32Ty()}, 
        "ArrayStruct");
}

ArrayInfo ArrayHandler::CreateArrayFromLiteral(ArrayNode* arrayLiteral, llvm::Type* elementType, 
                                              const std::string& name, ScopeStack& SymbolStack, 
                                              FunctionSymbols& Methods) {
    ArrayInfo info;
    info.elementType = elementType;
    info.isHeapAllocated = true;
    
    if (!arrayLiteral || !elementType) {
        info.dataPtr = nullptr;
        info.size = Builder.getInt32(0);
        return info;
    }
    
    if (arrayLiteral->elements.empty()) {
        info.dataPtr = nullptr;
        info.size = Builder.getInt32(0);
        return info;
    }
    
    uint64_t size = arrayLiteral->elements.size();
    if (size > 1000000) {
        info.dataPtr = nullptr;
        info.size = Builder.getInt32(0);
        return info;
    }
    
    info.size = Builder.getInt32(size);
    
    llvm::Function* mallocFunc = GetMallocFunction();
    if (!mallocFunc) {
        info.dataPtr = nullptr;
        return info;
    }
    
    uint64_t elementSize = elementType->getPrimitiveSizeInBits() / 8;
    if (elementSize == 0) elementSize = 8;
    
    llvm::Value* totalSize = llvm::ConstantInt::get(llvm::Type::getInt64Ty(Builder.getContext()), 
                                                   size * elementSize);
    llvm::Value* arrayPtr = Builder.CreateCall(mallocFunc, {totalSize});
    
    if (!arrayPtr) {
        info.dataPtr = nullptr;
        return info;
    }
    
    info.dataPtr = Builder.CreatePointerCast(arrayPtr, llvm::PointerType::get(elementType, 0));
    
    RegisterArrayForCleanup(arrayPtr);
    
    for (size_t i = 0; i < size; i++) {
        llvm::Value* elementValue = nullptr;
        
        if (!arrayLiteral->elements[i]) {
            continue;
        }
        
        if (arrayLiteral->elements[i]->type == NodeType::Number) {
            NumberNode* numNode = static_cast<NumberNode*>(arrayLiteral->elements[i].get());
            if (!numNode) {
                continue;
            }
            
            if (elementType->isIntegerTy()) {
                elementValue = llvm::ConstantInt::get(elementType, static_cast<int>(numNode->value));
            } else if (elementType->isFloatingPointTy()) {
                elementValue = llvm::ConstantFP::get(elementType, numNode->value);
            }
        } else {
            elementValue = GenerateExpression(arrayLiteral->elements[i], Builder, SymbolStack, Methods);
        }
        
        if (elementValue) {
            llvm::Value* elementPtr = Builder.CreateInBoundsGEP(elementType, info.dataPtr, 
                                                               Builder.getInt32(i));
            Builder.CreateStore(elementValue, elementPtr);
        }
    }
    
    RegisterArray(name, info);
    return info;
}

ArrayInfo ArrayHandler::CreateArrayWithSize(llvm::Type* elementType, llvm::Value* size, 
                                           const std::string& name) {
    ArrayInfo info;
    info.elementType = elementType;
    info.size = size;
    info.isHeapAllocated = true;
    
    if (!elementType || !size) {
        info.dataPtr = nullptr;
        info.size = Builder.getInt32(0);
        return info;
    }
    
    llvm::ConstantInt* constSize = llvm::dyn_cast<llvm::ConstantInt>(size);
    if (constSize) {
        int64_t sizeVal = constSize->getSExtValue();
        if (sizeVal < 0 || sizeVal > 1000000) {
            info.dataPtr = nullptr;
            info.size = Builder.getInt32(0);
            return info;
        }
    }
    
    llvm::Function* mallocFunc = GetMallocFunction();
    if (!mallocFunc) {
        info.dataPtr = nullptr;
        return info;
    }
    
    uint64_t elementSize = elementType->getPrimitiveSizeInBits() / 8;
    if (elementSize == 0) elementSize = 8;
    
    llvm::Value* elementSizeValue = Builder.getInt64(elementSize);
    llvm::Value* totalSize = Builder.CreateMul(
        Builder.CreateZExtOrTrunc(size, Builder.getInt64Ty()), 
        elementSizeValue
    );
    
    llvm::Value* arrayPtr = Builder.CreateCall(mallocFunc, {totalSize});
    
    if (!arrayPtr) {
        info.dataPtr = nullptr;
        return info;
    }
    
    info.dataPtr = Builder.CreatePointerCast(arrayPtr, llvm::PointerType::get(elementType, 0));
    
    RegisterArrayForCleanup(arrayPtr);
    RegisterArray(name, info);
    
    return info;
}

llvm::Value* ArrayHandler::GetArrayElement(const ArrayInfo& arrayInfo, llvm::Value* index) {
    if (!arrayInfo.dataPtr || !arrayInfo.elementType || !index) {
        return nullptr;
    }
    
    if (!CheckBounds(arrayInfo, index)) {
        return nullptr;
    }
    
    llvm::Value* elementPtr = Builder.CreateInBoundsGEP(arrayInfo.elementType, 
                                                       arrayInfo.dataPtr, index);
    return Builder.CreateLoad(arrayInfo.elementType, elementPtr);
}

void ArrayHandler::SetArrayElement(const ArrayInfo& arrayInfo, llvm::Value* index, llvm::Value* value) {
    if (!arrayInfo.dataPtr || !arrayInfo.elementType || !index || !value) {
        return;
    }
    
    if (!CheckBounds(arrayInfo, index)) {
        return;
    }
    
    llvm::Value* elementPtr = Builder.CreateInBoundsGEP(arrayInfo.elementType, 
                                                       arrayInfo.dataPtr, index);
    Builder.CreateStore(value, elementPtr);
}

llvm::Value* ArrayHandler::GetArraySize(const std::string& arrayName) {
    auto it = managedArrays.find(arrayName);
    if (it != managedArrays.end()) {
        return it->second.size;
    }
    return nullptr;
}

void ArrayHandler::RegisterArray(const std::string& name, const ArrayInfo& info) {
    if (name.empty() || !info.dataPtr) {
        return;
    }
    managedArrays[name] = info;
}

ArrayInfo* ArrayHandler::GetArrayInfo(const std::string& name) {
    if (name.empty()) {
        return nullptr;
    }
    
    auto it = managedArrays.find(name);
    if (it == managedArrays.end()) {
        return nullptr;
    }
    return &it->second;
}

void ArrayHandler::GenerateCleanupFunction() {
    llvm::Module* module = Builder.GetInsertBlock()->getParent()->getParent();
    
    if (!module) {
        return;
    }
    
    llvm::FunctionType* cleanupFuncType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(Builder.getContext()), {}, false
    );
    
    llvm::Function* cleanupFunc = llvm::Function::Create(
        cleanupFuncType, llvm::Function::InternalLinkage, 
        "__cleanup_arrays", module
    );
    
    llvm::BasicBlock* cleanupBB = llvm::BasicBlock::Create(Builder.getContext(), 
                                                          "cleanup", cleanupFunc);
    
    llvm::IRBuilder<> cleanupBuilder(cleanupBB);
    
    llvm::GlobalVariable* arrayList = module->getGlobalVariable("__array_cleanup_list");
    llvm::GlobalVariable* countVar = module->getGlobalVariable("__array_cleanup_count");
    
    if (arrayList && countVar) {
        llvm::Function* freeFunc = GetFreeFunction();
        
        if (!freeFunc) {
            return;
        }
        
        llvm::Value* count = cleanupBuilder.CreateLoad(cleanupBuilder.getInt32Ty(), countVar);
        
        llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(cleanupBuilder.getContext(), 
                                                           "loop", cleanupFunc);
        llvm::BasicBlock* exitBB = llvm::BasicBlock::Create(cleanupBuilder.getContext(), 
                                                           "exit", cleanupFunc);
        
        llvm::Value* i = cleanupBuilder.CreateAlloca(cleanupBuilder.getInt32Ty());
        cleanupBuilder.CreateStore(cleanupBuilder.getInt32(0), i);
        
        cleanupBuilder.CreateBr(loopBB);
        
        cleanupBuilder.SetInsertPoint(loopBB);
        llvm::Value* currentI = cleanupBuilder.CreateLoad(cleanupBuilder.getInt32Ty(), i);
        llvm::Value* condition = cleanupBuilder.CreateICmpSLT(currentI, count);
        
        llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(cleanupBuilder.getContext(), 
                                                           "body", cleanupFunc);
        cleanupBuilder.CreateCondBr(condition, bodyBB, exitBB);
        
        cleanupBuilder.SetInsertPoint(bodyBB);
        llvm::Value* arrayPtr = cleanupBuilder.CreateInBoundsGEP(
            arrayList->getType(), arrayList,
            {cleanupBuilder.getInt32(0), currentI}
        );
        
        if (!arrayPtr) {
            return;
        }
        
        llvm::Value* ptr = cleanupBuilder.CreateLoad(
            llvm::PointerType::get(cleanupBuilder.getContext(), 0), arrayPtr
        );
        cleanupBuilder.CreateCall(freeFunc, {ptr});
        
        llvm::Value* nextI = cleanupBuilder.CreateAdd(currentI, cleanupBuilder.getInt32(1));
        cleanupBuilder.CreateStore(nextI, i);
        cleanupBuilder.CreateBr(loopBB);
        
        cleanupBuilder.SetInsertPoint(exitBB);
    }
    
    cleanupBuilder.CreateRetVoid();
    
    llvm::Function* atexitFunc = GetAtExitFunction();
    Builder.CreateCall(atexitFunc, {cleanupFunc});
}

llvm::Value* ArrayHandler::PassArrayAsParameter(const ArrayInfo& arrayInfo, 
                                              std::vector<llvm::Value*>& args) {
    if (!arrayInfo.dataPtr || !arrayInfo.size) {
        return nullptr;
    }
    
    args.push_back(arrayInfo.dataPtr);
    args.push_back(arrayInfo.size);
    return arrayInfo.dataPtr;
}