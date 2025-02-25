#include "gb.h"
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

bool load_bootrom(Gameboy *gb, std::string filename)
{
}

void load_rom(Gameboy *gb, char *filename)
{
    FILE *f;
    fopen_s(&f, filename, "rb");

    if (f)
    {
        /* Find the ROM file size */
        fseek(f, 0L, SEEK_END);
        u64 rom_size = ftell(f);
        gb->memory.rom_size = rom_size;
        fseek(f, 0L, SEEK_SET);

        if (gb->memory.rom)
        {
            free(gb->memory.rom);
        }

        if (rom_size > 0)
        {
            gb->memory.rom = (u8 *)malloc(gb->memory.rom_size);
            memset(gb->memory.rom, 0xFF, gb->memory.rom_size);
            if (rom_size <= 0x10000)
            {
                fread(gb->memory.rom + gb->cpu.reg.pc, 1, gb->memory.rom_size, f);
                fseek(f, 0L, SEEK_SET);
                fread(gb->memory.memory_map + gb->cpu.reg.pc, 1, gb->memory.rom_size, f);
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
        fclose(f);
    }
    else
    {
        /* Handle file opening error */
        printf("Error: Failed to open the ROM file.\n");
        exit(EXIT_FAILURE);
    }
}

void dmg_tick_t1(Gameboy *gb)
{
    sm83_tick_t1(&gb->cpu);
}

void dmg_tick_t2(Gameboy *gb)
{
    sm83_tick_t2(&gb->cpu);
}

void dmg_tick_t3(Gameboy *gb)
{
    sm83_tick_t3(&gb->cpu);
}

void dmg_tick_t4(Gameboy *gb)
{
    sm83_tick_t4(&gb->cpu);
}

void dmg_cycle(Gameboy *gb)
{
    dmg_tick_t1(gb);
    dmg_tick_t2(gb);
    dmg_tick_t3(gb);
    dmg_tick_t4(gb);
}

int main(int argv, char **argc)
{
    Gameboy gb = {};

    char *bootrom = "../roms/dmg_boot.bin";
    char *rom_path = "R:/dmg/tests/roms/blargg/cpu_instrs/individual/03-op sp,hl.gb";

    load_rom(&gb, rom_path);
    gb_init(&gb);

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
    // fetch(&gb.cpu);
    // for (u64 tick = 0; tick < CLOCK_FREQ; tick += 4)
    for (;;)
    {
        dmg_cycle(&gb);
    }
    // }

    std::cout << '\n';
    return 0;
}
