#ifndef SM83_H
#define SM83_H

#include "common.h"
#include "idu.h"
#include "memory.h"

enum
{
    REG8_F,
    REG8_A,
    REG8_C,
    REG8_B,
    REG8_E,
    REG8_D,
    REG8_L,
    REG8_H,
    REG8_COUNT
};

enum
{
    REG16_AF,
    REG16_BC,
    REG16_DE,
    REG16_HL,
    REG16_SP,
    REG16_PC,
    REG16_COUNT
};

enum
{
    CARRY_FLAG = 0x10,
    HALF_CARRY_FLAG = 0x20,
    SUBTRACT_FLAG = 0x40,
    ZERO_FLAG = 0x80,
};

enum InterruptState
{
    None,
    Pending,
    Servicing,
    Serviced,
};

struct Flags
{
    u8 reserved : 4; // Unused bits (always 0)
    bool c : 1;      // Carry flag
    bool h : 1;      // Half-carry flag
    bool n : 1;      // Subtract flag
    bool z : 1;      // Zero flag
};

struct Register
{
    union
    {
        u16 word_regs[REG16_COUNT];
        u8 byte_regs[REG8_COUNT];
        struct
        {
            union
            {
                struct
                {
                    union
                    {
                        u8 f;
                        Flags flags;
                    };
                    u8 a;
                };
                u16 af;
            };

            union
            {
                struct
                {
                    u8 c;
                    u8 b;
                };
                u16 bc;
            };

            union
            {
                struct
                {
                    u8 e;
                    u8 d;
                };
                u16 de;
            };

            union
            {
                struct
                {
                    u8 l;
                    u8 h;
                };
                u16 hl;
            };
            u16 sp;
        };
    };

    u16 pc;
    u8 ir; // Instruction register
};

enum IMEState
{
    Disabled,
    Requested,
    Enabled
};

union InstructionArg
{
    u8 reg8;
    u8 reg16;
    u8 data;
    u8 flags;
};

struct Instruction;
struct SM83;

typedef void (*MicroOperation)(SM83 *cpu, InstructionArg *args);
struct Instruction
{
    MicroOperation microops[6];
    InstructionArg args[2];
};

enum IOState
{
    Idle,
    Read,
    Write
};

struct SM83
{
    // Registers
    Register reg;

    // ISA
    Instruction *instructions;

    // Current instruction
    struct
    {
        u8 microop_index;
        InstructionArg *args;
    } instruction;

    bool fetching;
    IOState io_state;

    // Memory
    Memory *memory;

    IMEState ime;
    struct
    {
        InterruptState state;
        u8 remaining_ticks;
    } interrupt;
    bool halted;

    // Scratch
    struct
    {
        bool b;
        u8 lsb;
        u8 msb;
        u8 n;
        u16 nn;
        u16 address;
    };

    u32 clockm;
    u32 clockt;
    u32 cycles;
};

extern Instruction instructions[];
extern Instruction cb_instructions[];
extern Instruction isr[];

static inline u8 reset_bit(u8 value, u8 bit)
{
    value &= ~bit;
    return value;
}

static inline u8 set_bit(u8 value, u8 bit)
{
    value |= bit;
    return value;
}

void sm83_reset(SM83 *cpu);

void sm83_tick_t0(SM83 *cpu);
void sm83_tick_t1(SM83 *cpu);
void sm83_tick_t2(SM83 *cpu);
void sm83_tick_t3(SM83 *cpu);

/*********** Instructions ***********/
/*
    r: 8-bit register
    rr: 16-bit register

    n: unsigned 8-bit immediate data
    nn: unsigned 16-bit little endian immediate data
    e: signed 8-bit immediate data

    a: unsigned 8-bit data, which is added to 0xFF00 to create a 16-bit address in HRAM
    aa: 16-bit little endian address

    cc: if given flag condition is true
    ncc: if given flag condition is false
*/

void unimplemented_instruction(SM83 *cpu, InstructionArg *args);
void nop(SM83 *cpu, InstructionArg *args);
void stop(SM83 *cpu, InstructionArg *args);
void halt(SM83 *cpu, InstructionArg *args);
void di(SM83 *cpu, InstructionArg *args);
void ei(SM83 *cpu, InstructionArg *args);

void ld_r_rp(SM83 *cpu, InstructionArg *args);

