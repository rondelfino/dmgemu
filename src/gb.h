#pragma once

#include "memory.h"
#include "ppu.h"
#include "sm83.h"

typedef struct
{
    SM83 cpu;
    Memory memory;
    PPU ppu;
    // APU apu;

    u64 cycles_elapsed;

    bool previous_and_result;
} Gameboy;

void gb_init(Gameboy *gb);
void gb_free(Gameboy *gb);
u64 gb_run(Gameboy *gb);
