#pragma once

#include "elf/elf++.hh"

#include <cstddef>
#include <string>

namespace tinydbg {

enum class SymbolType {
    Notype, // No type (e.g., absolute symbol)
    Object, // Data Object
    Func, // Function entry point
    Section, // Symbol is associated with a Section
    File, // Source File associated with the
};

struct Symbol {
    SymbolType type;
    std::string name;
    uint64_t addr;
};

std::string toString(SymbolType st);
SymbolType toSymbolType(elf::stt sym);

} // namespace tinydbg
