#include "debugger.h"

#include "linenoise.h"

#include <cstddef>
#include <iostream>
#include <sstream>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace tinydbg {

namespace {

bool isPreffix(const std::string& prefix, const std::string& s)
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

} // namespace

void Debugger::run()
{
    int waitStatus;
    auto options = 0;
    waitpid(pid, &waitStatus, options);

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

    if (isPreffix(command, "continue")) {
        continueExecution();
    } else {
        std::cerr << "Unknown command\n";
    }
}

void Debugger::continueExecution()
{
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);

    int waitStatus;
    auto options = 0;
    waitpid(pid, &waitStatus, options);
}

int debug(const std::string& programName)
{
    auto pid = fork();

    if (pid == 0) {
        // we're in the child process
        // execute debugee
        ptrace(PTRACE_TRACEME, pid, nullptr, nullptr);
        execl(programName.c_str(), programName.c_str(), nullptr);
    } else if (pid >= 1) {
        // we're in the parent process
        // execute debugger
        tinydbg::Debugger debugger{programName, pid};
        debugger.run();
    } else {
        std::cerr << "fork failed, pid: " << pid;
        return -1;
    }

    return 0;
}

} // namespace tinydbg
