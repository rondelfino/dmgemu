#include "mmu.h"

u8 mmu_read_byte(MMU *mmu, u16 address, bool in_bootrom)
{
    u8 value = 0;

    if (mmu->bootrom_enabled && address <= 0xFF)
    {
        value = mmu->bootrom_memory[address];
    }
    else if (address == REG_LY)
    {
        value = 0x90;
    }
    else
    {
        value = mmu->memory_map[address];
    }

    // printf("Read: 0x%04X = 0x%02X\n", address, value);

    return value;
}

void mmu_write_byte(MMU *mmu, u16 address, u8 value)
{
    // printf("Write: 0x%04X = 0x%02X\n", address, value);
    // Blargg's tests - serial output
    if (address == 0xFF02)
    {
        mmu->memory_map[address] = value;

        if (value == 0x81)
        {
            char c = mmu->memory_map[0xFF01];

            printf("%c", c);

            mmu->memory_map[0xFF02] = 0;
        }
    }

    if (mmu->bootrom_enabled)
    {
        if (value && address == 0xFF50)
        {
            mmu->bootrom_enabled = false;
        }
    }
    else
    {
        mmu->memory_map[address] = value;
    }
    // else if ((address >= 0x8000) && (address <= 0x9FFF))
    // {
    //     // DMG only has 1 bank available
    //     mmu->previous_value = mmu->video_ram[0][address - 0x8000];
    //     mmu->video_ram[0][address - 0x8000] = value;
    // }
    // else if (address > 0x7FFF)
    // {
    //     mmu->memory_map[address] = value;
    // }
}

void write_word(MMU *mmu, u16 address, u16 value)
{
    mmu_write_byte(mmu, address, lsb(value));
    mmu_write_byte(mmu, address + 1, msb(value));
}
