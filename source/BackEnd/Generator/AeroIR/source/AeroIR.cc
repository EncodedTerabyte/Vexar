// Aero - an LLVM wrapper library
// Copyright 2025 EncodedTerabyte
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0

#include "AeroIR.hh"

AeroIR::AeroIR(const std::string& moduleName) {
    context = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>(moduleName, *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);
    currentFunction = nullptr;
    pushScope();
    setupBuiltins();
}

void AeroIR::setupBuiltins() {
    mallocFunc = llvm::Function::Create(
        llvm::FunctionType::get(i8ptr(), {i64()}, false),
        llvm::Function::ExternalLinkage, "malloc", *module);
    
    freeFunc = llvm::Function::Create(
        llvm::FunctionType::get(void_t(), {i8ptr()}, false),
        llvm::Function::ExternalLinkage, "free", *module);
        
    gcCleanupFunc = createBuiltinFunction("__gc_cleanup", void_t(), {});
    llvm::BasicBlock* gcEntry = llvm::BasicBlock::Create(*context, "entry", gcCleanupFunc);
    builder->SetInsertPoint(gcEntry);
    for (auto ptr : gcPointers) {
        builder->CreateCall(freeFunc, {ptr});
    }
    builder->CreateRetVoid();
}

void AeroIR::pushScope() {
    scopes.push(std::unordered_map<std::string, llvm::Value*>());
}

void AeroIR::popScope() {
    if (!scopes.empty()) scopes.pop();
}

llvm::Type* AeroIR::i8() { return llvm::Type::getInt8Ty(*context); }
llvm::Type* AeroIR::i16() { return llvm::Type::getInt16Ty(*context); }
llvm::Type* AeroIR::i32() { return llvm::Type::getInt32Ty(*context); }
llvm::Type* AeroIR::i64() { return llvm::Type::getInt64Ty(*context); }
llvm::Type* AeroIR::f32() { return llvm::Type::getFloatTy(*context); }
llvm::Type* AeroIR::f64() { return llvm::Type::getDoubleTy(*context); }
llvm::Type* AeroIR::void_t() { return llvm::Type::getVoidTy(*context); }
llvm::Type* AeroIR::i8ptr() { return llvm::PointerType::get(i8(), 0); }
llvm::Type* AeroIR::char_t() { return i8(); }
llvm::Type* AeroIR::int_t() { return i32(); }
llvm::Type* AeroIR::float_t() { return f32(); }
llvm::Type* AeroIR::double_t() { return f64(); }
llvm::Type* AeroIR::string_t() { return i8ptr(); }
llvm::Type* AeroIR::bool_t() { return llvm::Type::getInt1Ty(*context); }

llvm::Type* AeroIR::ptr(llvm::Type* elemType) { 
    return llvm::PointerType::get(elemType, 0); 
}

llvm::Type* AeroIR::array(llvm::Type* elemType, int size) { 
    return llvm::ArrayType::get(elemType, size); 
}

llvm::Type* AeroIR::struct_t(const std::vector<llvm::Type*>& fields) {
    return llvm::StructType::get(*context, fields);
}

void AeroIR::defineCustomType(const std::string& name, const std::vector<llvm::Type*>& fieldTypes, 
                        const std::vector<std::string>& fieldNames) {
    llvm::Type* structType = llvm::StructType::get(*context, fieldTypes);
    CustomType customType;
    customType.type = structType;
    for (size_t i = 0; i < fieldNames.size(); ++i) {
        customType.fieldIndices[fieldNames[i]] = i;
    }
    customTypes[name] = customType;
}

void AeroIR::addMethod(const std::string& typeName, const std::string& methodName, llvm::Function* method) {
    if (customTypes.count(typeName)) {
        customTypes[typeName].methods[methodName] = method;
    }
}

llvm::Type* AeroIR::getCustomType(const std::string& name) {
    return customTypes.count(name) ? customTypes[name].type : nullptr;
}

AeroIR::CustomType* AeroIR::getCustomTypeInfo(const std::string& name) {
    return customTypes.count(name) ? &customTypes[name] : nullptr;
}

llvm::Value* AeroIR::createStruct(const std::string& typeName) {
    llvm::Type* type = getCustomType(typeName);
    if (!type) return nullptr;
    return llvm::UndefValue::get(type);
}

