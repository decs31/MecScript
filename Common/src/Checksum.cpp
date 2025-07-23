//
// Created by Declan Walsh on 26/06/2024.
//

#include "Checksum.h"

#include <assert.h>

u32 Checksum::Calculate(const u8 *data, const u32 length)
{

    /*
     * Checksum is initially done in 4 byte words.
     * Any bytes outside the 4 byte boundary are done separately.
     */

    if ((data == nullptr) || (length == 0) || ((length & 0x03) != 0)) {
        return 0;
    }

    u32 checksum = length;
    u32 *words = (u32 *)data;
    const u32 wordCount = length / sizeof(u32);

    for (u32 i = 0; i < wordCount; ++i) {
        checksum = (checksum ^ words[i]);
    }

    const u32 remainderStart = wordCount * sizeof(u32);
    for (u32 i = remainderStart; i < length; ++i) {
        checksum = (checksum ^ data[i]);
    }

    return checksum;
}
