#include "debugger.h"

#include "registers.h"

#include "linenoise.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace tinydbg {

namespace {

bool isPrefix(const std::string& prefix, const std::string& s)
{
    if (prefix.size() > s.size()) {
        return false;
    }
    return std::equal(prefix.cbegin(), prefix.cend(), s.cbegin());
}

std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream stream{s};
    std::string token;

    while (std::getline(stream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

// ugly workaround to get offset from /proc/<pid>/maps
std::optional<uint64_t> getOffset(pid_t pid)
{
    // TODO get rid of this sleep
    // wait to change /proc/<pid>/maps after fork
    sleep(1);
    std::string filename = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream f{filename};
    if (f.good()) {
        std::string line;
        getline(f, line);
        // assume ADDRESS-ADDRESS ... format
        auto tokens = split(line, '-');
        if (tokens.size() < 2) {
            return {};
        }
        return std::stol(tokens[0], 0, 16);
    }

    return {};
}

// Parse string in OxADDRESS format
std::optional<uint64_t> parseAddress(const std::string& s)
{
    if (!isPrefix("0x", s)) {
        return {};
    }

    std::string addr{s, 2};
    return std::stol(addr, 0, 16);
}

siginfo_t getSigInfo(pid_t pid)
{
    siginfo_t info;
    ptrace(PTRACE_GETSIGINFO, pid, nullptr, &info);
    return info;
}

} // namespace

Debugger::Debugger(std::string programName, int pid)
    : programName{std::move(programName)}
    , pid{pid}
    , memoryOffset{0}
{
    auto offset = getOffset(pid);
    if (offset) {
        memoryOffset = *offset;
    } else {
        std::cerr << "Failed to get proc memory offset\n";
    }

    auto fd = open(this->programName.c_str(), O_RDONLY);

    elf = elf::elf{elf::create_mmap_loader(fd)};
    dwarf = dwarf::dwarf{dwarf::elf::create_loader(elf)};
}

void Debugger::run()
{
    waitForSignal();

    char* line = linenoise("tinydbg> ");
    while (line != nullptr) {
        handleCommand(line);
        linenoiseHistoryAdd(line);
        linenoiseFree(line);
        line = linenoise("tinydbg> ");
    }
}

void Debugger::handleCommand(const std::string& line)
{
    auto args = split(line, ' ');
    const auto& command = args[0];

    if (isPrefix(command, "continue")) {
        continueExecution();
    } else if (isPrefix(command, "breakpoint")) {
        handleBreakpoint(args);
    } else if (isPrefix(command, "register")) {
        handleRegister(args);
    } else if (isPrefix(command, "step")) {
        stepIn();
    } else if (isPrefix(command, "next")) {
        stepOver();
    } else if (isPrefix(command, "finish")) {
        stepOut();
    } else if (isPrefix(command, "stepi")) {
        handleStepi();
    } else {
        std::cerr << "Unknown command\n";
    }
}

void Debugger::handleBreakpoint(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        std::cerr << "Insufficient num of args to set breakpoint\n";
        return;
    }

    auto address = parseAddress(args[1]);
    if (!address) {
        std::cerr << "Failed to parse address, expected format: 0xADDRESS\n";
        return;
    }
    auto offsettedAddress = getOffsettedAddress(*address);
    setBreakpoint(offsettedAddress);
}

void Debugger::handleRegister(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        std::cerr << "Insufficient num of args to register command\n";
        return;
    }

    if (isPrefix(args[1], "dump")) {
        dumpRegisters(pid);
        return;
    }

    if (args.size() < 3) {
        std::cerr << "Insufficient num of args to read register\n";
        return;
    }

    const auto reg = getRegister(args[2]);
    if (!reg) {
        std::cerr << "Unknown register: '" << args[2] << "'\n";
        return;
    }

    if (isPrefix(args[1], "read")) {
        std::cerr << "0x" << std::hex << getRegisterValue(pid, *reg) << std::endl;
    } else if (isPrefix(args[1], "write")) {
        if (args.size() < 4) {
            std::cerr << "Insufficient num of args to write register\n";
            return;
        }

        const auto address = parseAddress(args[3]);
        if (!address) {
            std::cerr << "Failed to parse address, expected format: 0xADDRESS\n";
            return;
        }

        setRegisterValue(pid, *reg, *address);
    } else {
        std::cerr << "Unknown register command: '" << args[1] << "'\n";
    }
}

