#pragma once

#include "memory.h"
#include "sm83.h"

struct Gameboy
{
    SM83 cpu;
    Memory memory;
};

void gb_init(Gameboy *gb);
void gb_free(Gameboy *gb);
