#include "registers.h"

#include <sys/ptrace.h>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>

namespace tinydbg {

namespace {

auto findRegisterDescriptor(std::function<bool(const RegisterDescriptor&)> predicate)
{
    return std::find_if(
        REGISTOR_DESCRIPTORS.cbegin(),
        REGISTOR_DESCRIPTORS.cend(),
        predicate);
}

} // namespace

uint64_t getRegisterValue(pid_t pid, Register r)
{
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);

    const auto it = findRegisterDescriptor([r](const auto& rd) { return rd.reg == r; });
    // we can do that cause we have same layout of REGISTOR_DESCRIPTORS and user_regs_struct
    // same could be done with switch(Register)
    return *(reinterpret_cast<uint64_t*>(&regs + (it - REGISTOR_DESCRIPTORS.cbegin())));
}

uint64_t getRegisterValueFromDwarf(pid_t pid, int dwarfRegNum)
{
    const auto it = findRegisterDescriptor(
        [dwarfRegNum](const auto& rd) { return rd.dwarfReg == dwarfRegNum; });
    if (it == REGISTOR_DESCRIPTORS.cend()) {
        throw std::out_of_range{"Unknown dwarf register"};
    }
    return getRegisterValue(pid, it->reg);
}

void setRegisterValue(pid_t pid, Register r, uint64_t value)
{
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);

    const auto it = findRegisterDescriptor([r](const auto& rd) { return rd.reg == r; });
    *(reinterpret_cast<uint64_t*>(&regs + (it - REGISTOR_DESCRIPTORS.cbegin()))) = value;
    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
}

std::string getRegisterName(Register r)
{
    const auto it = findRegisterDescriptor([r](auto&& rd) { return rd.reg == r; });
    return it->name;
}

Register getRegister(const std::string& name)
{
    const auto it = findRegisterDescriptor([name](auto&& rd) { return rd.name == name; });
    return it->reg;
}

void dumpRegisters(pid_t pid)
{
    for (const auto& r : REGISTOR_DESCRIPTORS) {
        std::cerr << r.name << " 0x"
                  << std::setfill('0') << std::setw(16) << std::hex
                  << getRegisterValue(pid, r.reg) << std::endl;
    }
}

} // namespace tinydbg
