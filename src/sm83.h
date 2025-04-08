#ifndef SM83_H
#define SM83_H

#pragma once

#include "common.h"
#include "idu.h"
#include "memory.h"

typedef struct SM83 SM83;

typedef enum
{
    REG8_F,
    REG8_A,
    REG8_C,
    REG8_B,
    REG8_E,
    REG8_D,
    REG8_L,
    REG8_H,
    REG8_COUNT
} Reg8;

typedef enum
{
    REG16_AF,
    REG16_BC,
    REG16_DE,
    REG16_HL,
    REG16_SP,
    REG16_COUNT
} Reg16;

enum
{
    CARRY_FLAG = 0x10,
    HALF_CARRY_FLAG = 0x20,
    SUBTRACT_FLAG = 0x40,
    ZERO_FLAG = 0x80
};

typedef enum
{
    INTR_NONE,
    INTR_PENDING,
    INTR_SERVICING,
    INTR_SERVICED
} InterruptState;

typedef enum
{
    IME_DISABLED,
    IME_REQUESTED,
    IME_ENABLED
} IMEState;

typedef enum
{
    SM83_IO_IDLE,
    SM83_IO_READ,
    SM83_IO_WRITE
} IOState;

typedef struct
{
    u8 reserved : 4; // Unused bits (always 0)
    bool c : 1;      // Carry flag
    bool h : 1;      // Half-carry flag
    bool n : 1;      // Subtract flag
    bool z : 1;      // Zero flag
} Flags;

typedef struct
{
    union
    {
        u16 word_regs[REG16_COUNT];
        u8 byte_regs[REG8_COUNT];
        struct
        {
            union
            {
                struct
                {
                    union
                    {
                        u8 f;
                        Flags flags;
                    };
                    u8 a;
                };
                u16 af;
            };

            union
            {
                struct
                {
                    u8 c;
                    u8 b;
                };
                u16 bc;
            };

            union
            {
                struct
                {
                    u8 e;
                    u8 d;
                };
                u16 de;
            };

            union
            {
                struct
                {
                    u8 l;
                    u8 h;
                };
                u16 hl;
            };
            u16 sp;
        };
    };

    u16 pc;
    u8 ir;
} Register;

typedef struct
{
    u8 value;
} InstructionArg;

typedef void (*MicroOperation)(struct SM83 *cpu, InstructionArg *args);

typedef struct
{
    MicroOperation microops[6];
    InstructionArg args[2];
} Instruction;

struct SM83
{
    // Scratch
    struct
    {
        u16 nn;
        u16 address;
        u8 lsb;
        u8 msb;
        u8 n;
        bool b;
    };

    // Registers
    Register reg;

    struct
    {
        InterruptState state;
        u8 remaining_ticks;
    } interrupt;

    // Current instruction
    struct
    {
        u8 microop_index;
        InstructionArg *args;
    } instruction;

    // ISA
    Instruction *instructions;

    Memory *memory;

    IMEState ime;
    IOState io_state;

    bool halted;
    bool fetching;
};

static inline u8 reset_bit(u8 value, u8 bit)
{
    value &= ~bit;
    return value;
}

static inline u8 set_bit(u8 value, u8 bit)
{
    value |= bit;
    return value;
}

void sm83_reset(SM83 *cpu);

void sm83_tick_t0(SM83 *cpu);
void sm83_tick_t1(SM83 *cpu);
void sm83_tick_t2(SM83 *cpu);
void sm83_tick_t3(SM83 *cpu);

#endif
