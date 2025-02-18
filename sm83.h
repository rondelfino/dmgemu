#ifndef SM83_H
#define SM83_H

#include "common.h"
#include "mmu.h"

#define BYTE_REG_COUNT 8
#define WORD_REG_COUNT 5

#define FLAG_C 0x10
#define FLAG_H 0x20
#define FLAG_N 0x40
#define FLAG_Z 0x80

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
        u16 word_regs[BYTE_REG_COUNT];
        u8 byte_regs[WORD_REG_COUNT + 2]; // 2 padding bytes for sp
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
    Pending,
    Enabled
};

struct IDU
{
};

struct Interrupts
{
};

struct Instruction;

struct SM83
{
    Register reg;
    IDU idu;
    MMU *memory;
    Interrupts *interrupts;

    Instruction *instructions;

    struct IO
    {
        enum State
        {
            Idle,
            Read,
            Write,
        };

        State state;
        u8 data;
    } io;

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

    IMEState ime;
};

// enum ArgType
// {
//     ARG_NONE,
//     ARG_REG8,
//     ARG_REG16,
//     ARG_DATA,
// };

// struct Arg
// {
//     ArgType type;
//     union
//     {
//         Reg8 reg8;
//         Reg16 reg16;
//         u8 data;
//     } arg;
// };
//

enum Reg8
{
    REG8_F,
    REG8_A,
    REG8_C,
    REG8_B,
    REG8_E,
    REG8_D,
    REG8_L,
    REG8_H,
};

enum Reg16
{
    REG16_AF,
    REG16_BC,
    REG16_DE,
    REG16_HL,
    REG16_SP,
};

struct Instruction
{
    u8 value;
    std::string mnemonic;
    std::vector<void (*)(SM83 *, Instruction *)> handlers;
    union
    {
        Reg8 reg8;
        Reg16 reg16;
        u8 data;
    } args[2];
    // Arg args[2];
    u8 length;              // in bytes
    std::vector<u8> cycles; // if applicable, 0th duration is when an action is taken
    u8 flags;
    u8 microop_index;
};

extern Instruction instructions[];
extern Instruction cb_instructions[];

void sm83_reset(SM83 *cpu);
void fetch(SM83 *cpu);

// static u8 rotate_left(SM83 *cpu, u8 value);
// static u8 rotate_left_carry(SM83 *cpu, u8 value);
// static u8 rotate_right(SM83 *cpu, u8 value);
// static u8 rotate_right_carry(SM83 *cpu, u8 value);

// static u8 sla(SM83 *cpu, u8 value);
// static u8 sra(SM83 *cpu, u8 value);
// static u8 srl(SM83 *cpu, u8 value);
// static u8 swap(SM83 *cpu, u8 value);

static inline u8 test_bit(u8 reg, u8 bit)
{
    return 1;
}

static inline u8 reset_bit(u8 reg, u8 bit)
{
    reg &= ~bit;
    return reg;
}

static inline u8 set_bit(u8 reg, u8 bit)
{
    reg |= bit;
    return reg;
}

void sm83_tick_t1(SM83 *cpu);
void sm83_tick_t2(SM83 *cpu);
void sm83_tick_t3(SM83 *cpu);
void sm83_tick_t4(SM83 *cpu);
void sm83_tick(SM83 *cpu);

void idu_increment(u16 *data);
void idu_decrement(u16 *data);

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

void unimplemented_instruction(SM83 *cpu, Instruction *instr);
void nop(SM83 *cpu, Instruction *instr);
void stop(SM83 *cpu, Instruction *instr);
void halt(SM83 *cpu, Instruction *instr);
void di(SM83 *cpu, Instruction *instr);
void ei(SM83 *cpu, Instruction *instr);

void ld_r_rp(SM83 *cpu, Instruction *instr);

void ld_rr_nn_m1(SM83 *cpu, Instruction *instr);
void ld_rr_nn_m2(SM83 *cpu, Instruction *instr);
void ld_rr_nn_m3(SM83 *cpu, Instruction *instr);

