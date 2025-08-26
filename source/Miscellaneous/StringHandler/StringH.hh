#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <string>
#include <string_view>
#include <windows.h>

std::string str(std::string_view input);

std::string wstr(const std::wstring& winput);
