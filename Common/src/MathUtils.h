/*
 * MathUtils.h
 *
 * Created: 23/01/2022 4:37:14 PM
 *  Author: Declan Walsh
 */

#ifndef MATHUTILS_H
#define MATHUTILS_H

// Bit Manipulators
#define mBitRead(val, bit)      (((val) >> (bit)) & 0x01)
#define mBitSet(val, bit)       ((val) |= (1UL << (bit)))
#define mBitClear(val, bit)     ((val) &= ~(1UL << (bit)))
#define mBitWrite(val, bit, bv) (bv ? mBitSet(val, bit) : mBitClear(val, bit))
#define mBitEquals(data, bit)   ((data) = (1UL << (bit)))

// Byte Deconstruction
#define mByte0(w)               ((u8)((w) & 0xFF))
#define mByte1(w)               ((u8)((w) >> 8))
#define mByte2(w)               ((u8)((w) >> 16))
#define mByte3(w)               ((u8)((w) >> 24))

#endif /* MATHUTILS_H */
