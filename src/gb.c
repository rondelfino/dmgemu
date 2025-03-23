#include "gb.h"
#include "timers.h"

void gb_init(Gameboy *gb)
{
    // memset(gb, 0, sizeof(*gb));

    // gb->cpu.bus = &gb->cpu_bus;
    gb->cpu.memory = &gb->memory;

    gb->memory.wram = (u8 *)malloc(gb->memory.wram_size = 0x2000);
    gb->memory.vram = (u8 *)malloc(gb->memory.vram_size = 0x2000);
    memset(gb->memory.wram, 0, gb->memory.wram_size);
    memset(gb->memory.vram, 0, gb->memory.vram_size);

    gb->memory.data_bus = 0;
    gb->memory.address_bus = 0;
    gb->memory.div_counter = 0;

    // TODO: DEBUG build only
    gb->memory.boot_rom_finished = true;

    memset(&(gb->cpu.reg), 0, sizeof(gb->cpu.reg));
    // gb->cpu.reg = {};
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
    gb->cpu.reg.ir = 0x00;
    gb->cpu.instructions = instructions;
    gb->cpu.instruction.microop_index = 0;
    gb->cpu.ime = Disabled;

    gb->cpu.fetching = false;
    gb->cpu.io_state = Idle;

    gb->memory.div_counter = 0xABCC;
    gb->memory.io_registers[IO_TAC] = 0xF8;
    gb->memory.io_registers[IO_IF] = 0xE1;

    gb->previous_and_result = 0;
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

static void gb_tick_t0(Gameboy *gb)
{
    sm83_tick_t0(&gb->cpu);
    timers_tick(gb);
}

static void gb_tick_t1(Gameboy *gb)
{
    sm83_tick_t1(&gb->cpu);
    timers_tick(gb);
}

static void gb_tick_t2(Gameboy *gb)
{
    sm83_tick_t2(&gb->cpu);
    timers_tick(gb);
}

static void gb_tick_t3(Gameboy *gb)
{
    sm83_tick_t3(&gb->cpu);
    timers_tick(gb);
}

u64 gb_run(Gameboy *gb)
{
    gb_tick_t0(gb);
    gb->cycles_elapsed++;

    gb_tick_t1(gb);
    gb->cycles_elapsed++;

    gb_tick_t2(gb);
    gb->cycles_elapsed++;

    gb_tick_t3(gb);
    gb->cycles_elapsed++;

    return gb->cycles_elapsed;
}