void Debugger::handleMemory(const std::vector<std::string>& args)
{
    if (args.size() < 3) {
        std::cerr << "Insufficient num of args to work with memory\n";
        return;
    }

    const auto address = parseAddress(args[2]);
    if (!address) {
        std::cerr << "Failed to parse address, expected format: 0xADDRESS\n";
        return;
    }

    if (isPrefix(args[1], "read")) {
        std::cerr << std::hex << readMemory(*address) << std::endl;
    } else if (isPrefix(args[1], "write")) {
        if (args.size() < 4) {
            std::cerr << "Insufficient num of args to write memory\n";
            return;
        }
        const auto value = parseAddress(args[3]);
        if (!value) {
            std::cerr << "Failed to parse address, expected format: 0xADDRESS\n";
            return;
        }

        writeMemory(*address, *value);
    }
}

void Debugger::handleStepi()
{
    singleStepInstructionWithBpCheck();
    const auto line = getLineEntry(getPC());
    printSource(line->file->path, line->line);
}

void Debugger::continueExecution()
{
    stepOverBreakpoint();
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    waitForSignal();
}

void Debugger::setBreakpoint(uint64_t address)
{
    std::cerr << "Set breakpoint at address 0x" << std::hex << address << std::endl;
    Breakpoint breakpoint{pid, address};
    breakpoint.enable();
    breakpoints.insert({address, breakpoint});
}

void Debugger::removeBreakpoint(uint64_t address)
{
    auto& breakpoint = breakpoints.at(address);
    if (breakpoint.isEnabled()) {
        breakpoint.disable();
    }
    breakpoints.erase(address);
}

void Debugger::singleStepInstruction()
{
    ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
    waitForSignal();
}
void Debugger::singleStepInstructionWithBpCheck()
{
    if (breakpoints.count(getPC()) > 0) {
        stepOverBreakpoint();
    } else {
        singleStepInstruction();
    }
}

void Debugger::stepIn()
{
    const auto line = getLineEntry(getPC())->line;

    // single instruction step until get to new line
    while (getLineEntry(getPC())->line == line) {
        singleStepInstructionWithBpCheck();
    }

    const auto line_entry = getLineEntry(getPC());
    printSource(line_entry->file->path, line_entry->line);
}

void Debugger::stepOut()
{
    const auto framePointer = getRegisterValue(pid, Register::rbp);
    const auto returnAddress = readMemory(framePointer + 8);

    bool shouldRemoveBreakpoint = false;
    if (breakpoints.count(returnAddress) == 0) {
        setBreakpoint(returnAddress);
        shouldRemoveBreakpoint = true;
    }

    continueExecution();

    if (shouldRemoveBreakpoint) {
        removeBreakpoint(returnAddress);
    }
}

void Debugger::stepOver()
{
    // to deal with loops, ifs and jumps
    // add breakpoint on every line in current function
    // because we don't know which line'll be executed next
    const auto function = getFunction(getPC());
    const auto functionEntry = at_low_pc(function);
    const auto functionEnd = at_high_pc(function);

    // TODO fix this memoryOffset addition
    auto line = getLineEntry(functionEntry + memoryOffset);
    const auto startLine = getLineEntry(getPC());

    std::vector<uint64_t> toDelete;
    while (line->address < functionEnd) {
        if (line->address != startLine->address && breakpoints.count(line->address) == 0) {
            setBreakpoint(line->address);
            toDelete.push_back(line->address);
        }
        ++line;
    }

    const auto framePointer = getRegisterValue(pid, Register::rbp);
    const auto returnAddress = readMemory(framePointer + 8);
    if (breakpoints.count(returnAddress) == 0) {
        setBreakpoint(returnAddress);
        toDelete.push_back(returnAddress);
    }

    continueExecution();

    for (const auto address : toDelete) {
        removeBreakpoint(address);
    }
}

void Debugger::stepOverBreakpoint()
{
    if (breakpoints.count(getPC()) > 0) {
        auto& breakpoint = breakpoints.at(getPC());
        if (breakpoint.isEnabled()) {
            breakpoint.disable();
            ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
            waitForSignal();
            breakpoint.enable();
        }
    }
}

