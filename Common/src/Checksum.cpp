//
// Created by Declan Walsh on 26/06/2024.
//

#include "Checksum.h"

u32 Checksum::Calculate(const u8 *data, const u32 length) {

    if (data == nullptr || length == 0)
        return 0;

    u32 checksum = length;

    for (u32 i = 0; i < length; ++i) {
        checksum = (checksum ^ data[i]);
    }

    return checksum;
}