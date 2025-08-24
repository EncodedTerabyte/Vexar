#include "StringH.hh"

std::string str(std::string_view input) {
    return std::string(input);
}

std::string wstr(const std::wstring& winput) {
    if (winput.empty()) return {};

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, winput.data(),
                                          (int)winput.size(), nullptr, 0, nullptr, nullptr);

    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, winput.data(),
                        (int)winput.size(), result.data(), size_needed, nullptr, nullptr);

    return result;
}
