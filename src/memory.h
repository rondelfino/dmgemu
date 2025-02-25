#pragma once

#include "common.h"

struct Bus
{
    u16 address;
    u8 data;
    u8 previous_data;
    bool active;

    // typedef u8 read(Bus *bus, u16 address);
    // typedef void write(Bus *bus, u16 address, u8 value);

    // Memory *memory;
};

enum BusType
{
    BUS_EXT,
    BUS_CPU,
    BUS_OAM,
    BUS_VRAM,
};

struct Memory
{
    u8 memory_map[0xFFFF];

    u8 *rom;
    u32 rom_size;

    u8 *wram;
    u32 wram_size;

    // Video
    u8 *vram;
    u32 vram_size;
    // u8 oam[0xFF00 - 0xFE00];

    // MBC
    u8 *banked_ram;
    u32 banked_ram_size;

    // Hardware Registers
    u8 io_registers[0x80];
    u8 hram[0xFFFF - 0xFF80];

    // Buses
    Bus ext_bus;
    Bus cpu_bus;
    Bus oam_bus;
    Bus vram_bus;

    // IE
    u8 interrupt_enable;

    u8 boot_rom[0x100];
    bool boot_rom_finished;
};

// TODO: Refactor functions to check for bus type to prevent conflicts
u8 gb_read_memory(Memory *memory, u16 address);
void gb_write_memory(Memory *memory, u16 address, u8 value);