void ld_rr_nn_m0(SM83 *cpu, InstructionArg *args);
void ld_rr_nn_m1(SM83 *cpu, InstructionArg *args);
void ld_rr_nn_m2(SM83 *cpu, InstructionArg *args);

void ld_bc_a_m0(SM83 *cpu, InstructionArg *args);
void ld_bc_a_m1(SM83 *cpu, InstructionArg *args);
void ld_de_a_m0(SM83 *cpu, InstructionArg *args);
void ld_de_a_m1(SM83 *cpu, InstructionArg *args);
void ldi_hl_a_m0(SM83 *cpu, InstructionArg *args);
void ldi_hl_a_m1(SM83 *cpu, InstructionArg *args);
void ldd_hl_a_m0(SM83 *cpu, InstructionArg *args);
void ldd_hl_a_m1(SM83 *cpu, InstructionArg *args);

void ldi_a_hl_m0(SM83 *cpu, InstructionArg *args);
void ldi_a_hl_m1(SM83 *cpu, InstructionArg *args);

void ldd_a_hl_m0(SM83 *cpu, InstructionArg *args);
void ldd_a_hl_m1(SM83 *cpu, InstructionArg *args);

void ld_nn_sp_m0(SM83 *cpu, InstructionArg *args);
void ld_nn_sp_m1(SM83 *cpu, InstructionArg *args);
void ld_nn_sp_m2(SM83 *cpu, InstructionArg *args);
void ld_nn_sp_m3(SM83 *cpu, InstructionArg *args);
void ld_nn_sp_m4(SM83 *cpu, InstructionArg *args);

void ld_sp_hl_m0(SM83 *cpu, InstructionArg *args);
void ld_sp_hl_m1(SM83 *cpu, InstructionArg *args);

void ld_hl_spe_m0(SM83 *cpu, InstructionArg *args);
void ld_hl_spe_m1(SM83 *cpu, InstructionArg *args);
void ld_hl_spe_m2(SM83 *cpu, InstructionArg *args);

void ld_nn_a_m0(SM83 *cpu, InstructionArg *args);
void ld_nn_a_m1(SM83 *cpu, InstructionArg *args);
void ld_nn_a_m2(SM83 *cpu, InstructionArg *args);
void ld_nn_a_m3(SM83 *cpu, InstructionArg *args);

void ld_a_bc_m0(SM83 *cpu, InstructionArg *args);
void ld_a_bc_m1(SM83 *cpu, InstructionArg *args);

void ld_a_de_m0(SM83 *cpu, InstructionArg *args);
void ld_a_de_m1(SM83 *cpu, InstructionArg *args);

void ld_a_nn_m0(SM83 *cpu, InstructionArg *args);
void ld_a_nn_m1(SM83 *cpu, InstructionArg *args);
void ld_a_nn_m2(SM83 *cpu, InstructionArg *args);
void ld_a_nn_m3(SM83 *cpu, InstructionArg *args);

void ld_r_n_m0(SM83 *cpu, InstructionArg *args);
void ld_r_n_m1(SM83 *cpu, InstructionArg *args);

void ld_r_hl_m0(SM83 *cpu, InstructionArg *args);
void ld_r_hl_m1(SM83 *cpu, InstructionArg *args);

void ld_hl_r_m0(SM83 *cpu, InstructionArg *args);
void ld_hl_r_m1(SM83 *cpu, InstructionArg *args);

void ld_hl_n_m0(SM83 *cpu, InstructionArg *args);
void ld_hl_n_m1(SM83 *cpu, InstructionArg *args);
void ld_hl_n_m2(SM83 *cpu, InstructionArg *args);

void ldh_n_a_m0(SM83 *cpu, InstructionArg *args);
void ldh_n_a_m1(SM83 *cpu, InstructionArg *args);
void ldh_n_a_m2(SM83 *cpu, InstructionArg *args);

void ldh_a_n_m0(SM83 *cpu, InstructionArg *args);
void ldh_a_n_m1(SM83 *cpu, InstructionArg *args);
void ldh_a_n_m2(SM83 *cpu, InstructionArg *args);

void ldh_c_a_m0(SM83 *cpu, InstructionArg *args);
void ldh_c_a_m1(SM83 *cpu, InstructionArg *args);

void ldh_a_c_m0(SM83 *cpu, InstructionArg *args);
void ldh_a_c_m1(SM83 *cpu, InstructionArg *args);

void inc_rr_m0(SM83 *cpu, InstructionArg *args);
void inc_rr_m1(SM83 *cpu, InstructionArg *args);

