#pragma once
#include "../Miscellaneous/conf/FileAssociations.hh"
#include "../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../Token.hh"

#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <set>
#include <filesystem>
#include <set>

std::string FindFileWithExtension(const std::string& basePath);
std::vector<std::string> FindAllFilesInDirectory(const std::string& dirPath);
std::map<unsigned int, std::string> ProcessImports(const std::map<unsigned int, std::string>& FileContents, std::set<std::string>& importedFiles);
std::vector<Token> Tokenize(const std::map<unsigned int, std::string>& FileContents);