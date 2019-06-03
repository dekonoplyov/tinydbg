#pragma once

#include "breakpoint.h"

#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"

#include <string>
#include <unordered_map>
#include <vector>

namespace tinydbg {

int debug(const std::string& programName);

class Debugger {
public:
    Debugger(std::string programName, int pid);

    void run();
    void handleCommand(const std::string& line);
    void handleBreakpoint(const std::vector<std::string>& args);
    void handleRegister(const std::vector<std::string>& args);
    void handleMemory(const std::vector<std::string>& args);
    void continueExecution();
    // address should be offset to process virtual memory
    void setBreakpoint(uint64_t address);
    void stepOverBreakpoint();
    void waitForSignal();

    uint64_t readMemory(uint64_t address) const;
    void writeMemory(uint64_t address, uint64_t value);

    uint64_t getPC() const;
    void setPC(uint64_t pc);

    dwarf::die getFunction(uint64_t pc);
    dwarf::line_table::iterator getLineEntry(uint64_t pc);

    uint64_t getOffsettedAddress(uint64_t addr);

    void printSource(const std::string& fileName, size_t line, size_t linesContext);

private:
    std::string programName;
    int pid;
    uint64_t memoryOffset;
    elf::elf elf;
    dwarf::dwarf dwarf;
    std::unordered_map<uint64_t, Breakpoint> breakpoints;
};

} // namespace tinydbg
