#include "FunctionGenerator.hh"
#include "BlockGenerator.hh"
#include "../Helper/Types.hh"
#include "../Helper/Mapping.hh"

llvm::Function* GenerateFunction(FunctionNode* Node, llvm::Module* Module, AllocaSymbols& GlobalAllocas, FunctionSymbols& Methods) {
    llvm::LLVMContext& ctx = Module->getContext();

    std::string Name = Node->name;
    std::string ReturnTypeStr = Node->returnType;
    llvm::Type* ReturnType = GetLLVMTypeFromString(ReturnTypeStr, ctx);

    if (!ReturnType) {
        std::cerr << "Unknown return type: " << ReturnTypeStr << std::endl;
        exit(0);
    }

    std::vector<llvm::Type*> ArgTypes;
    for (const auto& Arg : Node->params) {
        llvm::Type* ArgType = GetLLVMTypeFromString(Arg.second, ctx);
        if (!ArgType) {
            std::cerr << "Unknown argument type: " << Arg.second << std::endl;
            exit(0);
        }
        ArgTypes.push_back(ArgType);
    }

    llvm::FunctionType* FuncType = llvm::FunctionType::get(ReturnType, ArgTypes, false);
    llvm::Function* Function = llvm::Function::Create(FuncType, llvm::Function::ExternalLinkage, Name, Module);

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
    llvm::IRBuilder<> Builder(EntryBlock);

    auto ArgIter = Function->arg_begin();
    for (const auto& Arg : Node->params) {
        ArgIter->setName(Arg.first);
        
        llvm::AllocaInst* Alloca = Builder.CreateAlloca(ArgIter->getType(), nullptr, Arg.first + ".addr");
        Builder.CreateStore(ArgIter, Alloca);
        
        LocalScopes.back()[Arg.first] = Alloca;
        ++ArgIter;
    }

    GenerateBlock(Node->body, Builder, LocalScopes, Methods);

    if (!EntryBlock->getTerminator()) {
        if (ReturnType->isVoidTy()) {
            Builder.CreateRetVoid();
        } else {
            llvm::Value* DefaultValue;
            if (ReturnType->isFloatingPointTy()) {
                DefaultValue = llvm::ConstantFP::get(ReturnType, 0.0);
            } else if (ReturnType->isIntegerTy()) {
                DefaultValue = llvm::ConstantInt::get(ReturnType, 0);
            } else if (ReturnType->isPointerTy()) {
                DefaultValue = llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(ReturnType));
            } else {
                DefaultValue = llvm::UndefValue::get(ReturnType);
            }
            Builder.CreateRet(DefaultValue);
        }
    }

    return Function;
}