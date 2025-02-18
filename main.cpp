#include "sm83.h"
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

void create_log_file()
{
    FILE *log = fopen("cpu_log.txt", "w");
    if (!log)
    {
    }
    fclose(log);
}

struct DMGCore
{
    // CPU
    SM83 cpu;

    // Buses

    MMU mmu;
};

bool read_file(u8 *memory, std::string filename)
{
    std::ifstream file(filename, std::ios_base::binary);

    // Determine the length of the file by seeking
    // to the end of the file, reading the value of the
    // position indicator, and then seeking back to the beginning.
    file.seekg(0, std::ios_base::end);
    u16 length = file.tellg();
    if (!length)
    {
        return false;
    }
    file.seekg(0, std::ios_base::beg);

    // Make a buffer of the exact size of the file and read the data into it.
    file.read(reinterpret_cast<char *>(memory), length);

    file.close();

    return true;
}

bool load_bootrom(DMGCore *core, std::string filename)
{
    if (read_file(core->mmu.bootrom_memory, filename))
    {
        core->mmu.bootrom_enabled = true;
        return true;
    }

    return false;
}

void load_rom(DMGCore *core, u8 *memory, char *filename)
{
    FILE *rom;
    fopen_s(&rom, filename, "rb");

    if (rom)
    {
        /* Find the ROM file size */
        fseek(rom, 0L, SEEK_END);
        u64 rom_size = ftell(rom);
        fseek(rom, 0L, SEEK_SET);

        if (rom_size > 0)
        {
            if (rom_size <= 0x10000)
            {
                fread(memory + core->cpu.reg.pc, 1, rom_size, rom);
            }
            else
            {
                /* Handle ROM too large for memory */
                printf("Error: ROM size exceeds memory bounds.\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            /* Handle empty ROM file */
            printf("Error: ROM file is empty.\n");
            exit(EXIT_FAILURE);
        }
        fclose(rom);
    }
    else
    {
        /* Handle file opening error */
        printf("Error: Failed to open the ROM file.\n");
        exit(EXIT_FAILURE);
    }
}

void dmg_init(DMGCore *core)
{
    core->cpu.reg = {};
    core->cpu.reg.a = 0x01;
    core->cpu.reg.c = 0x13;
    core->cpu.reg.e = 0xD8;
    core->cpu.reg.h = 0x01;
    core->cpu.reg.l = 0x4D;
    core->cpu.reg.pc = 0x100;
    core->cpu.reg.sp = 0xFFFE;
    core->cpu.reg.flags.c = 1;
    core->cpu.reg.flags.h = 1;
    core->cpu.reg.flags.z = 1;
    core->cpu.reg.ir = mmu_read_byte(&core->mmu, core->cpu.reg.pc, false);
    // core->cpu.instructions = instructions;
}

void dmg_tick_t1(DMGCore *core)
{
    sm83_tick_t1(&core->cpu);
}

void dmg_tick_t2(DMGCore *core)
{
    sm83_tick_t2(&core->cpu);
}

void dmg_tick_t3(DMGCore *core)
{
    sm83_tick_t3(&core->cpu);
}

void dmg_tick_t4(DMGCore *core)
{
    sm83_tick_t4(&core->cpu);
}

void dmg_cycle(DMGCore *core)
{
    dmg_tick_t1(core);
    dmg_tick_t2(core);
    dmg_tick_t3(core);
    dmg_tick_t4(core);
}

int main(int argv, char **argc)
{
    DMGCore core = {};
    core.cpu.memory = &core.mmu;

    char *bootrom = "../roms/dmg_boot.bin";
    char *rom_path = "R:/dmg/tests/roms/blargg/cpu_instrs/individual/11-op a,(hl).gb";
    // load_bootrom(&core, bootrom);
    load_rom(&core, core.mmu.memory_map, rom_path);
    dmg_init(&core);

    create_log_file();

    auto start = std::chrono::high_resolution_clock::now();
    // while (1)
    // {
    //     auto elapsed_ms = std::chrono::high_resolution_clock::now() - start;
    //     if (elapsed_ms.count() < CLOCK_FREQ)
    //     {
    //         while (elapsed_ms.count() < CLOCK_FREQ)
    //         {
    //         }
    //     }
    //
    fetch(&core.cpu);
    // for (u64 tick = 0; tick < CLOCK_FREQ; tick += 4)
    for (;;)
    {
        dmg_cycle(&core);
    }
    // }

    std::cout << '\n';
    return 0;
}
