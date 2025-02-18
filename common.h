#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

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

const u32 CLOCK_FREQ = 4194304;
const u32 CPU_FREQ = 1048576;

// ROM Header
const u16 ROM_COLOR = 0x143;
const u16 ROM_MBC = 0x147;
const u16 ROM_ROMSIZE = 0x148;
const u16 ROM_RAMSIZE = 0x149;

// Object Attribute Memory
const u16 OAM_START = 0xFE00;

// Joypad
const u16 REG_P1 = 0xFF00;

// Timer
const u16 REG_DIV = 0xFF04;
const u16 REG_TIMA = 0xFF05;
const u16 REG_TMA = 0xFF06;
const u16 REG_TAC = 0xFF07;

// Interrupt Flags
const u16 IF_FLAG = 0xFF0F;
const u16 IE_FLAG = 0xFFFF;

// Sound registers
const u16 NR10 = 0xFF10;
const u16 NR11 = 0xFF11;
const u16 NR12 = 0xFF12;
const u16 NR13 = 0xFF13;
const u16 NR14 = 0xFF14;

const u16 NR21 = 0xFF16;
const u16 NR22 = 0xFF17;
const u16 NR23 = 0xFF18;
const u16 NR24 = 0xFF19;

const u16 NR30 = 0xFF1A;
const u16 NR31 = 0xFF1B;
const u16 NR32 = 0xFF1C;
const u16 NR33 = 0xFF1D;
const u16 NR34 = 0xFF1E;

const u16 NR41 = 0xFF20;
const u16 NR42 = 0xFF21;
const u16 NR43 = 0xFF22;
const u16 NR44 = 0xFF23;

const u16 NR50 = 0xFF24;
const u16 NR51 = 0xFF25;
const u16 NR52 = 0xFF26;

// Display registers
const u16 REG_LCDC = 0xFF40;
const u16 REG_STAT = 0xFF41;
const u16 REG_SY = 0xFF42;
const u16 REG_SX = 0xFF43;
const u16 REG_LY = 0xFF44;
const u16 REG_LYC = 0xFF45;
const u16 REG_DMA = 0xFF46;
const u16 REG_BGP = 0xFF47;
const u16 REG_OBP0 = 0xFF48;
const u16 REG_OBP1 = 0xFF49;
const u16 REG_WY = 0xFF4A;
const u16 REG_WX = 0xFF4B;

// Double Speed Control
const u16 REG_KEY1 = 0xFF4D;

const u16 BOOTROM = 0xFF50;

// HDMA
const u16 REG_HDMA1 = 0xFF51;
const u16 REG_HDMA2 = 0xFF52;
const u16 REG_HDMA3 = 0xFF53;
const u16 REG_HDMA4 = 0xFF54;
const u16 REG_HDMA5 = 0xFF55;

// Video RAM Bank
const u16 REG_VBK = 0xFF4F;

// GBC IR Port
const u16 REG_RP = 0xFF56;

// Serial Input-Output
const u16 REG_SB = 0xFF01;
const u16 REG_SC = 0xFF02;

// GBC palettes
const u16 REG_BCPS = 0xFF68;
const u16 REG_BCPD = 0xFF69;
const u16 REG_OCPS = 0xFF6A;
const u16 REG_OCPD = 0xFF6B;

// Working RAM Bank
const u16 REG_SVBK = 0xFF70;

#endif