void Debugger::waitForSignal()
{
    int waitStatus;
    auto options = 0;
    waitpid(pid, &waitStatus, options);

    auto siginfo = getSigInfo(pid);
    switch (siginfo.si_signo) {
    case SIGTRAP:
        handleSigtrap(siginfo);
        break;
    case SIGSEGV:
        std::cerr << "Segfault, reason: " << siginfo.si_code << std::endl;
        break;
    default:
        std::cerr << "Got signal: " << strsignal(siginfo.si_signo) << std::endl;
        break;
    }
}

void Debugger::handleSigtrap(siginfo_t siginfo)
{
    switch (siginfo.si_code) {
    case SI_KERNEL:
    case TRAP_BRKPT: {
        // put pc back where it should be
        setPC(getPC() - 1);
        std::cerr << "Hit breakpoint at address 0x" << std::hex << getPC() << std::endl;
        const auto lineEntry = getLineEntry(getPC());
        printSource(lineEntry->file->path, lineEntry->line);
        return;
    }
    case TRAP_TRACE:
        return;
    default:
        std::cerr << "Unknown SIGTRAP code: " << siginfo.si_code << std::endl;
        return;
    }
}

uint64_t Debugger::readMemory(uint64_t address) const
{
    return ptrace(PTRACE_PEEKDATA, pid, address, nullptr);
}

void Debugger::writeMemory(uint64_t address, uint64_t value)
{
    ptrace(PTRACE_POKEDATA, pid, address, value);
}

uint64_t Debugger::getPC() const
{
    return getRegisterValue(pid, Register::rip);
}

void Debugger::setPC(uint64_t pc)
{
    setRegisterValue(pid, Register::rip, pc);
}

dwarf::die Debugger::getFunction(uint64_t pc)
{
    pc -= memoryOffset;
    for (auto& cu : dwarf.compilation_units()) {
        if (die_pc_range(cu.root()).contains(pc)) {
            for (const auto& die : cu.root()) {
                if (die.tag == dwarf::DW_TAG::subprogram) {
                    if (die_pc_range(cu.root()).contains(pc)) {
                        return die;
                    }
                }
            }
        }
    }

    throw std::out_of_range{"Cannot find function"};
}

dwarf::line_table::iterator Debugger::getLineEntry(uint64_t pc)
{
    pc -= memoryOffset;
    for (auto& cu : dwarf.compilation_units()) {
        if (die_pc_range(cu.root()).contains(pc)) {
            auto& lineTable = cu.get_line_table();
            auto it = lineTable.find_address(pc);
            if (it == lineTable.end()) {
                throw std::out_of_range{"Cannot find line entry"};
            } else {
                return it;
            }
        }
    }

    throw std::out_of_range{"Cannot find line entry"};
}

uint64_t Debugger::getOffsettedAddress(uint64_t addr)
{
    return memoryOffset + addr;
}

void Debugger::printSource(const std::string& fileName, size_t line, size_t linesContext)
{
    std::ifstream file{fileName};

    // Work out a window around the desired line
    auto startLine = line <= linesContext ? 1 : line - linesContext;
    auto endLine = line + linesContext + (line < linesContext ? linesContext - line : 0) + 1;

    char c{};
    size_t currentLine = 1;
    while (currentLine != startLine && file.get(c)) {
        if (c == '\n') {
            ++currentLine;
        }
    }

    // Output cursor if we're at the current line
    std::cerr << (currentLine == line ? "> " : "  ");

    // Write lines up until end_line
    while (currentLine <= endLine && file.get(c)) {
        std::cerr << c;
        if (c == '\n') {
            ++currentLine;
            // Output cursor if we're at the current line
            std::cerr << (currentLine == line ? "> " : "  ");
        }
    }

    std::cerr << std::endl;
}

int debug(const std::string& programName)
{
    auto pid = fork();

    if (pid == 0) {
        // we're in the child process
        // execute debugee
        std::cerr << "child pid: " << getpid() << std::endl;
        ptrace(PTRACE_TRACEME, pid, nullptr, nullptr);
        execl(programName.c_str(), programName.c_str(), nullptr);
    } else if (pid >= 1) {
        // we're in the parent process
        // execute debugger
        tinydbg::Debugger debugger{programName, pid};
        debugger.run();
    } else {
        std::cerr << "fork failed, pid: " << pid << std::endl;
        return -1;
    }

    return 0;
}

} // namespace tinydbg