llvm::Value* AeroIR::fieldAccess(llvm::Value* structPtr, const std::string& fieldName, const std::string& typeName) {
    CustomType* typeInfo = getCustomTypeInfo(typeName);
    if (!typeInfo || typeInfo->fieldIndices.find(fieldName) == typeInfo->fieldIndices.end()) {
        return nullptr;
    }
    int fieldIndex = typeInfo->fieldIndices[fieldName];
    return builder->CreateStructGEP(typeInfo->type, structPtr, fieldIndex);
}

llvm::Value* AeroIR::fieldAccess(const std::string& varName, const std::string& fieldName) {
    llvm::Value* structPtr = getVar(varName);
    if (!structPtr) return nullptr;
    
    for (auto& [typeName, typeInfo] : customTypes) {
        if (typeInfo.fieldIndices.find(fieldName) != typeInfo.fieldIndices.end()) {
            return fieldAccess(structPtr, fieldName, typeName);
        }
    }
    return nullptr;
}

void AeroIR::fieldStore(llvm::Value* structPtr, const std::string& fieldName, const std::string& typeName, llvm::Value* val) {
    llvm::Value* fieldPtr = fieldAccess(structPtr, fieldName, typeName);
    if (fieldPtr) store(val, fieldPtr);
}

void AeroIR::fieldStore(const std::string& varName, const std::string& fieldName, llvm::Value* val) {
    llvm::Value* fieldPtr = fieldAccess(varName, fieldName);
    if (fieldPtr) store(val, fieldPtr);
}

llvm::Value* AeroIR::methodCall(llvm::Value* structPtr, const std::string& methodName, const std::string& typeName, 
                                 const std::vector<llvm::Value*>& args) {
    CustomType* typeInfo = getCustomTypeInfo(typeName);
    if (!typeInfo || typeInfo->methods.find(methodName) == typeInfo->methods.end()) {
        return nullptr;
    }
    llvm::Function* method = typeInfo->methods[methodName];
    std::vector<llvm::Value*> allArgs = {structPtr};
    allArgs.insert(allArgs.end(), args.begin(), args.end());
    return builder->CreateCall(method, allArgs);
}

llvm::Value* AeroIR::methodCall(const std::string& varName, const std::string& methodName, 
                                 const std::vector<llvm::Value*>& args) {
    llvm::Value* structPtr = getVar(varName);
    if (!structPtr) return nullptr;
    
    for (auto& [typeName, typeInfo] : customTypes) {
        if (typeInfo.methods.find(methodName) != typeInfo.methods.end()) {
            return methodCall(structPtr, methodName, typeName, args);
        }
    }
    return nullptr;
}

llvm::Value* AeroIR::constI8(int val) { return llvm::ConstantInt::get(i8(), val); }
llvm::Value* AeroIR::constI16(int val) { return llvm::ConstantInt::get(i16(), val); }
llvm::Value* AeroIR::constI32(int val) { return llvm::ConstantInt::get(i32(), val); }
llvm::Value* AeroIR::constI64(long val) { return llvm::ConstantInt::get(i64(), val); }
llvm::Value* AeroIR::constF32(float val) { return llvm::ConstantFP::get(f32(), val); }
llvm::Value* AeroIR::constF64(double val) { return llvm::ConstantFP::get(f64(), val); }
llvm::Value* AeroIR::constBool(bool val) { return llvm::ConstantInt::get(bool_t(), val); }
llvm::Value* AeroIR::constChar(char val) { return llvm::ConstantInt::get(char_t(), val); }

llvm::Value* AeroIR::constString(const std::string& str) {
    return builder->CreateGlobalStringPtr(str);
}

llvm::Function* AeroIR::createFunction(const std::string& name, llvm::Type* retType, 
                               const std::vector<llvm::Type*>& paramTypes) {
    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, false);
    llvm::Function* func = llvm::Function::Create(funcType, 
                                                  llvm::Function::ExternalLinkage, 
                                                  name, *module);
    currentFunction = func;
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context, "entry", func);
    builder->SetInsertPoint(entry);
    pushScope();
    return func;
}

llvm::Function* AeroIR::createBuiltinFunction(const std::string& name, llvm::Type* retType,
                                      const std::vector<llvm::Type*>& paramTypes) {
    llvm::FunctionType* funcType = llvm::FunctionType::get(retType, paramTypes, false);
    llvm::Function* func = llvm::Function::Create(funcType,
                                                  llvm::Function::ExternalLinkage,
                                                  name, *module);
    builtinFuncs[name] = func;
    return func;
}

llvm::Function* AeroIR::getBuiltinFunction(const std::string& name) {
    return builtinFuncs.count(name) ? builtinFuncs[name] : nullptr;
}

