#include "LLVMHeader.hh"

inline void runExecutable(const fs::path& OutputDir, const std::string& OutputName, const std::string& SourceFile) {
    fs::path TempExe;
#ifdef _WIN32
    std::string TempDir = std::getenv("TEMP") ? std::getenv("TEMP") : "C:\\Windows\\Temp";
    TempExe = fs::path(TempDir) / (OutputName + "_temp.exe");
   
    std::string TempCommand = "clang -w -o \"" + TempExe.string() + "\" \"" + SourceFile + "\"";
    int TempResult = std::system(TempCommand.c_str());
   
    if (TempResult == 0) {
       std::string RunCommand = "\"" + TempExe.string() + "\"";
       std::system(RunCommand.c_str());
       std::filesystem::remove(TempExe);
    }
#else
   std::string TempDir = std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp";
   TempExe = fs::path(TempDir) / (OutputName + "_temp");
   
   std::string TempCommand = "clang -w -o \"" + TempExe.string() + "\" \"" + SourceFile + "\"";
   int TempResult = std::system(TempCommand.c_str());
   
   if (TempResult == 0) {
       Write("Code Generation", "Running executable...", 3, true, true, "");
       std::string RunCommand = "\"" + TempExe.string() + "\"";
       std::system(RunCommand.c_str());
       std::filesystem::remove(TempExe);
   } else {
       Write("Code Generation", "Failed to create temporary executable for running", 2, true, true, "");
   }
#endif
}

void CreatePlatformBinary(std::unique_ptr<llvm::Module> Module, std::string Triple, bool RunAfterCompile, int OptLevel, fs::path Output);