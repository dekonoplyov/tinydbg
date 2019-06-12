#pragma once

#include "breakpoint.h"
#include "symbol.h"

#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"

#include <signal.h>
#include <string>
#include <unordered_map>

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
    void handleStepi();
    void handleSymbol(const std::vector<std::string>& args);
    void continueExecution();
    void printBacktrace();
    void readVariables();
    // address should be offset to process virtual memory
    void setBreakpoint(uint64_t address);
    void setBreakpointAtFunction(const std::string& name);
    void setBreakpointAtLine(const std::string& file, size_t line);
    std::vector<Symbol> lookupSymbol(const std::string& name);
    void removeBreakpoint(uint64_t address);
    void singleStepInstruction();
    void singleStepInstructionWithBpCheck();
    void stepIn();
    void stepOut();
    void stepOver();
    void stepOverBreakpoint();
    void waitForSignal();
    void handleSigtrap(siginfo_t siginfo);

    uint64_t readMemory(uint64_t address) const;
    void writeMemory(uint64_t address, uint64_t value);

    uint64_t getPC() const;
    void setPC(uint64_t pc);

    dwarf::die getFunction(uint64_t pc, bool addrOffsetted = true);
    dwarf::line_table::iterator getLineEntry(uint64_t pc, bool addrOffsetted = true);

    uint64_t getOffsettedAddress(uint64_t addr);
    uint64_t getSourceAddress(uint64_t offsettedAddress);

    void printSource(const std::string& fileName, size_t line, size_t linesContext = 2);

private:
    std::string programName;
    int pid;
    uint64_t memoryOffset;
    elf::elf elf;
    dwarf::dwarf dwarf;
    std::unordered_map<uint64_t, Breakpoint> breakpoints;
};

} // namespace tinydbg
