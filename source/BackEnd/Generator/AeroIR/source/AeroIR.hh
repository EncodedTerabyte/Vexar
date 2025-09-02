// Aero - an LLVM wrapper library
// Copyright 2025 EncodedTerabyte
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/LegacyPassManager.h>

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>

#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <llvm/TargetParser/Host.h>
#include <llvm/MC/TargetRegistry.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>

#include <llvm/Bitcode/BitcodeWriter.h>

#include <lld/Common/Driver.h>

#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>

#include <llvm/TargetParser/Triple.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <stack>
#include <set>

class AeroIR {
public:
    struct CustomType {
        llvm::Type* type;
        std::unordered_map<std::string, int> fieldIndices;
        std::unordered_map<std::string, llvm::Function*> methods;
    };

private:
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::stack<std::unordered_map<std::string, llvm::Value*>> scopes;
    std::unordered_map<std::string, CustomType> customTypes;
    std::unordered_map<std::string, llvm::Function*> builtinFuncs;
    std::set<llvm::Value*> gcPointers;
    llvm::Function* currentFunction;
    llvm::Function* mallocFunc;
    llvm::Function* freeFunc;
    llvm::Function* gcCleanupFunc;
    
    void setupBuiltins();
    
public:
    AeroIR(const std::string& moduleName);
    
    void pushScope();
    void popScope();
    
    llvm::Type* i8();
    llvm::Type* i16();
    llvm::Type* i32();
    llvm::Type* i64();
    llvm::Type* f32();
    llvm::Type* f64();
    llvm::Type* void_t();
    llvm::Type* i8ptr();
    llvm::Type* char_t();
    llvm::Type* int_t();
    llvm::Type* float_t();
    llvm::Type* double_t();
    llvm::Type* string_t();
    llvm::Type* bool_t();
    llvm::Type* ptr(llvm::Type* elemType);
    llvm::Type* array(llvm::Type* elemType, int size);
    llvm::Type* struct_t(const std::vector<llvm::Type*>& fields);
    
    void defineCustomType(const std::string& name, const std::vector<llvm::Type*>& fieldTypes, 
                          const std::vector<std::string>& fieldNames);
    void addMethod(const std::string& typeName, const std::string& methodName, llvm::Function* method);
    llvm::Type* getCustomType(const std::string& name);
    CustomType* getCustomTypeInfo(const std::string& name);
    
    llvm::Value* createStruct(const std::string& typeName);
    llvm::Value* fieldAccess(llvm::Value* structPtr, const std::string& fieldName, const std::string& typeName);
    llvm::Value* fieldAccess(const std::string& varName, const std::string& fieldName);
    void fieldStore(llvm::Value* structPtr, const std::string& fieldName, const std::string& typeName, llvm::Value* val);
    void fieldStore(const std::string& varName, const std::string& fieldName, llvm::Value* val);
    llvm::Value* methodCall(llvm::Value* structPtr, const std::string& methodName, const std::string& typeName, 
                           const std::vector<llvm::Value*>& args = {});
    llvm::Value* methodCall(const std::string& varName, const std::string& methodName, 
                           const std::vector<llvm::Value*>& args = {});
    
    llvm::Value* constI8(int val);
    llvm::Value* constI16(int val);
    llvm::Value* constI32(int val);
    llvm::Value* constI64(long val);
    llvm::Value* constF32(float val);
    llvm::Value* constF64(double val);
    llvm::Value* constBool(bool val);
    llvm::Value* constChar(char val);
    llvm::Value* constString(const std::string& str);
    
    llvm::Function* createFunction(const std::string& name, llvm::Type* retType, 
                                   const std::vector<llvm::Type*>& paramTypes);
    llvm::Function* createBuiltinFunction(const std::string& name, llvm::Type* retType,
                                          const std::vector<llvm::Type*>& paramTypes);
    llvm::Function* getBuiltinFunction(const std::string& name);
    void endFunction();
    llvm::Value* ret(llvm::Value* val = nullptr);
    
