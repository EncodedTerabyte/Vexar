#pragma once
#include "../Miscellaneous/LoggerHandler/LoggerFile.hh"
#include "../Token.hh"

#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <set>

std::vector<Token> Tokenize(const std::map<unsigned int, std::string>& FileContents);