#pragma once

#include "breakpoint.h"

#include <string>
#include <unordered_map>
#include <vector>

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
    void handleBreakpoint(const std::vector<std::string>& args);
    void continueExecution();
    // address should be offset to process virtual memory
    void setBreakpoint(std::intptr_t address);

private:
    std::string programName;
    int pid;
    std::unordered_map<std::intptr_t, Breakpoint> breakpoints;
};

} // namespace tinydbg
