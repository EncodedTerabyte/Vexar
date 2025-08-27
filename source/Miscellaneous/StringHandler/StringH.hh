#include <string>
#include <string_view>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <windows.h>
#endif

std::string str(std::string_view input);

#ifdef _WIN32
std::string wstr(const std::wstring& winput);
#endif