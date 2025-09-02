#pragma once

#include "../Helper/Mapping.hh"
#include "../Helper/Types.hh"
#include "../LLVMHeader.hh"

struct ArrayInfo {
    llvm::Type* elementType;
    llvm::Value* dataPtr;
    llvm::Value* size;
    bool isHeapAllocated;
    
    ArrayInfo() : elementType(nullptr), dataPtr(nullptr), size(nullptr), isHeapAllocated(false) {}
};

class ArrayHandler {
private:
    llvm::IRBuilder<>& Builder;
    std::unordered_map<std::string, ArrayInfo> managedArrays;
    
    llvm::Function* GetMallocFunction();
    llvm::Function* GetFreeFunction();
    llvm::Function* GetAtExitFunction();
    void RegisterArrayForCleanup(llvm::Value* arrayPtr);
    void GenerateCleanupFunction();
    bool CheckBounds(const ArrayInfo& arrayInfo, llvm::Value* index);

public:
    ArrayHandler(llvm::IRBuilder<>& builder);
    
    llvm::StructType* CreateArrayStructType(llvm::Type* elementType);
    
    ArrayInfo CreateArrayWithSize(llvm::Type* elementType, 
                                 llvm::Value* size, 
                                 const std::string& name);
    
    ArrayInfo CreateArrayFromLiteral(ArrayNode* arrayLiteral, llvm::Type* elementType, 
                                    const std::string& name, ScopeStack& SymbolStack, 
                                    FunctionSymbols& Methods);
    
    llvm::Value* GetArrayElement(const ArrayInfo& arrayInfo, llvm::Value* index);
    void SetArrayElement(const ArrayInfo& arrayInfo, llvm::Value* index, llvm::Value* value);
    
    llvm::Value* GetArraySize(const std::string& arrayName);
    
    void RegisterArray(const std::string& name, const ArrayInfo& info);
    ArrayInfo* GetArrayInfo(const std::string& name);
    
    llvm::Value* PassArrayAsParameter(const ArrayInfo& arrayInfo, 
                                     std::vector<llvm::Value*>& args);
};