#ifndef COMMON_H
#define COMMON_H

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float r32;
typedef double r64;

typedef uint32_t b32;

#define ASSERT(exp)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(exp))                                                                                                    \
        {                                                                                                              \
            __builtin_trap();                                                                                          \
        }                                                                                                              \
    } while (0)

inline s8 signed_8(u8 n)
{
    s8 value = n <= INT8_MAX ? (s8)n : (s8)(n - INT8_MIN) + INT8_MIN;
    return value;
}

inline u16 unsigned_16(u8 msb, u8 lsb)
{
    return (msb << 8) | lsb;
}

inline u8 msb(u16 word)
{
    u8 result = (word >> 8);
    return result;
}

inline u8 lsb(u16 word)
{
    u8 result = (word & 0xFF);
    return result;
}

inline bool get_bit(u8 bit_pos, u16 value)
{
    return (value >> bit_pos) & 0x01;
}

inline u8 bit(u8 bit_pos, u16 value)
{
    return ((u64)1 << bit_pos) & value;
}

#endif