void AeroIR::endFunction() {
    popScope();
    currentFunction = nullptr;
}

llvm::Value* AeroIR::ret(llvm::Value* val) {
    if (val) {
        return builder->CreateRet(val);
    } else {
        return builder->CreateRetVoid();
    }
}

llvm::Value* AeroIR::param(int index) {
    if (!currentFunction) return nullptr;
    auto it = currentFunction->arg_begin();
    std::advance(it, index);
    return &*it;
}

void AeroIR::nameParam(int index, const std::string& name) {
    llvm::Value* p = param(index);
    if (p) {
        p->setName(name);
        setVar(name, p);
    }
}

llvm::Value* AeroIR::var(const std::string& name, llvm::Type* type, llvm::Value* init) {
    llvm::Value* alloca = builder->CreateAlloca(type, nullptr, name);
    if (init) builder->CreateStore(init, alloca);
    setVar(name, alloca);
    return alloca;
}

llvm::Value* AeroIR::var(const std::string& name, const std::string& typeName, llvm::Value* init) {
    llvm::Type* type = getCustomType(typeName);
    if (!type) return nullptr;
    return var(name, type, init);
}

llvm::Value* AeroIR::getVar(const std::string& name) {
    auto tempScopes = scopes;
    while (!tempScopes.empty()) {
        auto& scope = tempScopes.top();
        if (scope.find(name) != scope.end()) {
            return scope[name];
        }
        tempScopes.pop();
    }
    return nullptr;
}

void AeroIR::setVar(const std::string& name, llvm::Value* val) {
    if (!scopes.empty()) scopes.top()[name] = val;
}

void AeroIR::reassign(const std::string& name, llvm::Value* val) {
    llvm::Value* ptr = getVar(name);
    if (ptr) store(val, ptr);
}

void AeroIR::reassign(llvm::Value* ptr, llvm::Value* val) {
    store(val, ptr);
}

llvm::Value* AeroIR::load(llvm::Value* ptr) {
    if (!ptr->getType()->isPointerTy()) {
        return ptr;
    }
    
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(ptr)) {
        return builder->CreateLoad(alloca->getAllocatedType(), ptr);
    }
    
    return builder->CreateLoad(i32(), ptr);
}

llvm::Value* AeroIR::load(llvm::Value* ptr, llvm::Type* elementType) {
    if (!ptr->getType()->isPointerTy()) {
        return ptr;
    }
    
    return builder->CreateLoad(elementType, ptr);
}

void AeroIR::store(llvm::Value* val, llvm::Value* ptr) {
    builder->CreateStore(val, ptr);
}

void AeroIR::store(llvm::Value* val, const std::string& name) {
    llvm::Value* ptr = getVar(name);
    if (ptr) store(val, ptr);
}

llvm::Value* AeroIR::stackArray(const std::string& name, llvm::Type* elemType, int size) {
    llvm::Type* arrayType = array(elemType, size);
    return var(name, arrayType);
}

llvm::Value* AeroIR::heapArray(const std::string& name, llvm::Type* elemType, llvm::Value* size) {
    llvm::DataLayout DL = module->getDataLayout();
    uint64_t elemSize = DL.getTypeAllocSize(elemType);
    llvm::Value* totalSize = mul(size, constI64(elemSize));
    llvm::Value* ptr = builder->CreateCall(mallocFunc, {totalSize});
    gcPointers.insert(ptr);
    setVar(name, ptr);
    return ptr;
}

llvm::Value* AeroIR::arrayAccess(llvm::Value* arrayPtr, llvm::Value* index) {
    if (!arrayPtr->getType()->isPointerTy()) return nullptr;
    
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(arrayPtr)) {
        llvm::Type* allocatedType = alloca->getAllocatedType();
        if (allocatedType->isArrayTy()) {
            return builder->CreateInBoundsGEP(allocatedType, arrayPtr, {constI32(0), index});
        }
    }
    
    return builder->CreateInBoundsGEP(i8(), arrayPtr, index);
}

llvm::Value* AeroIR::arrayAccess(const std::string& arrayName, llvm::Value* index) {
    llvm::Value* arrayPtr = getVar(arrayName);
    return arrayPtr ? arrayAccess(arrayPtr, index) : nullptr;
}

