#pragma once

#include <string>

namespace tinydbg {

int debug(const std::string& programName);

class Debugger {
public:
    Debugger(std::string programName, int pid)
        : programName{std::move(programName)}
        , pid{pid}
    {
    }

    void run();
    void handleCommand(const std::string& line);
    void continueExecution();

private:
    std::string programName;
    int pid;
};

} // namespace tinydbg
