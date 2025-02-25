#ifndef MMU_H
#define MMU_H

#include "common.h"

#define READ_IF()
#define READ_IE()

struct MMU
{
    enum MBCTypes
    {
        NO_ROM,
        MBC1,
        MBC2,
        MBC3,
        MBC4,
        MBC5,
        MBC6,
        MBC7,
        MMM01,
        M161,
        HUC1,
        HUC3,
        TAMA5,
    };

    u8 memory_map[0xFFFF];
    u8 bootrom_memory[0x100];

    // 8 KiB of VRAM
    // 0/1 banks in CGB mode
    u8 video_ram[2][0x2000];

    bool bootrom_enabled;

    // Cartridge data structure
    struct CartData
    {
        // General MBC attributes
        u32 rom_size;
        u32 ram_size;
        MBCTypes mbc_type;
        bool battery;
        bool ram;
        bool multicart;
        bool sonar;
        bool rumble;

        // GB Memory Cartridge
        u8 gb_mem_map[128];
    } cart;

    u8 previous_value;
};

u8 mmu_read_byte(MMU *mmu, u16 address, bool in_bootrom);
void mmu_write_byte(MMU *mmu, u16 address, u8 value);

#endif
