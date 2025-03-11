#include "timers.h"
#include "gb.h"

static const u16 TAC_TRIGGER_BITS[] = {512, 8, 32, 128};

static void inc_tima(Gameboy *gb)
{
    gb->memory.io_registers[IO_TIMA]++;
    if (gb->memory.io_registers[IO_TIMA] == 0)
    {
        gb->memory.tima_state = TIMA_RELOADING;
    }
}

static void advance_tima_state(Gameboy *gb)
{
    if (gb->memory.tima_state == TIMA_RELOADED)
    {
        gb->memory.tima_state = TIMA_RUNNING;
    }

    if (gb->memory.tima_state == TIMA_RELOADING)
    {
        // TODO: Change to function to make it more clear?
        gb->memory.io_registers[IO_IF] |= 4;
        gb->memory.io_registers[IO_TIMA] = gb->memory.io_registers[IO_TAC];
        gb->memory.tima_state = TIMA_RELOADED;
    }
}

static void check_tima_inc_triggers(Gameboy *gb, u16 new_div)
{
    u8 tac_enable = gb->memory.io_registers[IO_TAC] & 4;
    u16 tac_trigger_bit = TAC_TRIGGER_BITS[gb->memory.io_registers[IO_TAC] & 3];
    u16 falling_edges = gb->memory.div_counter & ~new_div; // Detects falling edges (bits that transition from 1 to 0)

    if (tac_enable && (falling_edges & tac_trigger_bit))
    {
        inc_tima(gb);
    }
}

static void set_div(Gameboy *gb, u16 value)
{
    check_tima_inc_triggers(gb, value);
    gb->memory.div_counter = value;
}

void timers_tick(Gameboy *gb)
{
    advance_tima_state(gb);
    // Increment div_counter by 4 every mcycle.
    // When it overflows (> TAC), this means that the DIV register is incremented.
    // The value if the DIV register is div_counter >> 8
    // NOTE: Only increment when not in stop mode.
    set_div(gb, gb->memory.div_counter + 4);
}
