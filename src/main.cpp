#include "debugger.h"

#include <iostream>

int main(int argc, char* argv[])
{
    try {
        if (argc < 2) {
            std::cerr << "Program name not specified";
            return -1;
        }

        return tinydbg::debug(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "An error occured: " << e.what();
        return -1;
    } catch (...) {
        std::cerr << "Something went wrong";
        return -1;
    }
}
