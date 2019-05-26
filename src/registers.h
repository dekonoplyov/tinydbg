#pragma once

#include <sys/user.h>

#include <array>

namespace tinydbg {

enum class Register {
    rax, rbx, rcx, rdx, //
    rdi, rsi, rbp, rsp,
    r8,  r9,  r10, r11,
    r12, r13, r14, r15,
    rip, rflags,    cs,
    orig_rax, fs_base,
    gs_base,
    fs, gs, ss, ds, es,
};

static constexpr std::size_t REGISTER_NUMBER = 27;

struct RegisterDescriptor {
    Register reg;
    int dwarfReg;
    std::string name;
};

// same layout like in sys/user.h
static const std::array<RegisterDescriptor, REGISTER_NUMBER> REGISTOR_DESCRIPTORS{{
    {Register::r15, 15, "r15"},
    {Register::r14, 14, "r14"},
    {Register::r13, 13, "r13"},
    {Register::r12, 12, "r12"},
    {Register::rbp, 6, "rbp"},
    {Register::rbx, 3, "rbx"},
    {Register::r11, 11, "r11"},
    {Register::r10, 10, "r10"},
    {Register::r9, 9, "r9"},
    {Register::r8, 8, "r8"},
    {Register::rax, 0, "rax"},
    {Register::rcx, 2, "rcx"},
    {Register::rdx, 1, "rdx"},
    {Register::rsi, 4, "rsi"},
    {Register::rdi, 5, "rdi"},
    {Register::orig_rax, -1, "orig_rax"},
    {Register::rip, -1, "rip"},
    {Register::cs, 51, "cs"},
    {Register::rflags, 49, "eflags"},
    {Register::rsp, 7, "rsp"},
    {Register::ss, 52, "ss"},
    {Register::fs_base, 58, "fs_base"},
    {Register::gs_base, 59, "gs_base"},
    {Register::ds, 53, "ds"},
    {Register::es, 50, "es"},
    {Register::fs, 54, "fs"},
    {Register::gs, 55, "gs"},
}};

uint64_t getRegisterValue(pid_t pid, Register r);
uint64_t setRegisterValue(pid_t pid, Register r);

} // namespace tinydbg