    llvm::Value* param(int index);
    void nameParam(int index, const std::string& name);
    
    llvm::Value* var(const std::string& name, llvm::Type* type, llvm::Value* init = nullptr);
    llvm::Value* var(const std::string& name, const std::string& typeName, llvm::Value* init = nullptr);
    llvm::Value* getVar(const std::string& name);
    void setVar(const std::string& name, llvm::Value* val);
    void reassign(const std::string& name, llvm::Value* val);
    void reassign(llvm::Value* ptr, llvm::Value* val);
    
    llvm::Value* load(llvm::Value* ptr);
    llvm::Value* load(llvm::Value* ptr, llvm::Type* elementType);
    llvm::Value* load(const std::string& name);
    void store(llvm::Value* val, llvm::Value* ptr);
    void store(llvm::Value* val, const std::string& name);
    
    llvm::Value* stackArray(const std::string& name, llvm::Type* elemType, int size);
    llvm::Value* heapArray(const std::string& name, llvm::Type* elemType, llvm::Value* size);
    llvm::Value* arrayAccess(llvm::Value* arrayPtr, llvm::Value* index);
    llvm::Value* arrayAccess(const std::string& arrayName, llvm::Value* index);
    
    llvm::Value* malloc(llvm::Type* type, llvm::Value* count = nullptr);
    void free(llvm::Value* ptr);
    void gcCleanup();
    
    llvm::Value* add(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* sub(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* mul(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* div(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* mod(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* neg(llvm::Value* val);
    
    llvm::Value* eq(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* ne(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* lt(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* le(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* gt(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* ge(llvm::Value* lhs, llvm::Value* rhs);
    
    llvm::Value* and_(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* or_(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* not_(llvm::Value* val);
    
    llvm::Value* bitAnd(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* bitOr(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* bitXor(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* bitNot(llvm::Value* val);
    llvm::Value* shl(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* shr(llvm::Value* lhs, llvm::Value* rhs);
    llvm::Value* ashr(llvm::Value* lhs, llvm::Value* rhs);
    
    llvm::Value* cast(llvm::Value* val, llvm::Type* toType);
    llvm::Value* intCast(llvm::Value* val, llvm::Type* toType);
    llvm::Value* floatCast(llvm::Value* val, llvm::Type* toType);
    
    llvm::Value* call(llvm::Function* func, const std::vector<llvm::Value*>& args = {});
    llvm::Value* call(const std::string& funcName, const std::vector<llvm::Value*>& args = {});
    
    llvm::BasicBlock* createBlock(const std::string& name);
    void setInsertPoint(llvm::BasicBlock* block);
    llvm::Value* branch(llvm::BasicBlock* dest);
    llvm::Value* condBranch(llvm::Value* cond, llvm::BasicBlock* trueBlock, llvm::BasicBlock* falseBlock);
    
    llvm::BasicBlock* createWhileLoop(llvm::Value* condition, llvm::BasicBlock* body, llvm::BasicBlock* afterLoop = nullptr);
    llvm::BasicBlock* createForLoop(llvm::Value* init, llvm::Value* condition, llvm::Value* increment, 
                                   llvm::BasicBlock* body, llvm::BasicBlock* afterLoop = nullptr);
    void breakLoop(llvm::BasicBlock* afterLoop);
    void continueLoop(llvm::BasicBlock* conditionBlock);
    
    void registerBuiltin(const std::string& name, llvm::Function* func);
    void registerBuiltin(const std::string& name, llvm::Type* retType, const std::vector<llvm::Type*>& paramTypes);
    llvm::Function* getRegisteredBuiltin(const std::string& name);
    
    void print();
    bool verify();
    
    llvm::Module* getModule();
    llvm::LLVMContext* getContext();
    llvm::IRBuilder<>* getBuilder();
};