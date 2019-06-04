#include "symbol.h"

namespace tinydbg {

std::string toString(SymbolType st)
{
    switch (st) {
    case SymbolType::Notype:
        return "Notype";
    case SymbolType::Object:
        return "Object";
    case SymbolType::Func:
        return "Func";
    case SymbolType::Section:
        return "Section";
    case SymbolType::File:
        return "File";
    }
}

SymbolType toSymbolType(elf::stt sym)
{
    switch (sym) {
    case elf::stt::notype:
        return SymbolType::Notype;
    case elf::stt::object:
        return SymbolType::Object;
    case elf::stt::func:
        return SymbolType::Func;
    case elf::stt::section:
        return SymbolType::Section;
    case elf::stt::file:
        return SymbolType::File;
    }
};

} // namespace tinydbg