void inc_r(SM83 *cpu, InstructionArg *args);

void inc_hl_m0(SM83 *cpu, InstructionArg *args);
void inc_hl_m1(SM83 *cpu, InstructionArg *args);
void inc_hl_m2(SM83 *cpu, InstructionArg *args);

void dec_rr_m0(SM83 *cpu, InstructionArg *args);
void dec_rr_m1(SM83 *cpu, InstructionArg *args);

void dec_r(SM83 *cpu, InstructionArg *args);

void dec_hl_m0(SM83 *cpu, InstructionArg *args);
void dec_hl_m1(SM83 *cpu, InstructionArg *args);
void dec_hl_m2(SM83 *cpu, InstructionArg *args);

void add_r(SM83 *cpu, InstructionArg *args);

void add_hl_m0(SM83 *cpu, InstructionArg *args);
void add_hl_m1(SM83 *cpu, InstructionArg *args);

void add_hl_rr_m0(SM83 *cpu, InstructionArg *args);
void add_hl_rr_m1(SM83 *cpu, InstructionArg *args);

void add_n_m0(SM83 *cpu, InstructionArg *args);
void add_n_m1(SM83 *cpu, InstructionArg *args);

void add_sp_e_m0(SM83 *cpu, InstructionArg *args);
void add_sp_e_m1(SM83 *cpu, InstructionArg *args);
void add_sp_e_m2(SM83 *cpu, InstructionArg *args);
void add_sp_e_m3(SM83 *cpu, InstructionArg *args);

void adc_r(SM83 *cpu, InstructionArg *args);

void adc_hl_m0(SM83 *cpu, InstructionArg *args);
void adc_hl_m1(SM83 *cpu, InstructionArg *args);

void adc_n_m0(SM83 *cpu, InstructionArg *args);
void adc_n_m1(SM83 *cpu, InstructionArg *args);

void sub_r(SM83 *cpu, InstructionArg *args);

void sub_hl_m0(SM83 *cpu, InstructionArg *args);
void sub_hl_m1(SM83 *cpu, InstructionArg *args);

void sub_n_m0(SM83 *cpu, InstructionArg *args);
void sub_n_m1(SM83 *cpu, InstructionArg *args);

void sbc_r(SM83 *cpu, InstructionArg *args);

void sbc_hl_m0(SM83 *cpu, InstructionArg *args);
void sbc_hl_m1(SM83 *cpu, InstructionArg *args);

void sbc_n_m0(SM83 *cpu, InstructionArg *args);
void sbc_n_m1(SM83 *cpu, InstructionArg *args);

void and_r(SM83 *cpu, InstructionArg *args);

void and_hl_m0(SM83 *cpu, InstructionArg *args);
void and_hl_m1(SM83 *cpu, InstructionArg *args);

void and_n_m0(SM83 *cpu, InstructionArg *args);
void and_n_m1(SM83 *cpu, InstructionArg *args);

void or_r(SM83 *cpu, InstructionArg *args);

void or_hl_m0(SM83 *cpu, InstructionArg *args);
void or_hl_m1(SM83 *cpu, InstructionArg *args);

void or_n_m0(SM83 *cpu, InstructionArg *args);
void or_n_m1(SM83 *cpu, InstructionArg *args);

void xor_r(SM83 *cpu, InstructionArg *args);

void xor_hl_m0(SM83 *cpu, InstructionArg *args);
void xor_hl_m1(SM83 *cpu, InstructionArg *args);

void xor_n_m0(SM83 *cpu, InstructionArg *args);
void xor_n_m1(SM83 *cpu, InstructionArg *args);

void cp_r(SM83 *cpu, InstructionArg *args);

void cp_hl_m0(SM83 *cpu, InstructionArg *args);
void cp_hl_m1(SM83 *cpu, InstructionArg *args);

void cp_n_m0(SM83 *cpu, InstructionArg *args);
void cp_n_m1(SM83 *cpu, InstructionArg *args);

// Rotate, shift, and bit operations
void rla(SM83 *cpu, InstructionArg *args);
void rra(SM83 *cpu, InstructionArg *args);
void rrca(SM83 *cpu, InstructionArg *args);
void rlca(SM83 *cpu, InstructionArg *args);
void cpl(SM83 *cpu, InstructionArg *args);
void scf(SM83 *cpu, InstructionArg *args);
void ccf(SM83 *cpu, InstructionArg *args);
void daa(SM83 *cpu, InstructionArg *args);
void cb_fetch(SM83 *cpu, InstructionArg *args);

