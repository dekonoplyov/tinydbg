#pragma once

#include <cstdint>
#include <termio.h>

namespace tinydbg {

class Breakpoint {
public:
    Breakpoint(pid_t pid, std::intptr_t addr)
        : pid{pid}
        , addr{addr}
        , enabled{false}
        , savedData{}
    {
    }

    void enable();
    void disable();
    bool isEnabled() const { return enabled; }
    std::intptr_t getAddress() const { return addr; }

private:
    pid_t pid;
    std::intptr_t addr;
    bool enabled;
    // data which used to be at the breakpoint address
    uint8_t savedData;
};

} // namespace tinydbg
