#include "debugger.h"

#include <cstddef>
#include <iostream>
#include <sys/ptrace.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Program name not specified";
        return -1;
    }

    auto prog = argv[1];

    auto pid = fork();

    if (pid == 0) {
        // we're in the child process
        // execute debugee
        ptrace(PTRACE_TRACEME, pid, nullptr, nullptr);
        execl(prog, prog, nullptr);
    } else if (pid >= 1) {
        // we're in the parent process
        // execute debugger
        tinydbg::Debugger debugger{prog, pid};
        debugger.run();
    } else {
        std::cerr << "fork failed, pid: " << pid;
        return -1;
    }

    return 0;
}
