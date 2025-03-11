#include "memory.h"

typedef u8 read_function(Memory *memory, u16 address);
typedef void write_function(Memory *memory, u16 address, u8 value);

static u8 read_rom(Memory *memory, u16 address)
{
    if (address < 0x100 && !memory->boot_rom_finished)
    {
        return memory->boot_rom[address];
    }

    if (!memory->rom_size)
    {
        return 0xFF;
    }
    // TODO: Multicart support (ROM0 banking)
    return memory->rom[address];
}

static u8 read_rom_bank(Memory *memory, u16 address)
{
    return memory->rom[address];
}

static u8 read_vram(Memory *memory, u16 address)
{
    // TODO: Change indexing method
    u16 effective_address = address;
    u16 msb = address & 0xF000;
    u16 offset = 0;
    offset = msb - 0x8000;
    effective_address = (address & 0x0FFF) | offset;

    ASSERT(effective_address < memory->vram_size);
    return memory->vram[effective_address];
}

static u8 read_mbc_ram(Memory *memory, u16 address)
{
}

static u8 read_wram(Memory *memory, u16 address)
{
    // TODO: Change indexing method
    u16 effective_address = address;
    u16 msb = address & 0xF000;
    u16 offset = 0;
    offset = msb - 0xC000;
    effective_address = (address & 0x0FFF) | offset;

    return memory->wram[effective_address];
}

// static u8 read_banked_ram(Memory *memory, u16 address)
// {
//     return memory->wram[(address & 0x0FFF)];
// }

static u8 read_high_memory(Memory *memory, u16 address)
{
    if (address == 0xFFFF)
    {
        return memory->interrupt_enable;
    }

    if (address == 0xFF44)
    {
        return 0x90;
    }

    if (address < 0xFF80)
    {
        ASSERT((address & 0x00FF) < 0x80);

        switch (address & 0xFF)
        {
        case IO_IF:
            // TODO: |= 0xE1
            return memory->io_registers[IO_IF];
        case IO_TAC:
            return memory->io_registers[IO_TAC] | 0xF8;
        case IO_TMA:
            return memory->io_registers[address & 0xFF];
        case IO_TIMA:
            return memory->io_registers[IO_TIMA];
        case IO_DIV:
            return memory->div_counter >> 8;
        }

        return memory->io_registers[address & 0x00FF];
    }
    ASSERT(((address & 0x00FF) % 0x0080) < (0xFFFF - 0xFF80));
    return memory->hram[(address & 0x00FF) % 0x0080];
}

static read_function *read_memory_map[] = {
    read_rom,      read_rom,         read_rom,      read_rom,      // 0x0XXX, 0x1XXX, 0x2XXX, 0x3XXX
    read_rom_bank, read_rom_bank,    read_rom_bank, read_rom_bank, // 0x4XXX, 0x5XXX, 0x6XXX, 0x7XXX
    read_vram,     read_vram,                                      // 0x8XXX, 0x9XXX
    read_mbc_ram,  read_mbc_ram,                                   // 0xAXXX, 0xBXXX
    read_wram,     read_wram,                                      // 0xCXXX, 0xDXXX
    read_wram,     read_high_memory,                               // 0xEXXX, 0xFXXX
};

u8 gb_read_memory(Memory *memory, u16 address)
{
    u8 data = read_memory_map[address >> 12](memory, address);
    if (address >= 0x8000 && address != 0xFF44)
    {
        // ASSERT(data == memory->memory_map[address]);
    }

    memory->data_bus = data;
    return data;
}

static void write_mbc(Memory *memory, u16 address, u8 value)
{
}

static void write_vram(Memory *memory, u16 address, u8 value)
{
    // TODO: Change indexing method
    u16 effective_address = address;
    u16 msb = address & 0xF000;
    u16 offset = 0;
    offset = msb - 0x8000;
    effective_address = (address & 0x0FFF) | offset;

    ASSERT(effective_address < memory->vram_size);
    memory->vram[effective_address] = value;
}

static void write_mbc_ram(Memory *memory, u16 address, u8 value)
{
}

static void write_wram(Memory *memory, u16 address, u8 value)
{
    // TODO: Change indexing method
    u16 effective_address = address;
    u16 msb = address & 0xF000;
    u16 offset = 0;
    offset = msb - 0xC000;
    effective_address = (address & 0x0FFF) | offset;

    memory->wram[effective_address] = value;
}

static void write_banked_ram(Memory *memory, u16 address, u8 value)
{
    write_wram(memory, address, value);
}

static void write_high_memory(Memory *memory, u16 address, u8 value)
{
    if (address == 0xFFFF)
    {
        memory->interrupt_enable = value;
    }
    // NOTE: Blargg's tests
    else if (address == 0xFF02)
    {
        memory->io_registers[address & 0x00FF] = value;

        if (value == 0x81)
        {
            char c = memory->io_registers[0xFF01 & 0x00FF];

            printf("%c", c);

            memory->io_registers[0xFF02 & 0x00FF] = 0;
        }
    }
    else if (address < 0xFF80)
    {
        switch (address & 0xFF)
        {
        case IO_IF:
            memory->io_registers[address & 0xFF] = value;
            return;
        case IO_DIV:
            memory->io_registers[IO_DIV] = 0;
            return;
        case IO_TIMA:

            return;
        case IO_TAC:
            memory->io_registers[IO_TAC] = value;
            return;
        }

        ASSERT((address & 0x00FF) < 0x80);
        memory->io_registers[address & 0x00FF] = value;
    }
    else
    {
        ASSERT(((address & 0x00FF) % 0x0080) < (0xFFFF - 0xFF80));
        memory->hram[(address & 0x00FF) % 0x0080] = value;
    }
}

static write_function *write_memory_map[] = {
    write_mbc,     write_mbc,         write_mbc, write_mbc, // 0x0XXX, 0x1XXX, 0x2XXX, 0x3XXX
    write_mbc,     write_mbc,         write_mbc, write_mbc, // 0x4XXX, 0x5XXX, 0x6XXX, 0x7XXX
    write_vram,    write_vram,                              // 0x8XXX, 0x9XXX
    write_mbc_ram, write_mbc_ram,                           // 0xAXXX, 0xBXXX
    write_wram,    write_wram,                              // 0xCXXX, 0xDXXX
    write_wram,    write_high_memory,                       // 0xEXXX, 0xFXXX
};

void gb_write_memory(Memory *memory, u16 address, u8 value)
{
    write_memory_map[address >> 12](memory, address, value);

    memory->data_bus = value;
}