void ld_bc_a(SM83 *cpu, Instruction *instr);
void ld_de_a(SM83 *cpu, Instruction *instr);
void ldi_hl_a(SM83 *cpu, Instruction *instr);
void ldd_hl_a(SM83 *cpu, Instruction *instr);

void ldi_a_hl_m1(SM83 *cpu, Instruction *instr);
void ldi_a_hl_m2(SM83 *cpu, Instruction *instr);

void ldd_a_hl_m1(SM83 *cpu, Instruction *instr);
void ldd_a_hl_m2(SM83 *cpu, Instruction *instr);

void ld_nn_sp_m1(SM83 *cpu, Instruction *instr);
void ld_nn_sp_m2(SM83 *cpu, Instruction *instr);
void ld_nn_sp_m3(SM83 *cpu, Instruction *instr);
void ld_nn_sp_m4(SM83 *cpu, Instruction *instr);
void ld_nn_sp_m5(SM83 *cpu, Instruction *instr);

void ld_sp_hl(SM83 *cpu, Instruction *instr);

void ld_hl_spe_m1(SM83 *cpu, Instruction *instr);
void ld_hl_spe_m2(SM83 *cpu, Instruction *instr);
void ld_hl_spe_m3(SM83 *cpu, Instruction *instr);

void ld_nn_a_m1(SM83 *cpu, Instruction *instr);
void ld_nn_a_m2(SM83 *cpu, Instruction *instr);
void ld_nn_a_m3(SM83 *cpu, Instruction *instr);

void ld_a_bc_m1(SM83 *cpu, Instruction *instr);
void ld_a_bc_m2(SM83 *cpu, Instruction *instr);

void ld_a_de_m1(SM83 *cpu, Instruction *instr);
void ld_a_de_m2(SM83 *cpu, Instruction *instr);

void ld_a_nn_m1(SM83 *cpu, Instruction *instr);
void ld_a_nn_m2(SM83 *cpu, Instruction *instr);
void ld_a_nn_m3(SM83 *cpu, Instruction *instr);
void ld_a_nn_m4(SM83 *cpu, Instruction *instr);

void ld_r_n_m1(SM83 *cpu, Instruction *instr);
void ld_r_n_m2(SM83 *cpu, Instruction *instr);

void ld_r_hl_m1(SM83 *cpu, Instruction *instr);
void ld_r_hl_m2(SM83 *cpu, Instruction *instr);

void ld_hl_r_m1(SM83 *cpu, Instruction *instr);
void ld_hl_r_m2(SM83 *cpu, Instruction *instr);

void ld_hl_n_m1(SM83 *cpu, Instruction *instr);
void ld_hl_n_m2(SM83 *cpu, Instruction *instr);
void ld_hl_n_m3(SM83 *cpu, Instruction *instr);

void ldh_n_a_m1(SM83 *cpu, Instruction *instr);
void ldh_n_a_m2(SM83 *cpu, Instruction *instr);
void ldh_n_a_m3(SM83 *cpu, Instruction *instr);

void ldh_a_n_m1(SM83 *cpu, Instruction *instr);
void ldh_a_n_m2(SM83 *cpu, Instruction *instr);
void ldh_a_n_m3(SM83 *cpu, Instruction *instr);

void ldh_c_a_m1(SM83 *cpu, Instruction *instr);
void ldh_c_a_m2(SM83 *cpu, Instruction *instr);

void ldh_a_c_m1(SM83 *cpu, Instruction *instr);
void ldh_a_c_m2(SM83 *cpu, Instruction *instr);

void inc_rr_m1(SM83 *cpu, Instruction *instr);
void inc_rr_m2(SM83 *cpu, Instruction *instr);

void inc_r(SM83 *cpu, Instruction *instr);

void inc_hl_m1(SM83 *cpu, Instruction *instr);
void inc_hl_m2(SM83 *cpu, Instruction *instr);
void inc_hl_m3(SM83 *cpu, Instruction *instr);

void dec_rr_m1(SM83 *cpu, Instruction *instr);
void dec_rr_m2(SM83 *cpu, Instruction *instr);

