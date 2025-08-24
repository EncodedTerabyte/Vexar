#include "ColorPrint.hh"

#include <iostream>

void PrintRGB(const std::string& text, int r, int g, int b) {
    std::cout << "\033[38;2;" << r << ";" << g << ";" << b << "m"
              << text << "\033[0m";
}
