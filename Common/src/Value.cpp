//
// Created by Declan Walsh on 1/06/2024.
//

#include "Value.h"

VmPointer::VmPointer() : Address(0), Type(dtNone), Scope(scopeStackAbsolute)
{
    // Default
}

VmPointer::VmPointer(u16 address, DataType type, VarScopeType scope) : Address(address), Type(type), Scope(scope)
{
    //
}

VmPointer VmPointer::Null()
{

    return {};
}
