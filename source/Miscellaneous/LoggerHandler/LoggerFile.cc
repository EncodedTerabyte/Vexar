#include "LoggerFile.hh"

std::string EvalLevel(int lvl) {
    switch(lvl) {
        case 0:
            return "Info";
        case 1:
            return "Warning";
        case 2:
            return "Error";
        case 3: 
            return "Success";
        default:
            return "Unknown Level";
    }
}

fs::path GetExecutableDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
}

fs::path CacheLLoggerFile() {
    fs::path exeDir = GetExecutableDir();
    fs::path loggerPath = exeDir / "vexar.log";

    if (!fs::exists(loggerPath)) {
        std::ofstream loggerFile(loggerPath);
    }

    return loggerPath;
}

// 0 = log
// 1 = warm
// 2 = error
// 3 = success
void Write(const std::string& Caption, const std::string& Info, int Level, bool DisplayConsole, bool ShowTime, const std::string& ExtraInfo) {

    Level = std::clamp(Level, 0, 3);

    fs::path loggerPath = CacheLLoggerFile();
    std::ofstream loggerFile(loggerPath, std::ios::app);

    if (!loggerFile.is_open()) return;

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&now_time);

    std::ostringstream oss;
    oss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");
    
    std::string CurrentTime = oss.str();
    std::string LoggerLevel = EvalLevel(Level);
    std::string line = ShowTime 
        ? CurrentTime + " | [" + Caption + "] [" + LoggerLevel + "] " + Info + ExtraInfo
        : "[" + Caption + "] [" + LoggerLevel + "] " + Info + ExtraInfo;

    loggerFile << line << std::endl;

    if (DisplayConsole) {
        switch (Level) {
            case 0: PrintRGB(line, 189, 189, 189); break;
            case 1: PrintRGB(line, 255, 112, 67); break;
            case 2: PrintRGB(line, 239, 83, 80); break;
            case 3: PrintRGB(line, 102, 187, 106); break;
        }
        std::cout << std::endl;
    }

    if (Level == 2) {
        loggerFile << "Safely exiting" << std::endl;
        PrintRGB("Safely exiting", 239, 83, 80);
        std::cout << std::endl;
        exit(1);
    }
}