void jp_nn_m0(SM83 *cpu, InstructionArg *args);
void jp_nn_m1(SM83 *cpu, InstructionArg *args);
void jp_nn_m2(SM83 *cpu, InstructionArg *args);
void jp_nn_m3(SM83 *cpu, InstructionArg *args);

void jp_cc_nn_m0(SM83 *cpu, InstructionArg *args);
void jp_cc_nn_m1(SM83 *cpu, InstructionArg *args);
void jp_cc_nn_m2(SM83 *cpu, InstructionArg *args);
void jp_cc_nn_m3(SM83 *cpu, InstructionArg *args);

void jp_ncc_nn_m0(SM83 *cpu, InstructionArg *args);
void jp_ncc_nn_m1(SM83 *cpu, InstructionArg *args);
void jp_ncc_nn_m2(SM83 *cpu, InstructionArg *args);
void jp_ncc_nn_m3(SM83 *cpu, InstructionArg *args);

void jp_hl(SM83 *cpu, InstructionArg *args);

void jr_e_m0(SM83 *cpu, InstructionArg *args);
void jr_e_m1(SM83 *cpu, InstructionArg *args);
void jr_e_m2(SM83 *cpu, InstructionArg *args);

void jr_cc_e_m0(SM83 *cpu, InstructionArg *args);
void jr_cc_e_m1(SM83 *cpu, InstructionArg *args);
void jr_cc_e_m2(SM83 *cpu, InstructionArg *args);

void jr_ncc_e_m0(SM83 *cpu, InstructionArg *args);
void jr_ncc_e_m1(SM83 *cpu, InstructionArg *args);
void jr_ncc_e_m2(SM83 *cpu, InstructionArg *args);

void call_nn_m0(SM83 *cpu, InstructionArg *args);
void call_nn_m1(SM83 *cpu, InstructionArg *args);
void call_nn_m2(SM83 *cpu, InstructionArg *args);
void call_nn_m3(SM83 *cpu, InstructionArg *args);
void call_nn_m4(SM83 *cpu, InstructionArg *args);
void call_nn_m5(SM83 *cpu, InstructionArg *args);

void call_cc_nn_m0(SM83 *cpu, InstructionArg *args);
void call_cc_nn_m1(SM83 *cpu, InstructionArg *args);
void call_cc_nn_m2(SM83 *cpu, InstructionArg *args);
void call_cc_nn_m3(SM83 *cpu, InstructionArg *args);
void call_cc_nn_m4(SM83 *cpu, InstructionArg *args);
void call_cc_nn_m5(SM83 *cpu, InstructionArg *args);

void call_ncc_nn_m0(SM83 *cpu, InstructionArg *args);
void call_ncc_nn_m1(SM83 *cpu, InstructionArg *args);
void call_ncc_nn_m2(SM83 *cpu, InstructionArg *args);
void call_ncc_nn_m3(SM83 *cpu, InstructionArg *args);
void call_ncc_nn_m4(SM83 *cpu, InstructionArg *args);
void call_ncc_nn_m5(SM83 *cpu, InstructionArg *args);

// Returns
void ret_m0(SM83 *cpu, InstructionArg *args);
void ret_m1(SM83 *cpu, InstructionArg *args);
void ret_m2(SM83 *cpu, InstructionArg *args);
void ret_m3(SM83 *cpu, InstructionArg *args);

void ret_cc_m0(SM83 *cpu, InstructionArg *args);
void ret_cc_m1(SM83 *cpu, InstructionArg *args);
void ret_cc_m2(SM83 *cpu, InstructionArg *args);
void ret_cc_m3(SM83 *cpu, InstructionArg *args);
void ret_cc_m4(SM83 *cpu, InstructionArg *args);

void ret_ncc_m0(SM83 *cpu, InstructionArg *args);
void ret_ncc_m1(SM83 *cpu, InstructionArg *args);
void ret_ncc_m2(SM83 *cpu, InstructionArg *args);
void ret_ncc_m3(SM83 *cpu, InstructionArg *args);
void ret_ncc_m4(SM83 *cpu, InstructionArg *args);

void reti_m0(SM83 *cpu, InstructionArg *args);
void reti_m1(SM83 *cpu, InstructionArg *args);
void reti_m2(SM83 *cpu, InstructionArg *args);
void reti_m3(SM83 *cpu, InstructionArg *args);

