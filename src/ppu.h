#ifndef PPU_H
#define PPU_H

#include "common.h"
#include "mmu.h"

struct PPU
{
    MMU *memory;
};

void ppu_tick(PPU *ppu);

#endif
