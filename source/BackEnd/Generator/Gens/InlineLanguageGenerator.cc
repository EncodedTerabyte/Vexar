#include "InlineLanguageGenerator.hh"
#include <regex>
#include <fstream>
#include <cstdlib>
#include <random>

std::vector<llvm::Value*> ExtractVariableReferences(const std::string& code, AeroIR* IR) {
    std::vector<llvm::Value*> inputs;
    std::regex varPattern(R"(\$([a-zA-Z_][a-zA-Z0-9_]*)\b)");
    std::smatch match;
    std::string::const_iterator searchStart(code.cbegin());
    
    while (std::regex_search(searchStart, code.cend(), match, varPattern)) {
        std::string varName = match[1].str();
        llvm::Value* var = IR->getVar(varName);
        if (var) {
            llvm::Value* loaded = IR->load(var);
            inputs.push_back(loaded);
        }
        searchStart = match.suffix().first;
    }
    
    return inputs;
}

std::string ProcessInlineCode(const std::string& code, const std::vector<llvm::Value*>& inputs) {
    std::string processed = code;
    
    std::regex stringPattern(R"(\$"([^"]*)");
    processed = std::regex_replace(processed, stringPattern, R"($"$1")");

    std::regex varPattern(R"(\$([a-zA-Z_][a-zA-Z0-9_]*)\b)");
    int inputIndex = 0;
    while (std::regex_search(processed, varPattern) && inputIndex < inputs.size()) {
        std::string replacement = "arg" + std::to_string(inputIndex);
        processed = std::regex_replace(processed, varPattern, replacement, std::regex_constants::format_first_only);
        inputIndex++;
    }
    
    return processed;
}

std::string GenerateRandomString(int length = 8) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 35);
    
    std::string result;
    for (int i = 0; i < length; ++i) {
        int val = dis(gen);
        if (val < 10) result += char('0' + val);
        else result += char('a' + val - 10);
    }
    return result;
}

std::string DetermineReturnType(const std::string& code) {
    std::regex returnPattern(R"(return\s+(.+?);)");
    std::smatch match;
    
    if (std::regex_search(code, match, returnPattern)) {
        std::string returnExpr = match[1].str();
        
        if (returnExpr.find('"') != std::string::npos) {
            return "const char*";
        }
        
        if (returnExpr.find('\'') != std::string::npos) {
            return "char";
        }
        
        if (returnExpr.find('.') != std::string::npos || 
            returnExpr.find('f') == returnExpr.length() - 1) {
            if (returnExpr.find('f') == returnExpr.length() - 1) {
                return "float";
            }
            return "double";
        }
        
        std::regex intPattern(R"(^\s*[+-]?\d+\s*$)");
        if (std::regex_match(returnExpr, intPattern)) {
            return "int32_t";
        }
        
        std::regex boolPattern(R"(^\s*(true|false)\s*$)");
        if (std::regex_match(returnExpr, boolPattern)) {
            return "bool";
        }
        
        return "int32_t";
    }
    
    return "void";
}

