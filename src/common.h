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

enum
{
    /* Joypad and Serial */
    IO_JOYP = 0x00, // Joypad (R/W)
    IO_SB = 0x01,   // Serial transfer data (R/W)
    IO_SC = 0x02,   // Serial Transfer Control (R/W)

    /* Timers */
    IO_DIV = 0x04,  // Divider Register (R/W)
    IO_TIMA = 0x05, // Timer counter (R/W)
    IO_TMA = 0x06,  // Timer Modulo (R/W)
    IO_TAC = 0x07,  // Timer Control (R/W)

    IO_IF = 0x0F, // Interrupt Flag (R/W)

    /* Sound */
    IO_NR10 = 0x10, // Channel 1 Sweep register (R/W)
    IO_NR11 = 0x11, // Channel 1 Sound length/Wave pattern duty (R/W)
    IO_NR12 = 0x12, // Channel 1 Volume Envelope (R/W)
    IO_NR13 = 0x13, // Channel 1 Frequency lo (Write Only)
    IO_NR14 = 0x14, // Channel 1 Frequency hi (R/W)
    /* NR20 does not exist */
    IO_NR21 = 0x16, // Channel 2 Sound Length/Wave Pattern Duty (R/W)
    IO_NR22 = 0x17, // Channel 2 Volume Envelope (R/W)
    IO_NR23 = 0x18, // Channel 2 Frequency lo data (W)
    IO_NR24 = 0x19, // Channel 2 Frequency hi data (R/W)
    IO_NR30 = 0x1A, // Channel 3 Sound on/off (R/W)
    IO_NR31 = 0x1B, // Channel 3 Sound Length
    IO_NR32 = 0x1C, // Channel 3 Select output level (R/W)
    IO_NR33 = 0x1D, // Channel 3 Frequency's lower data (W)
    IO_NR34 = 0x1E, // Channel 3 Frequency's higher data (R/W)
    /* NR40 does not exist */
    IO_NR41 = 0x20, // Channel 4 Sound Length (R/W)
    IO_NR42 = 0x21, // Channel 4 Volume Envelope (R/W)
    IO_NR43 = 0x22, // Channel 4 Polynomial Counter (R/W)
    IO_NR44 = 0x23, // Channel 4 Counter/consecutive, Initial (R/W)
    IO_NR50 = 0x24, // Channel control / ON-OFF / Volume (R/W)
    IO_NR51 = 0x25, // Selection of Sound output terminal (R/W)
    IO_NR52 = 0x26, // Sound on/off

    IO_WAV_START = 0x30, // Wave pattern start
    IO_WAV_END = 0x3F,   // Wave pattern end

    /* Graphics */
    IO_LCDC = 0x40, // LCD Control (R/W)
    IO_STAT = 0x41, // LCDC Status (R/W)
    IO_SCY = 0x42,  // Scroll Y (R/W)
    IO_SCX = 0x43,  // Scroll X (R/W)
    IO_LY = 0x44,   // LCDC Y-Coordinate (R)
    IO_LYC = 0x45,  // LY Compare (R/W)
    IO_DMA = 0x46,  // DMA Transfer and Start Address (W)
    IO_BGP = 0x47,  // BG Palette Data (R/W) - Non CGB Mode Only
    IO_OBP0 = 0x48, // Object Palette 0 Data (R/W) - Non CGB Mode Only
    IO_OBP1 = 0x49, // Object Palette 1 Data (R/W) - Non CGB Mode Only
    IO_WY = 0x4A,   // Window Y Position (R/W)
    IO_WX = 0x4B,   // Window X Position minus 7 (R/W)
};

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
