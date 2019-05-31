#pragma once

#include <cstdint>
#include <termio.h>

namespace tinydbg {

class Breakpoint {
public:
    Breakpoint(pid_t pid, uint64_t addr)
        : pid{pid}
        , addr{addr}
        , enabled{false}
        , savedData{}
    {
    }

    void enable();
    void disable();
    bool isEnabled() const { return enabled; }
    uint64_t getAddress() const { return addr; }

private:
    pid_t pid;
    uint64_t addr;
    bool enabled;
    // data which used to be at the breakpoint address
    uint8_t savedData;
};

} // namespace tinydbg
