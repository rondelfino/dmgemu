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
        // Request interrupt
        // TODO: Change to function to make it more clear?
        gb->memory.io_registers[IO_IF] |= 4;
        gb->memory.io_registers[IO_TIMA] = gb->memory.io_registers[IO_TMA];
        gb->memory.tima_state = TIMA_RELOADED;
    }
}

static void check_tima_inc_triggers(Gameboy *gb, u16 new_div)
{
    u16 tac_trigger_bit = TAC_TRIGGER_BITS[gb->memory.io_registers[IO_TAC] & 3];
    bool timer_enable = (gb->memory.io_registers[IO_TAC] & 4) != 0;
    bool div_triggered = (new_div & tac_trigger_bit) != 0;

    bool and_result = div_triggered & timer_enable;

    // Increment tima when we have a falling edge
    // NOTE: If the timer is disabled via TAC and we still have a falling edge, increment anyway
    if (and_result < gb->previous_and_result)
    {
        inc_tima(gb);
    }

    gb->previous_and_result = and_result;
}

static void set_div(Gameboy *gb, u16 value)
{
    gb->memory.div_counter = value;
}

void timers_tick(Gameboy *gb)
{
    advance_tima_state(gb);
    // Increment div_counter by 4 every mcycle.
    // When it overflows (> TAC), this means that the DIV register is incremented.
    // The value if the DIV register is div_counter >> 8
    // NOTE: Only increment when not in stop mode.
    u16 new_div = gb->memory.div_counter + 1;
    check_tima_inc_triggers(gb, new_div);
    set_div(gb, new_div);
}
