#include "FileSerializer.hh"

std::map<unsigned int, std::string> SerializeFile(fs::path InputFile) {
    std::map<unsigned int, std::string> lines;
    std::ifstream file(InputFile);
    if (!file.is_open()) return lines;

    std::string line;
    unsigned int lineNumber = 1;
    while (std::getline(file, line)) {
        lines[lineNumber++] = line;
    }

    return lines;
}