void dec_r(SM83 *cpu, Instruction *instr);

void dec_hl_m1(SM83 *cpu, Instruction *instr);
void dec_hl_m2(SM83 *cpu, Instruction *instr);
void dec_hl_m3(SM83 *cpu, Instruction *instr);

void add_r(SM83 *cpu, Instruction *instr);

void add_hl_m1(SM83 *cpu, Instruction *instr);
void add_hl_m2(SM83 *cpu, Instruction *instr);

void add_hl_rr_m1(SM83 *cpu, Instruction *instr);
void add_hl_rr_m2(SM83 *cpu, Instruction *instr);

void add_n_m1(SM83 *cpu, Instruction *instr);
void add_n_m2(SM83 *cpu, Instruction *instr);

void add_sp_e_m1(SM83 *cpu, Instruction *instr);
void add_sp_e_m2(SM83 *cpu, Instruction *instr);
void add_sp_e_m3(SM83 *cpu, Instruction *instr);
void add_sp_e_m4(SM83 *cpu, Instruction *instr);

void adc_r(SM83 *cpu, Instruction *instr);

void adc_hl_m1(SM83 *cpu, Instruction *instr);
void adc_hl_m2(SM83 *cpu, Instruction *instr);

void adc_n_m1(SM83 *cpu, Instruction *instr);
void adc_n_m2(SM83 *cpu, Instruction *instr);

void sub_r(SM83 *cpu, Instruction *instr);

void sub_hl_m1(SM83 *cpu, Instruction *instr);
void sub_hl_m2(SM83 *cpu, Instruction *instr);

void sub_n_m1(SM83 *cpu, Instruction *instr);
void sub_n_m2(SM83 *cpu, Instruction *instr);

void sbc_r(SM83 *cpu, Instruction *instr);

void sbc_hl_m1(SM83 *cpu, Instruction *instr);
void sbc_hl_m2(SM83 *cpu, Instruction *instr);

void sbc_n_m1(SM83 *cpu, Instruction *instr);
void sbc_n_m2(SM83 *cpu, Instruction *instr);

void and_r(SM83 *cpu, Instruction *instr);

void and_hl_m1(SM83 *cpu, Instruction *instr);
void and_hl_m2(SM83 *cpu, Instruction *instr);

void and_n_m1(SM83 *cpu, Instruction *instr);
void and_n_m2(SM83 *cpu, Instruction *instr);

void or_r(SM83 *cpu, Instruction *instr);

void or_hl_m1(SM83 *cpu, Instruction *instr);
void or_hl_m2(SM83 *cpu, Instruction *instr);

void or_n_m1(SM83 *cpu, Instruction *instr);
void or_n_m2(SM83 *cpu, Instruction *instr);

void xor_r(SM83 *cpu, Instruction *instr);

void xor_hl_m1(SM83 *cpu, Instruction *instr);
void xor_hl_m2(SM83 *cpu, Instruction *instr);

void xor_n_m1(SM83 *cpu, Instruction *instr);
void xor_n_m2(SM83 *cpu, Instruction *instr);

void cp_r(SM83 *cpu, Instruction *instr);

void cp_hl_m1(SM83 *cpu, Instruction *instr);
void cp_hl_m2(SM83 *cpu, Instruction *instr);

void cp_n_m1(SM83 *cpu, Instruction *instr);
void cp_n_m2(SM83 *cpu, Instruction *instr);

// Rotate, shift, and bit operations
void rla(SM83 *cpu, Instruction *instr);
void rra(SM83 *cpu, Instruction *instr);
void rrca(SM83 *cpu, Instruction *instr);
void rlca(SM83 *cpu, Instruction *instr);
void cpl(SM83 *cpu, Instruction *instr);
void scf(SM83 *cpu, Instruction *instr);
void ccf(SM83 *cpu, Instruction *instr);
void daa(SM83 *cpu, Instruction *instr);
void cb_fetch(SM83 *cpu, Instruction *instr);