// next support returning custom types, custom c/c++ types should map nicely to a vexar struct / class
llvm::Value* CompileWithClang(const std::string& code, const std::string& lang, AeroIR* IR, const std::vector<llvm::Value*>& inputs) {
    #ifdef _WIN32
        std::string tempDir = std::getenv("TEMP") ? std::getenv("TEMP") : ".";
        std::string tempFile = tempDir + "\\inline_" + GenerateRandomString() + (lang == "c" ? ".c" : ".cpp");
    #else
        std::string tempFile = "/tmp/inline_" + GenerateRandomString() + (lang == "c" ? ".c" : ".cpp");
    #endif
    std::string bcFile = tempFile + ".bc";
    
    bool hasReturn = code.find("return") != std::string::npos;
    std::string returnType = hasReturn ? DetermineReturnType(code) : "void";
    
    std::string wrappedCode;
    if (lang == "c") {
        wrappedCode = "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <stdbool.h>\n#inlcude <windows.h>\n";
    } else {
        wrappedCode = "#include <cstdint>\n#include <iostream>\n#include <windows.h>\n extern \"C\" {\n";
    }
    
    std::string funcName = "inline_function_" + GenerateRandomString(4);
    wrappedCode += returnType + " " + funcName + "(";
    
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (i > 0) wrappedCode += ", ";
        llvm::Type* type = inputs[i]->getType();
        if (type->isIntegerTy(8)) wrappedCode += "char arg" + std::to_string(i);
        else if (type->isIntegerTy(16)) wrappedCode += "int32_t arg" + std::to_string(i);
        else if (type->isIntegerTy(32)) wrappedCode += "int32_t arg" + std::to_string(i);
        else if (type->isIntegerTy(64)) wrappedCode += "int32_t arg" + std::to_string(i);
        else if (type->isIntegerTy(1)) wrappedCode += "bool arg" + std::to_string(i);
        else if (type->isFloatTy()) wrappedCode += "float arg" + std::to_string(i);
        else if (type->isDoubleTy()) wrappedCode += "double arg" + std::to_string(i);
        else if (type->isPointerTy()) wrappedCode += "char* arg" + std::to_string(i);
        else wrappedCode += "void* arg" + std::to_string(i);
    }
    wrappedCode += ") {\n";
    
    std::string processedCode = code;
    std::regex varPattern(R"(\$([a-zA-Z_][a-zA-Z0-9_]*)\b)");
    int argIndex = 0;
    while (std::regex_search(processedCode, varPattern) && argIndex < inputs.size()) {
        std::string replacement = "arg" + std::to_string(argIndex++);
        processedCode = std::regex_replace(processedCode, varPattern, replacement, 
                                         std::regex_constants::format_first_only);
    }
    
    wrappedCode += processedCode;
    if (!hasReturn) {
        wrappedCode += "\n";
    }
    wrappedCode += "\n}";
    
    if (lang == "cxx") {
        wrappedCode += "\n}";
    }
    
    std::ofstream file(tempFile);
    file << wrappedCode;
    file.close();
    
    std::string compiler = (lang == "c") ? "clang" : "clang++";
    std::string cmd = compiler + " -emit-llvm -c -O2 \"" + tempFile + "\" -o \"" + bcFile + "\"";
    
    if (system(cmd.c_str()) != 0) {
        std::remove(tempFile.c_str());
        return nullptr;
    }
    
    llvm::SMDiagnostic error;
    std::unique_ptr<llvm::Module> inlineModule = 
        llvm::parseIRFile(bcFile, error, *IR->getContext());
    
    if (!inlineModule) {
        std::remove(tempFile.c_str());
        std::remove(bcFile.c_str());
        return nullptr;
    }
    
    std::string actualFuncName;
    for (auto& func : *inlineModule) {
        if (func.getName().str().find("inline_function_") == 0) {
            actualFuncName = func.getName().str();
            break;
        }
    }
    
    llvm::Linker linker(*IR->getModule());
    if (linker.linkInModule(std::move(inlineModule))) {
        std::remove(tempFile.c_str());
        std::remove(bcFile.c_str());
        return nullptr;
    }
    
    std::remove(tempFile.c_str());
    std::remove(bcFile.c_str());
    
    llvm::Function* inlineFunc = IR->getModule()->getFunction(actualFuncName);
    if (inlineFunc) {
        llvm::Value* result = IR->call(inlineFunc, inputs);
        if (hasReturn && result) {
            llvm::Type* resultType = result->getType();
            if (resultType->isFloatTy()) {
                return result;
            } else if (resultType->isDoubleTy()) {
                return result;
            } else if (resultType->isPointerTy()) {
                return result;
            } else if (resultType->isIntegerTy(1)) {
                return IR->intCast(result, IR->i32());
            } else if (resultType->isIntegerTy(8)) {
                return IR->intCast(result, IR->i32());
            } else if (resultType->isIntegerTy(16)) {
                return IR->intCast(result, IR->i32());
            } else if (resultType->isIntegerTy(64)) {
                return IR->intCast(result, IR->i32());
            } else {
                return result;
            }
        }
        return hasReturn ? result : IR->constI32(0);
    }
    
    return nullptr;
}

llvm::Value* GenerateInlineLanguage(InlineCodeNode* ICN, AeroIR* IR, FunctionSymbols& Methods) {
    if (ICN->lang == "assembly" || ICN->lang == "asm") {
        std::vector<llvm::Value*> varPtrs;
        std::regex varPattern(R"(\$([a-zA-Z_][a-zA-Z0-9_]*)\b)");
        std::smatch match;
        std::string::const_iterator searchStart(ICN->raw_code.cbegin());
        
        while (std::regex_search(searchStart, ICN->raw_code.cend(), match, varPattern)) {
            std::string varName = match[1].str();
            llvm::Value* var = IR->getVar(varName);
            if (var) {
                varPtrs.push_back(var);
            }
            searchStart = match.suffix().first;
        }
        
        std::vector<llvm::Value*> inputs;
        for (auto* varPtr : varPtrs) {
            inputs.push_back(IR->load(varPtr));
        }
        
        std::string processedCode = ProcessInlineCode(ICN->raw_code, inputs);
        
        std::vector<llvm::Type*> inputTypes;
        for (auto* input : inputs) {
            inputTypes.push_back(input->getType());
        }
        
        std::string constraints = "";
        for (size_t i = 0; i < inputs.size(); ++i) {
            if (i > 0) constraints += ",";
            constraints += "r";
        }
        
        llvm::FunctionType* asmType = llvm::FunctionType::get(IR->void_t(), inputTypes, false);
        llvm::InlineAsm* inlineAsm = llvm::InlineAsm::get(asmType, processedCode, constraints, true, ICN->IsVolatile);
        
        return IR->getBuilder()->CreateCall(inlineAsm, inputs);
    }

    if (ICN->IsVolatile) {
    }
    
    if (ICN->lang == "c") {
        std::vector<llvm::Value*> inputs = ExtractVariableReferences(ICN->raw_code, IR);
        return CompileWithClang(ICN->raw_code, "c", IR, inputs);
    }
    
    if (ICN->lang == "cxx") {
        std::vector<llvm::Value*> inputs = ExtractVariableReferences(ICN->raw_code, IR);
        return CompileWithClang(ICN->raw_code, "cxx", IR, inputs);
    }
    
    return nullptr;
}