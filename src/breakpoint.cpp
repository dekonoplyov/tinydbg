#include "breakpoint.h"

#include <iostream>
#include <sys/ptrace.h>

namespace tinydbg {

void Breakpoint::enable()
{
    const auto data = ptrace(PTRACE_PEEKDATA, pid, addr, nullptr);
    // save bottom byte
    savedData = static_cast<uint8_t>(data & 0xFF);

    // set bottom byte to 0xСС (int3)
    // ~ bitwise NOT ~01001 == 10110
    const uint64_t int3 = 0xCC;
    const uint64_t dataWithInt3 = ((data & ~0xFF) | int3);
    ptrace(PTRACE_POKEDATA, pid, addr, dataWithInt3);
    enabled = true;
}

void Breakpoint::disable()
{
    const auto data = ptrace(PTRACE_PEEKDATA, pid, addr, nullptr);
    const uint64_t restoredData = ((data & ~0xFF) | savedData);
    ptrace(PTRACE_POKEDATA, pid, addr, restoredData);
    enabled = false;
}

} // namespace tinydbg