void jp_nn_m1(SM83 *cpu, Instruction *instr);
void jp_nn_m2(SM83 *cpu, Instruction *instr);
void jp_nn_m3(SM83 *cpu, Instruction *instr);
void jp_nn_m4(SM83 *cpu, Instruction *instr);

void jp_cc_nn_m1(SM83 *cpu, Instruction *instr);
void jp_cc_nn_m2(SM83 *cpu, Instruction *instr);
void jp_cc_nn_m3(SM83 *cpu, Instruction *instr);
void jp_cc_nn_m4(SM83 *cpu, Instruction *instr);

void jp_ncc_nn_m1(SM83 *cpu, Instruction *instr);
void jp_ncc_nn_m2(SM83 *cpu, Instruction *instr);
void jp_ncc_nn_m3(SM83 *cpu, Instruction *instr);
void jp_ncc_nn_m4(SM83 *cpu, Instruction *instr);

void jp_hl(SM83 *cpu, Instruction *instr);

void jr_e_m1(SM83 *cpu, Instruction *instr);
void jr_e_m2(SM83 *cpu, Instruction *instr);
void jr_e_m3(SM83 *cpu, Instruction *instr);

void jr_cc_e_m1(SM83 *cpu, Instruction *instr);
void jr_cc_e_m2(SM83 *cpu, Instruction *instr);
void jr_cc_e_m3(SM83 *cpu, Instruction *instr);

void jr_ncc_e_m1(SM83 *cpu, Instruction *instr);
void jr_ncc_e_m2(SM83 *cpu, Instruction *instr);
void jr_ncc_e_m3(SM83 *cpu, Instruction *instr);

void call_nn_m1(SM83 *cpu, Instruction *instr);
void call_nn_m2(SM83 *cpu, Instruction *instr);
void call_nn_m3(SM83 *cpu, Instruction *instr);
void call_nn_m4(SM83 *cpu, Instruction *instr);
void call_nn_m5(SM83 *cpu, Instruction *instr);
void call_nn_m6(SM83 *cpu, Instruction *instr);

void call_cc_nn_m1(SM83 *cpu, Instruction *instr);
void call_cc_nn_m2(SM83 *cpu, Instruction *instr);
void call_cc_nn_m3(SM83 *cpu, Instruction *instr);
void call_cc_nn_m4(SM83 *cpu, Instruction *instr);
void call_cc_nn_m5(SM83 *cpu, Instruction *instr);
void call_cc_nn_m6(SM83 *cpu, Instruction *instr);

void call_ncc_nn_m1(SM83 *cpu, Instruction *instr);
void call_ncc_nn_m2(SM83 *cpu, Instruction *instr);
void call_ncc_nn_m3(SM83 *cpu, Instruction *instr);
void call_ncc_nn_m4(SM83 *cpu, Instruction *instr);
void call_ncc_nn_m5(SM83 *cpu, Instruction *instr);
void call_ncc_nn_m6(SM83 *cpu, Instruction *instr);

// Returns
void ret_m1(SM83 *cpu, Instruction *instr);
void ret_m2(SM83 *cpu, Instruction *instr);
void ret_m3(SM83 *cpu, Instruction *instr);
void ret_m4(SM83 *cpu, Instruction *instr);

void ret_cc_m1(SM83 *cpu, Instruction *instr);
void ret_cc_m2(SM83 *cpu, Instruction *instr);
void ret_cc_m3(SM83 *cpu, Instruction *instr);
void ret_cc_m4(SM83 *cpu, Instruction *instr);
void ret_cc_m5(SM83 *cpu, Instruction *instr);

void ret_ncc_m1(SM83 *cpu, Instruction *instr);
void ret_ncc_m2(SM83 *cpu, Instruction *instr);
void ret_ncc_m3(SM83 *cpu, Instruction *instr);
void ret_ncc_m4(SM83 *cpu, Instruction *instr);
void ret_ncc_m5(SM83 *cpu, Instruction *instr);

void reti_m1(SM83 *cpu, Instruction *instr);
void reti_m2(SM83 *cpu, Instruction *instr);
void reti_m3(SM83 *cpu, Instruction *instr);
void reti_m4(SM83 *cpu, Instruction *instr);