void push_rr_m0(SM83 *cpu, InstructionArg *args);
void push_rr_m1(SM83 *cpu, InstructionArg *args);
void push_rr_m2(SM83 *cpu, InstructionArg *args);
void push_rr_m3(SM83 *cpu, InstructionArg *args);

void pop_rr_m0(SM83 *cpu, InstructionArg *args);
void pop_rr_m1(SM83 *cpu, InstructionArg *args);
void pop_rr_m2(SM83 *cpu, InstructionArg *args);

void rst_n_m0(SM83 *cpu, InstructionArg *args);
void rst_n_m1(SM83 *cpu, InstructionArg *args);
void rst_n_m2(SM83 *cpu, InstructionArg *args);
void rst_n_m3(SM83 *cpu, InstructionArg *args);

// CB-prefixed operations
void rlc_r(SM83 *cpu, InstructionArg *args);

void rlc_hl_m0(SM83 *cpu, InstructionArg *args);
void rlc_hl_m1(SM83 *cpu, InstructionArg *args);
void rlc_hl_m2(SM83 *cpu, InstructionArg *args);

void rrc_r(SM83 *cpu, InstructionArg *args);

void rrc_hl_m0(SM83 *cpu, InstructionArg *args);
void rrc_hl_m1(SM83 *cpu, InstructionArg *args);
void rrc_hl_m2(SM83 *cpu, InstructionArg *args);

void rl_r(SM83 *cpu, InstructionArg *args);

void rl_hl_m0(SM83 *cpu, InstructionArg *args);
void rl_hl_m1(SM83 *cpu, InstructionArg *args);
void rl_hl_m2(SM83 *cpu, InstructionArg *args);

void rr_r(SM83 *cpu, InstructionArg *args);

void rr_hl_m0(SM83 *cpu, InstructionArg *args);
void rr_hl_m1(SM83 *cpu, InstructionArg *args);
void rr_hl_m2(SM83 *cpu, InstructionArg *args);

// Shift left arithmetic
void sla_r(SM83 *cpu, InstructionArg *args);

void sla_hl_m0(SM83 *cpu, InstructionArg *args);
void sla_hl_m1(SM83 *cpu, InstructionArg *args);
void sla_hl_m2(SM83 *cpu, InstructionArg *args);

// Shift right arithmetic
void sra_r(SM83 *cpu, InstructionArg *args);

void sra_hl_m0(SM83 *cpu, InstructionArg *args);
void sra_hl_m1(SM83 *cpu, InstructionArg *args);
void sra_hl_m2(SM83 *cpu, InstructionArg *args);

// Swap nibbles
void swap_r(SM83 *cpu, InstructionArg *args);

void swap_hl_m0(SM83 *cpu, InstructionArg *args);
void swap_hl_m1(SM83 *cpu, InstructionArg *args);
void swap_hl_m2(SM83 *cpu, InstructionArg *args);

// Shift right logical
void srl_r(SM83 *cpu, InstructionArg *args);

void srl_hl_m0(SM83 *cpu, InstructionArg *args);
void srl_hl_m1(SM83 *cpu, InstructionArg *args);
void srl_hl_m2(SM83 *cpu, InstructionArg *args);

// Test bit
void bit_b_r(SM83 *cpu, InstructionArg *args);

void bit_b_hl_m0(SM83 *cpu, InstructionArg *args);
void bit_b_hl_m1(SM83 *cpu, InstructionArg *args);

// Reset bit
void res_b_r(SM83 *cpu, InstructionArg *args);

void res_b_hl_m0(SM83 *cpu, InstructionArg *args);
void res_b_hl_m1(SM83 *cpu, InstructionArg *args);
void res_b_hl_m2(SM83 *cpu, InstructionArg *args);

// Set bit
void set_b_r(SM83 *cpu, InstructionArg *args);

void set_b_hl_m0(SM83 *cpu, InstructionArg *args);
void set_b_hl_m1(SM83 *cpu, InstructionArg *args);
void set_b_hl_m2(SM83 *cpu, InstructionArg *args);

void isr_m0(SM83 *cpu, InstructionArg *args);
void isr_m1(SM83 *cpu, InstructionArg *args);
void isr_m2(SM83 *cpu, InstructionArg *args);
void isr_m3(SM83 *cpu, InstructionArg *args);
void isr_m4(SM83 *cpu, InstructionArg *args);
#endif
