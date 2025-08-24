#pragma once

#include "../StringHandler/StringH.hh"

#include "ColorPrint.hh"

#include <windows.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

std::string EvalLevel(int lvl);
fs::path GetExecutableDir();
fs::path CacheLLoggerFile();

void Write(const std::string& Caption, const std::string& Info, int Level = 0, bool DisplayConsole = false, bool ShowTime = false, const std::string& ExtraInfo = "");