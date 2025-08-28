#include "ExpressionGenerator.hh"
#include "DefaultSymbols.hh"

class GarbageCollector {
private:
    llvm::Function* gcRegisterFunc;
    llvm::Function* gcCollectFunc;
    llvm::Function* freeDeepFunc;
    llvm::Function* gcCleanupFunc;
    
public:
    void Initialize(llvm::Module* module);
    llvm::Value* GenerateTrackedMalloc(llvm::IRBuilder<>& builder, llvm::Value* size);
    llvm::Value* GarbageCollector::GenerateGetSize(llvm::IRBuilder<>& builder, llvm::Value* ptr);
    void GenerateGCCall(llvm::IRBuilder<>& builder);
    void GenerateScopeCleanup(llvm::IRBuilder<>& builder);
    void GenerateGCCleanup(llvm::IRBuilder<>& builder);
};

extern GarbageCollector gc;
void InitializeGCBuiltins(BuiltinSymbols& Builtins, GarbageCollector& gc);