#include "gb.h"
#include <cstring>

void gb_init(Gameboy *gb)
{
    // memset(gb, 0, sizeof(*gb));

    gb->memory.ext_bus = {};
    gb->memory.cpu_bus = {};
    gb->memory.oam_bus = {};
    gb->memory.vram_bus = {};

    gb->cpu.bus = &gb->memory.cpu_bus;
    gb->cpu.memory = &gb->memory;

    gb->memory.wram = (u8 *)malloc(gb->memory.wram_size = 0x2000);
    gb->memory.vram = (u8 *)malloc(gb->memory.vram_size = 0x2000);
    memset(gb->memory.wram, 0, gb->memory.wram_size);
    memset(gb->memory.vram, 0, gb->memory.vram_size);

    // TODO: DEBUG build only
    gb->memory.boot_rom_finished = true;

    gb->cpu.reg = {};
    gb->cpu.reg.a = 0x01;
    gb->cpu.reg.c = 0x13;
    gb->cpu.reg.e = 0xD8;
    gb->cpu.reg.h = 0x01;
    gb->cpu.reg.l = 0x4D;
    gb->cpu.reg.pc = 0x100;
    gb->cpu.reg.sp = 0xFFFE;
    gb->cpu.reg.flags.c = 1;
    gb->cpu.reg.flags.h = 1;
    gb->cpu.reg.flags.z = 1;
    gb->cpu.reg.ir = gb_read_memory(&gb->memory, gb->cpu.reg.pc);
    gb->cpu.instructions = instructions;
}

void gb_free(Gameboy *gb)
{
    if (gb->memory.rom)
    {
        free(gb->memory.rom);
    }
    if (gb->memory.wram)
    {
        free(gb->memory.wram);
    }
    if (gb->memory.vram)
    {
        free(gb->memory.vram);
    }
}
