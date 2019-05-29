#include "debugger.h"

#include "registers.h"

#include "linenoise.h"

#include <cstddef>
#include <iostream>
#include <optional>
#include <sstream>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

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

// Parse string in OxADDRESS format
std::optional<std::intptr_t> parseAddress(const std::string& s)
{
    if (!isPrefix("0x", s)) {
        return {};
    }

    std::string addr{s, 2};
    return std::stol(addr, 0, 16);
}

} // namespace

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

    setBreakpoint(*address);
}

void Debugger::handleRegister(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        std::cerr << "Insufficient num of args to register command\n";
        return;
    }

    if (isPrefix(args[1], "dump")) {
        dumpRegisters(pid);
    } else if (isPrefix(args[1], "read")) {
        if (args.size() < 3) {
            std::cerr << "Insufficient num of args to read register\n";
            return;
        }

        const auto reg = getRegister(args[2]);
        if (!reg) {
            std::cerr << "Unknown register: '" << args[2] << "'\n";
            return;
        }

        std::cerr << getRegisterValue(pid, *reg) << std::endl;
    } else if (isPrefix(args[1], "write")) {
        if (args.size() < 4) {
            std::cerr << "Insufficient num of args to write register\n";
            return;
        }

        const auto reg = getRegister(args[2]);
        if (!reg) {
            std::cerr << "Unknown register: '" << args[2] << "'\n";
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
        std::cerr << "todo\n";
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
            std::cerr << "tododo\n";
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

void Debugger::continueExecution()
{
    stepOverBreakpoint();
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    waitForSignal();
}

void Debugger::setBreakpoint(std::intptr_t address)
{
    std::cerr << "Set breakpoint at address 0x" << std::hex << address << std::endl;
    Breakpoint breakpoint{pid, address};
    breakpoint.enable();
    breakpoints.insert({address, breakpoint});
}

void Debugger::stepOverBreakpoint() {
    // -1 because execution will go past the breakpoint
    const auto possibleBpLocation = getPC() - 1;
    if (breakpoints.count(possibleBpLocation) > 0) {
        auto& breakpoint = breakpoints.at(possibleBpLocation);
        if (breakpoint.isEnabled()) {
            const auto previousInstructionAddress = possibleBpLocation;
            setPC(previousInstructionAddress);
            breakpoint.disable();
            ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
            waitForSignal();
            breakpoint.enable();
        }
    }
}

void Debugger::waitForSignal() {
    int waitStatus;
    auto options = 0;
    waitpid(pid, &waitStatus, options);
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
