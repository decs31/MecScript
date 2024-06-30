//
// Created by Declan Walsh on 26/06/2024.
//

#ifndef MECSCRIPT_CHECKSUM_H
#define MECSCRIPT_CHECKSUM_H

#include "BasicTypes.h"

namespace Checksum
{
    u32 Calculate(const u8 *data, const u32 length);
}

#endif //MECSCRIPT_CHECKSUM_H
