#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "common.h"

struct SM83; // Forward declaration

typedef union
{
    u8 reg8;
    u8 reg16;
    u8 data;
    u8 flags;
} InstructionArg;

typedef void (*MicroOperation)(struct SM83 *cpu, InstructionArg *args);

typedef struct
{
    MicroOperation microops[6];
    InstructionArg args[2];
} Instruction;

extern Instruction instructions[];
extern Instruction cb_instructions[];
extern Instruction isr[];

#endif // INSTRUCTION_H

#endif
