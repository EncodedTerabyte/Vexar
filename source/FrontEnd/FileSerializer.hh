#pragma once

#include "../Miscellaneous/LoggerHandler/LoggerFile.hh"

#include <filesystem>
#include <string>
#include <vector>
#include <map>

namespace fs = std::filesystem;

std::map<unsigned int, std::string> SerializeFile(fs::path InputFile);