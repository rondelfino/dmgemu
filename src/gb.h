#pragma once

#include "memory.h"
#include "sm83.h"

struct Gameboy
{
    SM83 cpu;
    Memory memory;
    // APU apu;

    u64 cycles_elapsed;

    bool previous_and_result;
};

void gb_init(Gameboy *gb);
void gb_free(Gameboy *gb);
u64 gb_run(Gameboy *gb);
