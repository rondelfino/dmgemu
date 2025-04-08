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

enum
{
    INTERRUPT_VBLANK = 1,
    INTERRUPT_LCD = 2,
    INTERRUPT_TIMER = 4,
    INTERRUPT_SERIAL = 8,
    INTERRUPT_JOYPAD = 16
};

typedef enum
{
    TIMA_RUNNING,
    TIMA_RELOADING,
    TIMA_RELOADED
} TimaState;

typedef enum
{
    BUS_EXT,
    BUS_CPU,
    BUS_OAM,
    BUS_VRAM
} BusType;

enum
{
    /* Joypad and Serial */
    IO_JOYP = 0x00, // Joypad (R/W)
    IO_SB = 0x01,   // Serial transfer data (R/W)
    IO_SC = 0x02,   // Serial Transfer Control (R/W)

    /* Timers */
    IO_DIV = 0x04,  // Divider Register (R/W)
    IO_TIMA = 0x05, // Timer counter (R/W)
    IO_TMA = 0x06,  // Timer Modulo (R/W)
    IO_TAC = 0x07,  // Timer Control (R/W)

    IO_IF = 0x0F, // Interrupt Flag (R/W)

    /* Sound */
    IO_NR10 = 0x10, // Channel 1 Sweep register (R/W)
    IO_NR11 = 0x11, // Channel 1 Sound length/Wave pattern duty (R/W)
    IO_NR12 = 0x12, // Channel 1 Volume Envelope (R/W)
    IO_NR13 = 0x13, // Channel 1 Frequency lo (Write Only)
    IO_NR14 = 0x14, // Channel 1 Frequency hi (R/W)
    /* NR20 does not exist */
    IO_NR21 = 0x16, // Channel 2 Sound Length/Wave Pattern Duty (R/W)
    IO_NR22 = 0x17, // Channel 2 Volume Envelope (R/W)
    IO_NR23 = 0x18, // Channel 2 Frequency lo data (W)
    IO_NR24 = 0x19, // Channel 2 Frequency hi data (R/W)
    IO_NR30 = 0x1A, // Channel 3 Sound on/off (R/W)
    IO_NR31 = 0x1B, // Channel 3 Sound Length
    IO_NR32 = 0x1C, // Channel 3 Select output level (R/W)
    IO_NR33 = 0x1D, // Channel 3 Frequency's lower data (W)
    IO_NR34 = 0x1E, // Channel 3 Frequency's higher data (R/W)
    /* NR40 does not exist */
    IO_NR41 = 0x20, // Channel 4 Sound Length (R/W)
    IO_NR42 = 0x21, // Channel 4 Volume Envelope (R/W)
    IO_NR43 = 0x22, // Channel 4 Polynomial Counter (R/W)
    IO_NR44 = 0x23, // Channel 4 Counter/consecutive, Initial (R/W)
    IO_NR50 = 0x24, // Channel control / ON-OFF / Volume (R/W)
    IO_NR51 = 0x25, // Selection of Sound output terminal (R/W)
    IO_NR52 = 0x26, // Sound on/off

    IO_WAV_START = 0x30, // Wave pattern start
    IO_WAV_END = 0x3F,   // Wave pattern end

    /* Graphics */
    IO_LCDC = 0x40, // LCD Control (R/W)
    IO_STAT = 0x41, // LCDC Status (R/W)
    IO_SCY = 0x42,  // Scroll Y (R/W)
    IO_SCX = 0x43,  // Scroll X (R/W)
    IO_LY = 0x44,   // LCDC Y-Coordinate (R)
    IO_LYC = 0x45,  // LY Compare (R/W)
    IO_DMA = 0x46,  // DMA Transfer and Start Address (W)
    IO_BGP = 0x47,  // BG Palette Data (R/W) - Non CGB Mode Only
    IO_OBP0 = 0x48, // Object Palette 0 Data (R/W) - Non CGB Mode Only
    IO_OBP1 = 0x49, // Object Palette 1 Data (R/W) - Non CGB Mode Only
    IO_WY = 0x4A,   // Window Y Position (R/W)
    IO_WX = 0x4B    // Window X Position minus 7 (R/W)
};

typedef struct
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

    TimaState tima_state; // NOTE: After TIMA overflows, it takes 1 mcycle before it is reloaded.

    // Buses
    u8 data_bus;
    u16 address_bus;

    // IE
    u8 interrupt_enable;

    u8 boot_rom[0x100];
    bool boot_rom_finished;

    // 8 bit DIV register, but internally a 16-bit register?
    u16 div_counter;
} Memory;

// TODO: Refactor functions to check for bus type to prevent conflicts
u8 gb_read_memory(Memory *memory, u16 address);
void gb_write_memory(Memory *memory, u16 address, u8 value);
void gb_request_interrupt(Memory *memory, u8 interrupt);
