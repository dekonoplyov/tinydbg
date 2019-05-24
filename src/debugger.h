#pragma once

#include <string>

namespace tinydbg {

class Debugger {
public:
    Debugger(std::string programName, int pid)
        : programName{std::move(programName)}
        , pid{pid}
    {
    }

    void run();

private:
    std::string programName;
    int pid;
};

} // namespace tinydbg