llvm::Value* AeroIR::malloc(llvm::Type* type, llvm::Value* count) {
    llvm::DataLayout DL = module->getDataLayout();
    uint64_t elemSize = DL.getTypeAllocSize(type);
    llvm::Value* size = constI64(elemSize);
    if (count) size = mul(size, count);
    llvm::Value* ptr = builder->CreateCall(mallocFunc, {size});
    gcPointers.insert(ptr);
    return ptr;
}

void AeroIR::free(llvm::Value* ptr) {
    builder->CreateCall(freeFunc, {ptr});
    gcPointers.erase(ptr);
}

void AeroIR::gcCleanup() {
    builder->CreateCall(gcCleanupFunc);
}

llvm::Value* AeroIR::add(llvm::Value* lhs, llvm::Value* rhs) {
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateAdd(lhs, rhs) : builder->CreateFAdd(lhs, rhs);
}

llvm::Value* AeroIR::sub(llvm::Value* lhs, llvm::Value* rhs) {
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateSub(lhs, rhs) : builder->CreateFSub(lhs, rhs);
}

llvm::Value* AeroIR::mul(llvm::Value* lhs, llvm::Value* rhs) {
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateMul(lhs, rhs) : builder->CreateFMul(lhs, rhs);
}

llvm::Value* AeroIR::div(llvm::Value* lhs, llvm::Value* rhs) {
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateSDiv(lhs, rhs) : builder->CreateFDiv(lhs, rhs);
}

llvm::Value* AeroIR::mod(llvm::Value* lhs, llvm::Value* rhs) {
    return builder->CreateSRem(lhs, rhs);
}

llvm::Value* AeroIR::neg(llvm::Value* val) {
    return val->getType()->isIntegerTy() ? 
           builder->CreateNeg(val) : builder->CreateFNeg(val);
}

llvm::Value* AeroIR::eq(llvm::Value* lhs, llvm::Value* rhs) {
    if (lhs->getType() != rhs->getType()) {
        if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
            unsigned lhsBits = lhs->getType()->getIntegerBitWidth();
            unsigned rhsBits = rhs->getType()->getIntegerBitWidth();
            if (lhsBits < rhsBits) {
                lhs = builder->CreateZExt(lhs, rhs->getType());
            } else if (rhsBits < lhsBits) {
                rhs = builder->CreateZExt(rhs, lhs->getType());
            }
        }
    }
    
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateICmpEQ(lhs, rhs) : builder->CreateFCmpOEQ(lhs, rhs);
}

llvm::Value* AeroIR::ne(llvm::Value* lhs, llvm::Value* rhs) {
    if (lhs->getType()->isPointerTy() || rhs->getType()->isPointerTy()) {
        return builder->CreateICmpNE(lhs, rhs);
    }
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateICmpNE(lhs, rhs) : builder->CreateFCmpONE(lhs, rhs);
}

llvm::Value* AeroIR::lt(llvm::Value* lhs, llvm::Value* rhs) {
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateICmpSLT(lhs, rhs) : builder->CreateFCmpOLT(lhs, rhs);
}

llvm::Value* AeroIR::le(llvm::Value* lhs, llvm::Value* rhs) {
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateICmpSLE(lhs, rhs) : builder->CreateFCmpOLE(lhs, rhs);
}

llvm::Value* AeroIR::gt(llvm::Value* lhs, llvm::Value* rhs) {
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateICmpSGT(lhs, rhs) : builder->CreateFCmpOGT(lhs, rhs);
}

llvm::Value* AeroIR::ge(llvm::Value* lhs, llvm::Value* rhs) {
    return lhs->getType()->isIntegerTy() ? 
           builder->CreateICmpSGE(lhs, rhs) : builder->CreateFCmpOGE(lhs, rhs);
}

llvm::Value* AeroIR::and_(llvm::Value* lhs, llvm::Value* rhs) {
    return builder->CreateAnd(lhs, rhs);
}

llvm::Value* AeroIR::or_(llvm::Value* lhs, llvm::Value* rhs) {
    return builder->CreateOr(lhs, rhs);
}

llvm::Value* AeroIR::not_(llvm::Value* val) {
    return builder->CreateNot(val);
}

llvm::Value* AeroIR::bitAnd(llvm::Value* lhs, llvm::Value* rhs) {
    return builder->CreateAnd(lhs, rhs);
}

llvm::Value* AeroIR::bitOr(llvm::Value* lhs, llvm::Value* rhs) {
    return builder->CreateOr(lhs, rhs);
}

llvm::Value* AeroIR::bitXor(llvm::Value* lhs, llvm::Value* rhs) {
    return builder->CreateXor(lhs, rhs);
}