void push_rr_m1(SM83 *cpu, Instruction *instr);
void push_rr_m2(SM83 *cpu, Instruction *instr);
void push_rr_m3(SM83 *cpu, Instruction *instr);
void push_rr_m4(SM83 *cpu, Instruction *instr);

void pop_rr_m1(SM83 *cpu, Instruction *instr);
void pop_rr_m2(SM83 *cpu, Instruction *instr);
void pop_rr_m3(SM83 *cpu, Instruction *instr);

void rst_n_m1(SM83 *cpu, Instruction *instr);
void rst_n_m2(SM83 *cpu, Instruction *instr);
void rst_n_m3(SM83 *cpu, Instruction *instr);
void rst_n_m4(SM83 *cpu, Instruction *instr);

// CB-prefixed operations
void rlc_r(SM83 *cpu, Instruction *instr);

void rlc_hl_m1(SM83 *cpu, Instruction *instr);
void rlc_hl_m2(SM83 *cpu, Instruction *instr);
void rlc_hl_m3(SM83 *cpu, Instruction *instr);

void rrc_r(SM83 *cpu, Instruction *instr);

void rrc_hl_m1(SM83 *cpu, Instruction *instr);
void rrc_hl_m2(SM83 *cpu, Instruction *instr);
void rrc_hl_m3(SM83 *cpu, Instruction *instr);

void rl_r(SM83 *cpu, Instruction *instr);

void rl_hl_m1(SM83 *cpu, Instruction *instr);
void rl_hl_m2(SM83 *cpu, Instruction *instr);
void rl_hl_m3(SM83 *cpu, Instruction *instr);

void rr_r(SM83 *cpu, Instruction *instr);

void rr_hl_m1(SM83 *cpu, Instruction *instr);
void rr_hl_m2(SM83 *cpu, Instruction *instr);
void rr_hl_m3(SM83 *cpu, Instruction *instr);

// Shift left arithmetic
void sla_r(SM83 *cpu, Instruction *instr);

void sla_hl_m1(SM83 *cpu, Instruction *instr);
void sla_hl_m2(SM83 *cpu, Instruction *instr);
void sla_hl_m3(SM83 *cpu, Instruction *instr);

// Shift right arithmetic
void sra_r(SM83 *cpu, Instruction *instr);

void sra_hl_m1(SM83 *cpu, Instruction *instr);
void sra_hl_m2(SM83 *cpu, Instruction *instr);
void sra_hl_m3(SM83 *cpu, Instruction *instr);

// Swap nibbles
void swap_r(SM83 *cpu, Instruction *instr);

void swap_hl_m1(SM83 *cpu, Instruction *instr);
void swap_hl_m2(SM83 *cpu, Instruction *instr);
void swap_hl_m3(SM83 *cpu, Instruction *instr);

// Shift right logical
void srl_r(SM83 *cpu, Instruction *instr);

void srl_hl_m1(SM83 *cpu, Instruction *instr);
void srl_hl_m2(SM83 *cpu, Instruction *instr);
void srl_hl_m3(SM83 *cpu, Instruction *instr);

// Test bit
void bit_b_r(SM83 *cpu, Instruction *instr);

void bit_b_hl_m1(SM83 *cpu, Instruction *instr);
void bit_b_hl_m2(SM83 *cpu, Instruction *instr);

// Reset bit
void res_b_r(SM83 *cpu, Instruction *instr);

void res_b_hl_m1(SM83 *cpu, Instruction *instr);
void res_b_hl_m2(SM83 *cpu, Instruction *instr);
void res_b_hl_m3(SM83 *cpu, Instruction *instr);

// Set bit
void set_b_r(SM83 *cpu, Instruction *instr);

void set_b_hl_m1(SM83 *cpu, Instruction *instr);
void set_b_hl_m2(SM83 *cpu, Instruction *instr);
void set_b_hl_m3(SM83 *cpu, Instruction *instr);

#endif
