#include "FunctionGenerator.hh"
#include "BlockGenerator.hh"
#include "../Helper/Types.hh"
#include "../Helper/Mapping.hh"
#include "GarbageCollector.hh"

llvm::Function* GenerateFunction(FunctionNode* Node, llvm::Module* Module, AllocaSymbols& GlobalAllocas, FunctionSymbols& Methods) {
    if (!Node) {
        Write("Function Generation", "Null FunctionNode provided", 2, true, true, "");
        return nullptr;
    }

    llvm::LLVMContext& ctx = Module->getContext();
    std::string Name = Node->name;
    std::string ReturnTypeStr = Node->returnType;
    std::string Location = " at line " + std::to_string(Node->token.line) + ", column " + std::to_string(Node->token.column);

    llvm::Type* ReturnType = GetLLVMTypeFromStringWithArrays(ReturnTypeStr, ctx);
    if (!ReturnType) {
        Write("Function Generation", "Unknown return type: " + ReturnTypeStr + Location, 2, true, true, "");
        return nullptr;
    }

    std::vector<llvm::Type*> ArgTypes;
    for (const auto& Arg : Node->params) {
        std::string paramType = std::get<1>(Arg);
        int dimensions = std::get<2>(Arg);
        
        llvm::Type* ArgType = GetLLVMTypeFromStringWithArrays(paramType, ctx);
        if (!ArgType) {
            Write("Function Generation", "Unknown argument type: " + paramType + " for function: " + Name + Location, 2, true, true, "");
            return nullptr;
        }
        ArgTypes.push_back(ArgType);
    }

    llvm::FunctionType* FuncType = llvm::FunctionType::get(ReturnType, ArgTypes, false);
    llvm::Function* Function = llvm::Function::Create(FuncType, llvm::Function::ExternalLinkage, Name, Module);
    if (!Function) {
        Write("Function Generation", "Failed to create function: " + Name + Location, 2, true, true, "");
        return nullptr;
    }

    Methods[Name] = Function;

    Function->removeFnAttr(llvm::Attribute::AlwaysInline);
    Function->removeFnAttr(llvm::Attribute::InlineHint);
    Function->removeFnAttr(llvm::Attribute::NoInline);

    if (Node->alwaysInline) {
        Function->addFnAttr(llvm::Attribute::AlwaysInline);
    } else if (Node->isInlined) {
        Function->addFnAttr(llvm::Attribute::InlineHint);
    } else {
        Function->addFnAttr(llvm::Attribute::NoInline);
    }

    ScopeStack LocalScopes;
    PushScope(LocalScopes);

    llvm::BasicBlock* EntryBlock = llvm::BasicBlock::Create(ctx, "entry", Function);
    if (!EntryBlock) {
        Write("Function Generation", "Failed to create entry block for function: " + Name + Location, 2, true, true, "");
        return nullptr;
    }

    llvm::IRBuilder<> Builder(EntryBlock);

    auto ArgIter = Function->arg_begin();
    for (const auto& Arg : Node->params) {
        std::string paramName = std::get<0>(Arg);
        std::string paramType = std::get<1>(Arg);
        int dimensions = std::get<2>(Arg);
        
        ArgIter->setName(paramName);
        
        llvm::AllocaInst* Alloca = Builder.CreateAlloca(ArgIter->getType(), nullptr, paramName + ".addr");
        if (!Alloca) {
            Write("Function Generation", "Failed to create alloca for parameter: " + paramName + " in function: " + Name + Location, 2, true, true, "");
            return nullptr;
        }
        Builder.CreateStore(ArgIter, Alloca);
        LocalScopes.back()[paramName] = Alloca;
        
        ++ArgIter;
    }

    GenerateBlock(Node->body, Builder, LocalScopes, Methods);

    if (!EntryBlock->getTerminator()) {
        gc.GenerateScopeCleanup(Builder);
        
        if (ReturnType->isVoidTy()) {
            Builder.CreateRetVoid();
        } else if (ReturnType->isPointerTy()) {
            llvm::Value* DefaultValue = llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(ReturnType));
            Builder.CreateRet(DefaultValue);
        } else {
            llvm::Value* DefaultValue;
            if (ReturnType->isIntegerTy(1)) {
                DefaultValue = llvm::ConstantInt::get(ReturnType, false);
            } else if (ReturnType->isIntegerTy()) {
                DefaultValue = llvm::ConstantInt::get(ReturnType, 0);
            } else if (ReturnType->isFloatingPointTy()) {
                DefaultValue = llvm::ConstantFP::get(ReturnType, 0.0);
            } else {
                DefaultValue = llvm::UndefValue::get(ReturnType);
                Write("Function Generation", "Unsupported return type for default value in function: " + Name + Location, 1, true, true, "");
            }
            Builder.CreateRet(DefaultValue);
        }
    }

    return Function;
}