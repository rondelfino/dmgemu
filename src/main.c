#include "gb.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include <stdio.h>

bool load_bootrom(Gameboy *gb, char *filename)
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
            if (rom_size < 0x10000)
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

int main(int argv, char **argc)
{
    Gameboy gb = {};

    char *bootrom = "../roms/dmg_boot.bin";
    char *rom_path = "R:/dmg/tests/roms/blargg/mem_timing/individual/01-read_timing.gb";

    gb_init(&gb);
    gb.cpu.reg.pc = 0x00;
    load_rom(&gb, rom_path);
    gb.cpu.reg.pc = 0x100;

    u64 cycles = 0;
    for (;;)
    {
        cycles += gb_run(&gb);
    }

    return 0;
}