llvm::Value* AeroIR::bitNot(llvm::Value* val) {
    return builder->CreateNot(val);
}

llvm::Value* AeroIR::shl(llvm::Value* lhs, llvm::Value* rhs) {
    return builder->CreateShl(lhs, rhs);
}

llvm::Value* AeroIR::shr(llvm::Value* lhs, llvm::Value* rhs) {
    return builder->CreateLShr(lhs, rhs);
}

llvm::Value* AeroIR::ashr(llvm::Value* lhs, llvm::Value* rhs) {
    return builder->CreateAShr(lhs, rhs);
}

llvm::Value* AeroIR::cast(llvm::Value* val, llvm::Type* toType) {
    return builder->CreateBitCast(val, toType);
}

llvm::Value* AeroIR::intCast(llvm::Value* val, llvm::Type* toType) {
    return builder->CreateIntCast(val, toType, true);
}

llvm::Value* AeroIR::floatCast(llvm::Value* val, llvm::Type* toType) {
    return builder->CreateFPCast(val, toType);
}

llvm::Value* AeroIR::call(llvm::Function* func, const std::vector<llvm::Value*>& args) {
    return builder->CreateCall(func, args);
}

llvm::Value* AeroIR::call(const std::string& funcName, const std::vector<llvm::Value*>& args) {
    llvm::Function* func = module->getFunction(funcName);
    if (!func) func = getBuiltinFunction(funcName);
    return func ? call(func, args) : nullptr;
}

llvm::BasicBlock* AeroIR::createBlock(const std::string& name) {
    return llvm::BasicBlock::Create(*context, name, currentFunction);
}

void AeroIR::setInsertPoint(llvm::BasicBlock* block) {
    builder->SetInsertPoint(block);
}

llvm::Value* AeroIR::branch(llvm::BasicBlock* dest) {
    return builder->CreateBr(dest);
}

llvm::Value* AeroIR::condBranch(llvm::Value* cond, llvm::BasicBlock* trueBlock, llvm::BasicBlock* falseBlock) {
    return builder->CreateCondBr(cond, trueBlock, falseBlock);
}

llvm::BasicBlock* AeroIR::createWhileLoop(llvm::Value* condition, llvm::BasicBlock* body, llvm::BasicBlock* afterLoop) {
    llvm::BasicBlock* condBlock = createBlock("while.cond");
    if (!afterLoop) afterLoop = createBlock("while.end");
    
    branch(condBlock);
    setInsertPoint(condBlock);
    condBranch(condition, body, afterLoop);
    
    return afterLoop;
}

llvm::BasicBlock* AeroIR::createForLoop(llvm::Value* init, llvm::Value* condition, llvm::Value* increment, 
                                         llvm::BasicBlock* body, llvm::BasicBlock* afterLoop) {
    llvm::BasicBlock* condBlock = createBlock("for.cond");
    llvm::BasicBlock* incBlock = createBlock("for.inc");
    if (!afterLoop) afterLoop = createBlock("for.end");
    
    branch(condBlock);
    
    setInsertPoint(condBlock);
    condBranch(condition, body, afterLoop);
    
    setInsertPoint(incBlock);
    branch(condBlock);
    
    return afterLoop;
}

void AeroIR::breakLoop(llvm::BasicBlock* afterLoop) {
    branch(afterLoop);
}

void AeroIR::continueLoop(llvm::BasicBlock* conditionBlock) {
    branch(conditionBlock);
}

void AeroIR::registerBuiltin(const std::string& name, llvm::Function* func) {
    builtinFuncs[name] = func;
}

void AeroIR::registerBuiltin(const std::string& name, llvm::Type* retType, const std::vector<llvm::Type*>& paramTypes) {
    llvm::Function* func = createBuiltinFunction(name, retType, paramTypes);
    builtinFuncs[name] = func;
}

llvm::Function* AeroIR::getRegisteredBuiltin(const std::string& name) {
    return builtinFuncs.count(name) ? builtinFuncs[name] : nullptr;
}

void AeroIR::print() {
    module->print(llvm::outs(), nullptr);
}

bool AeroIR::verify() {
    return !llvm::verifyModule(*module, &llvm::errs());
}

llvm::Module* AeroIR::getModule() { return module.get(); }
llvm::LLVMContext* AeroIR::getContext() { return context.get(); }
llvm::IRBuilder<>* AeroIR::getBuilder() { return builder.get(); }