#include "sm83.h"

void log_cpu_state(SM83 *cpu)
{
    char buffer[256];
    sprintf(buffer,
            "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
            cpu->reg.a, cpu->reg.f, cpu->reg.b, cpu->reg.c, cpu->reg.d, cpu->reg.e, cpu->reg.h, cpu->reg.l, cpu->reg.sp,
            cpu->reg.pc, cpu->memory->memory_map[cpu->reg.pc], cpu->memory->memory_map[cpu->reg.pc + 1],
            cpu->memory->memory_map[cpu->reg.pc + 2], cpu->memory->memory_map[cpu->reg.pc + 3]);

    FILE *log = fopen("cpu_log.txt", "a");
    if (!log)
    {
    }
    fputs(buffer, log);

    fclose(log);
}

void sm83_reset(SM83 *cpu)
{
    *cpu = {};
}
// TODO: Instructions increment pc
void fetch(SM83 *cpu)
{
    cpu->instructions = instructions;

    cpu->instructions[cpu->reg.ir].microop_index = 0;
    cpu->reg.ir = mmu_read_byte(cpu->memory, cpu->reg.pc, false);

    log_cpu_state(cpu);

    idu_increment(&cpu->reg.pc);
}

static u8 rotate_left(SM83 *cpu, u8 value)
{
    u8 old_carry = cpu->reg.flags.c;

    u8 new_carry = (value & 0x80) ? 1 : 0;
    value = (value << 1) | old_carry;

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = new_carry;

    return value;
}

static u8 rotate_left_carry(SM83 *cpu, u8 value)
{
    u8 carry_flag = (value & 0x80) ? 1 : 0;
    value = (value << 1) | carry_flag;

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 rotate_right(SM83 *cpu, u8 value)
{
    u8 old_carry = cpu->reg.flags.c;
    cpu->reg.f = 0;

    u8 carry_flag = (value & 0x01) ? 1 : 0;
    value = (value >> 1) + (old_carry << 7);

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 rotate_right_carry(SM83 *cpu, u8 value)
{
    cpu->reg.f = 0;

    u8 carry_flag = (value & 0x01) ? 1 : 0;
    value = (value >> 1) + (carry_flag << 7);

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 sla(SM83 *cpu, u8 value)
{
    u8 carry_flag = (value & 0x80) ? 1 : 0;
    cpu->reg.f = 0;
    value <<= 1;

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 sra(SM83 *cpu, u8 value)
{
    u8 carry_flag = (value & 0x01) ? 1 : 0;
    cpu->reg.f = 0;
    u8 sign_bit = (value & 0x80) ? 1 : 0;
    value >>= 1;
    value |= (sign_bit << 7);

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 srl(SM83 *cpu, u8 value)
{
    u8 carry_flag = (value & 0x01) ? 1 : 0;
    cpu->reg.f = 0;
    value >>= 1;

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 swap(SM83 *cpu, u8 value)
{
    cpu->reg.f = 0;
    u8 temp = value >> 4;
    value = (value << 4) | temp;

    cpu->reg.flags.z = (value == 0);

    return value;
}

// u8 cpu_read(SM83 *cpu, u16 address)
// {
//     mmu_read_byte(cpu->memory, address, false);
//     cpu->io.state = cpu->io.Read;
// }

// u8 cpu_write(SM83 *cpu, u8 value)
// {
//     cpu->io.state = cpu->io.Write;
// }

void sm83_tick_t1(SM83 *cpu)
{
    sm83_tick(cpu);
}

void sm83_tick_t2(SM83 *cpu)
{
}

void sm83_tick_t3(SM83 *cpu)
{
}

void sm83_tick_t4(SM83 *cpu)
{
}

void sm83_tick(SM83 *cpu)
{
    if (!cpu->instructions)
    {
        cpu->instructions = instructions;
    }

    u8 op_index = cpu->instructions[cpu->reg.ir].microop_index;
    cpu->instructions[cpu->reg.ir].microop_index++;
    cpu->instructions[cpu->reg.ir].handlers[op_index](cpu, &cpu->instructions[cpu->reg.ir]);
}

static void increment(u16 *data, s8 value)
{
    if (*data >= OAM_START && *data <= 0xFFFF)
    {
        // TODO: Handle OAM bug
    }

    (*data) += value;
}

void idu_increment(u16 *data)
{
    increment(data, 1);
}

void idu_decrement(u16 *data)
{
    increment(data, -1);
}

void unimplemented_instruction(SM83 *cpu, Instruction *instr)
{
    // TODO: Error handling/logging
}

void nop(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void stop(SM83 *cpu, Instruction *instr)
{
}

void halt(SM83 *cpu, Instruction *instr)
{
}

void di(SM83 *cpu, Instruction *instr)
{
    cpu->ime = IMEState::Disabled;
    fetch(cpu);
}

void ei(SM83 *cpu, Instruction *instr)
{
    if (cpu->ime == IMEState::Disabled)
    {
        cpu->ime = IMEState::Requested;
    }
    fetch(cpu);
}

void ld_r_rp(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = cpu->reg.byte_regs[instr->args[1].reg8];
    fetch(cpu);
}

void ld_rr_nn_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void ld_rr_nn_m2(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    cpu->nn = unsigned_16(cpu->msb, cpu->lsb);
    idu_increment(&cpu->reg.pc);
}
void ld_rr_nn_m3(SM83 *cpu, Instruction *instr)
{
    u8 reg_index = instr->args[0].reg16;
    cpu->reg.word_regs[reg_index] = cpu->nn;
    fetch(cpu);
}

void ld_bc_a(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->reg.bc, cpu->reg.a);
    fetch(cpu);
}

void ld_de_a(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->reg.de, cpu->reg.a);
    fetch(cpu);
}

void ldi_hl_a(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->reg.a);
    idu_increment(&cpu->reg.hl);
    fetch(cpu);
}

void ldd_hl_a(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->reg.a);
    idu_decrement(&cpu->reg.hl);
    fetch(cpu);
}

void ldi_a_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.hl);
}
void ldi_a_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = cpu->io.data;
    fetch(cpu);
}

void ldd_a_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
    idu_decrement(&cpu->reg.hl);
}
void ldd_a_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = cpu->io.data;
    fetch(cpu);
}

void ld_nn_sp_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void ld_nn_sp_m2(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    cpu->address = unsigned_16(cpu->msb, cpu->lsb);
    idu_increment(&cpu->reg.pc);
}
void ld_nn_sp_m3(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->address, lsb(cpu->reg.sp));
    idu_increment(&cpu->address);
}
void ld_nn_sp_m4(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->address, msb(cpu->reg.sp));
}
void ld_nn_sp_m5(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void ld_sp_hl(SM83 *cpu, Instruction *instr)
{
    cpu->reg.sp = cpu->reg.hl;
    fetch(cpu);
}

// TODO: Test this
void ld_hl_spe_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void ld_hl_spe_m2(SM83 *cpu, Instruction *instr)
{
    u8 lsb_sp = lsb(cpu->reg.sp);

    u16 result = lsb_sp + cpu->io.data;

    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (lsb_sp & 0xF) + (cpu->io.data & 0xF) > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.l = result;
}
void ld_hl_spe_m3(SM83 *cpu, Instruction *instr)
{
    bool z_sign = get_bit(7, cpu->io.data);
    u8 adj = z_sign ? 0xFF : 0x00;

    u16 result = msb(cpu->reg.sp) + adj + cpu->reg.flags.c;
    cpu->reg.h = result & 0xFF;
    fetch(cpu);
}

void ld_nn_a_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void ld_nn_a_m2(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void ld_nn_a_m3(SM83 *cpu, Instruction *instr)
{
    cpu->address = unsigned_16(cpu->msb, cpu->lsb);
    mmu_write_byte(cpu->memory, cpu->address, cpu->reg.a);
    fetch(cpu);
}

void ld_a_bc_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.bc, false);
    cpu->io.data = z;
}
void ld_a_bc_m2(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = cpu->io.data;
    fetch(cpu);
}

void ld_a_de_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.de, false);
    cpu->io.data = z;
}
void ld_a_de_m2(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = cpu->io.data;
    fetch(cpu);
}

void ld_a_nn_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void ld_a_nn_m2(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void ld_a_nn_m3(SM83 *cpu, Instruction *instr)
{
    cpu->address = unsigned_16(cpu->msb, cpu->lsb);
    cpu->io.data = mmu_read_byte(cpu->memory, cpu->address, cpu->reg.a);
}
void ld_a_nn_m4(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = cpu->io.data;
    fetch(cpu);
}

void ld_r_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void ld_r_n_m2(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = cpu->io.data;
    fetch(cpu);
}

void ld_r_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void ld_r_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = cpu->io.data;
    fetch(cpu);
}

void ld_hl_r_m1(SM83 *cpu, Instruction *instr)
{
    u8 data = cpu->reg.byte_regs[instr->args[0].reg8];
    mmu_write_byte(cpu->memory, cpu->reg.hl, data);
}
void ld_hl_r_m2(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void ld_hl_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void ld_hl_n_m2(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->io.data);
}
void ld_hl_n_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void ldh_n_a_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void ldh_n_a_m2(SM83 *cpu, Instruction *instr)
{
    cpu->address = unsigned_16(0xFF, cpu->lsb);
    mmu_write_byte(cpu->memory, cpu->address, cpu->reg.a);
}
void ldh_n_a_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void ldh_a_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void ldh_a_n_m2(SM83 *cpu, Instruction *instr)
{
    cpu->address = unsigned_16(0xFF, cpu->lsb);
    cpu->io.data = mmu_read_byte(cpu->memory, cpu->address, false);
}
void ldh_a_n_m3(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = cpu->io.data;
    fetch(cpu);
}

void ldh_c_a_m1(SM83 *cpu, Instruction *instr)
{
    cpu->address = unsigned_16(0xFF, lsb(cpu->reg.c));
    mmu_write_byte(cpu->memory, cpu->address, cpu->reg.a);
}
void ldh_c_a_m2(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void ldh_a_c_m1(SM83 *cpu, Instruction *instr)
{
    cpu->address = unsigned_16(0xFF, lsb(cpu->reg.c));
    u8 z = mmu_read_byte(cpu->memory, cpu->address, false);
    cpu->io.data = z;
}
void ldh_a_c_m2(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = cpu->io.data;
    fetch(cpu);
}

void inc_rr_m1(SM83 *cpu, Instruction *instr)
{
    idu_increment(&cpu->reg.word_regs[instr->args[0].reg16]);
}
void inc_rr_m2(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void inc_r(SM83 *cpu, Instruction *instr)
{
    u8 carry_flag = cpu->reg.flags.c;
    cpu->reg.f = 0;

    u8 reg_index = instr->args[0].reg8;
    u8 reg = cpu->reg.byte_regs[reg_index];
    u16 result = reg + 1;
    cpu->reg.byte_regs[reg_index] = result & 0xFF;

    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.h = ((reg & 0xF) + 1) > 0xF;
    // cpu->reg.flags.h = (result & 0xF) == 0;
    cpu->reg.flags.c = carry_flag;
    fetch(cpu);
}

void inc_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void inc_hl_m2(SM83 *cpu, Instruction *instr)
{
    u8 carry_flag = cpu->reg.flags.c;
    cpu->reg.f = 0;

    u8 result = (cpu->io.data + 1) & 0xFF;
    mmu_write_byte(cpu->memory, cpu->reg.hl, result);

    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.h = (result & 0xF) == 0;
    cpu->reg.flags.c = carry_flag;
}
void inc_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void dec_rr_m1(SM83 *cpu, Instruction *instr)
{
    idu_decrement(&cpu->reg.word_regs[instr->args[0].reg16]);
}
void dec_rr_m2(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void dec_r(SM83 *cpu, Instruction *instr)
{
    u8 carry_flag = cpu->reg.flags.c;
    cpu->reg.f = 0;

    u8 reg_index = instr->args[0].reg8;
    u8 reg = cpu->reg.byte_regs[reg_index];
    u16 result = reg - 1;
    cpu->reg.byte_regs[reg_index] = result & 0xFF;

    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    // cpu->reg.flags.h = ((reg & 0xF) - 1) == 0xF;
    cpu->reg.flags.h = (cpu->reg.byte_regs[reg_index] & 0xF) == 0xF;
    cpu->reg.flags.c = carry_flag;
    fetch(cpu);
}

void dec_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void dec_hl_m2(SM83 *cpu, Instruction *instr)
{
    u8 carry_flag = cpu->reg.flags.c;
    cpu->reg.f = 0;

    u8 result = (cpu->io.data - 1) & 0xFF;
    mmu_write_byte(cpu->memory, cpu->reg.hl, result);

    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (result & 0xF) == 0xF;
    cpu->reg.flags.c = carry_flag;
}
void dec_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void add_r(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a + cpu->reg.byte_regs[instr->args[0].reg8];
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->reg.byte_regs[instr->args[0].reg8] & 0xF) > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void add_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void add_hl_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a + cpu->io.data;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->io.data & 0xF) > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

// TODO: Test this
void add_hl_rr_m1(SM83 *cpu, Instruction *instr)
{
    u8 zero_flag = cpu->reg.flags.z;
    cpu->msb = msb(cpu->reg.word_regs[instr->args[0].reg16]);
    cpu->lsb = lsb(cpu->reg.word_regs[instr->args[0].reg16]);

    u16 result = cpu->reg.l + cpu->lsb;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.c = result > 0xFF;
    cpu->reg.flags.h = ((cpu->reg.l & 0xF) + (cpu->lsb & 0xF) > 0xF);
    cpu->reg.flags.z = zero_flag;

    cpu->reg.l = result & 0xFF;
}
void add_hl_rr_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.h + cpu->msb + cpu->reg.flags.c;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = ((cpu->reg.h & 0xF) + (cpu->msb & 0xF) + cpu->reg.flags.c > 0xF);
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.h = result & 0xFF;
    fetch(cpu);
}

void add_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void add_n_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a + cpu->io.data;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->io.data & 0xF) > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

// TODO: Test this
void add_sp_e_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
// void add_sp_e_m2(SM83 *cpu, Instruction *instr)
// {
//     // Get sign of immediate data
//     cpu->b = get_bit(7, cpu->io.data);
//     cpu->lsb = lsb(cpu->reg.sp);

//     u16 result = cpu->lsb + signed_8(cpu->io.data);
//     cpu->reg.flags.z = (result == 0);
//     cpu->reg.flags.n = 0;
//     cpu->reg.flags.h = ((cpu->lsb & 0xF) + (cpu->io.data & 0xF)) > 0xF;
//     cpu->reg.flags.c = result > 0xFF;

//     cpu->io.data = result & 0xFF;
//     cpu->lsb = cpu->io.data;
// }
// void add_sp_e_m3(SM83 *cpu, Instruction *instr)
// {
//     s8 adj = 0;
//     if (cpu->reg.flags.c && !cpu->b)
//     {
//         adj = 1;
//     }
//     else if (!cpu->reg.flags.c && cpu->b)
//     {
//         adj = -1;
//     }

//     u16 result = msb(cpu->reg.sp) + adj + cpu->reg.flags.c;
//     cpu->msb = result & 0xFF;
// }
void add_sp_e_m2(SM83 *cpu, Instruction *instr)
{
    s8 e = signed_8(cpu->io.data);
    u16 result = cpu->reg.sp + e;

    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = ((cpu->reg.sp & 0xF) + (e & 0xF)) > 0xF;
    cpu->reg.flags.c = ((cpu->reg.sp & 0xFF) + (e & 0xFF)) > 0xFF;

    cpu->reg.sp = result;
}
void add_sp_e_m3(SM83 *cpu, Instruction *instr)
{
}
void add_sp_e_m4(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void adc_r(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a + cpu->reg.byte_regs[instr->args[0].reg8] + cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->reg.byte_regs[instr->args[0].reg8] & 0xF) + cpu->reg.flags.c > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void adc_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void adc_hl_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a + cpu->io.data + cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->io.data & 0xF) + cpu->reg.flags.c > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void adc_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void adc_n_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a + cpu->io.data + cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->io.data & 0xF) + cpu->reg.flags.c > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sub_r(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a - cpu->reg.byte_regs[instr->args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->reg.byte_regs[instr->args[0].reg8] & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->reg.byte_regs[instr->args[0].reg8];

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sub_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void sub_hl_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a - cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->io.data & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->io.data;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sub_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void sub_n_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a - cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->io.data & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->io.data;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sbc_r(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a - cpu->reg.byte_regs[instr->args[0].reg8] - cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) - cpu->reg.flags.c < (cpu->reg.byte_regs[instr->args[0].reg8] & 0xF);
    cpu->reg.flags.c = cpu->reg.a - cpu->reg.flags.c < cpu->reg.byte_regs[instr->args[0].reg8];

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sbc_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void sbc_hl_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a - cpu->io.data - cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) - cpu->reg.flags.c < (cpu->io.data & 0xF);
    cpu->reg.flags.c = cpu->reg.a - cpu->reg.flags.c < cpu->io.data;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sbc_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void sbc_n_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a - cpu->io.data - cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) - cpu->reg.flags.c < (cpu->io.data & 0xF);
    cpu->reg.flags.c = cpu->reg.a - cpu->reg.flags.c < cpu->io.data;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void and_r(SM83 *cpu, Instruction *instr)
{
    u8 result = cpu->reg.a & cpu->reg.byte_regs[instr->args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void and_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void and_hl_m2(SM83 *cpu, Instruction *instr)
{
    u8 result = cpu->reg.a & cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void and_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void and_n_m2(SM83 *cpu, Instruction *instr)
{
    u8 result = cpu->reg.a & cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void or_r(SM83 *cpu, Instruction *instr)
{
    u8 result = cpu->reg.a | cpu->reg.byte_regs[instr->args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void or_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void or_hl_m2(SM83 *cpu, Instruction *instr)
{
    u8 result = cpu->reg.a | cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void or_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void or_n_m2(SM83 *cpu, Instruction *instr)
{
    u8 result = cpu->reg.a | cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void xor_r(SM83 *cpu, Instruction *instr)
{
    u8 result = cpu->reg.a ^ cpu->reg.byte_regs[instr->args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void xor_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void xor_hl_m2(SM83 *cpu, Instruction *instr)
{
    u8 result = cpu->reg.a ^ cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void xor_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void xor_n_m2(SM83 *cpu, Instruction *instr)
{
    u8 result = cpu->reg.a ^ cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void cp_r(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a - cpu->reg.byte_regs[instr->args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->reg.byte_regs[instr->args[0].reg8] & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->reg.byte_regs[instr->args[0].reg8];
    fetch(cpu);
}

void cp_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void cp_hl_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a - cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->io.data & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->io.data;
    fetch(cpu);
}

void cp_n_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    idu_increment(&cpu->reg.pc);
}
void cp_n_m2(SM83 *cpu, Instruction *instr)
{
    u16 result = cpu->reg.a - cpu->io.data;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->io.data & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->io.data;
    fetch(cpu);
}

// Rotate, shift, and bit operations
void rla(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = rotate_left(cpu, cpu->reg.a);

    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rra(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = rotate_right(cpu, cpu->reg.a);
    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rrca(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = rotate_right_carry(cpu, cpu->reg.a);
    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rlca(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = rotate_left(cpu, cpu->reg.a);
    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void cpl(SM83 *cpu, Instruction *instr)
{
    cpu->reg.a = ~cpu->reg.a;
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = 1;
    fetch(cpu);
}

void scf(SM83 *cpu, Instruction *instr)
{
    cpu->reg.flags.c = 1;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void ccf(SM83 *cpu, Instruction *instr)
{
    // cpu->reg.flags.c = ~cpu->reg.flags.c;
    cpu->reg.flags.c = !cpu->reg.flags.c;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.n = 0;
    fetch(cpu);
}

void daa(SM83 *cpu, Instruction *instr)
{
    u8 offset = 0x00;

    u16 reg = cpu->reg.a;

    if (cpu->reg.flags.h || (!cpu->reg.flags.n && ((reg & 0xF) > 0x09)))
    {
        offset |= 0x06;
    }

    if (cpu->reg.flags.c || (!cpu->reg.flags.n && (reg > 0x99)))
    {
        offset |= 0x60;
    }

    if (cpu->reg.flags.n)
    {
        reg = (reg - offset) & 0xFF;
    }
    else
    {
        reg += offset;
    }

    cpu->reg.flags.z = ((reg & 0xFF) == 0);
    if (reg & 0x100)
    {
        cpu->reg.flags.c = 1;
    }
    cpu->reg.flags.h = 0;

    cpu->reg.a = (reg & 0xFF);
    fetch(cpu);
}

// NOTE: Fetch and execute instruction
void cb_fetch(SM83 *cpu, Instruction *instr)
{
    ASSERT(instr->microop_index == 1);

    cpu->instructions[cpu->reg.ir].microop_index = 0;
    cpu->reg.ir = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    idu_increment(&cpu->reg.pc);

    cpu->instructions = cb_instructions;

    cpu->instructions[cpu->reg.ir].microop_index = 0;
    u8 op_index = cpu->instructions[cpu->reg.ir].microop_index;
    cpu->instructions[cpu->reg.ir].handlers[op_index](cpu, &cpu->instructions[cpu->reg.ir]);
}

void jp_nn_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void jp_nn_m2(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void jp_nn_m3(SM83 *cpu, Instruction *instr)
{
    cpu->reg.pc = unsigned_16(cpu->msb, cpu->lsb);
}
void jp_nn_m4(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void jp_cc_nn_m1(SM83 *cpu, Instruction *instr)
{
    cpu->lsb = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    idu_increment(&cpu->reg.pc);
}
void jp_cc_nn_m2(SM83 *cpu, Instruction *instr)
{
    cpu->msb = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    idu_increment(&cpu->reg.pc);
}
void jp_cc_nn_m3(SM83 *cpu, Instruction *instr)
{
    u16 nn = unsigned_16(cpu->msb, cpu->lsb);
    if ((cpu->reg.f & instr->flags))
    {
        cpu->reg.pc = nn;
    }
    else
    {
        fetch(cpu);
    }
}
void jp_cc_nn_m4(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void jp_ncc_nn_m1(SM83 *cpu, Instruction *instr)
{
    cpu->lsb = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    idu_increment(&cpu->reg.pc);
}
void jp_ncc_nn_m2(SM83 *cpu, Instruction *instr)
{
    cpu->msb = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    idu_increment(&cpu->reg.pc);
}
void jp_ncc_nn_m3(SM83 *cpu, Instruction *instr)
{
    u16 nn = unsigned_16(cpu->msb, cpu->lsb);
    if (!(cpu->reg.f & instr->flags))
    {
        cpu->reg.pc = nn;
    }
    else
    {
        fetch(cpu);
    }
}
void jp_ncc_nn_m4(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void jp_hl(SM83 *cpu, Instruction *instr)
{
    cpu->reg.pc = cpu->reg.hl;
    fetch(cpu);
}

// TODO: Test this
void jr_e_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
// void jr_e_m2(SM83 *cpu, Instruction *instr)
// {
//     bool z_sign = (bit(7, cpu->io.data) != 0);
//     u16 result = cpu->io.data + lsb(cpu->reg.pc);
//     bool carry = result > 0xFF;
//     cpu->lsb = result & 0xFF;

//     s8 adj = 0;
//     if (carry && !z_sign)
//     {
//         adj = 1;
//     }
//     else if (!carry && z_sign)
//     {
//         adj = -1;
//     }

//     u8 w = msb(cpu->reg.pc) + adj;
//     cpu->io.data = w;
//     cpu->msb = cpu->io.data;
// }
void jr_e_m2(SM83 *cpu, Instruction *instr)
{
    cpu->reg.pc = (s16)cpu->reg.pc + signed_8(cpu->io.data);
}
void jr_e_m3(SM83 *cpu, Instruction *instr)
{
    // cpu->reg.pc = unsigned_16(cpu->msb, cpu->lsb) + 1;
    fetch(cpu);
}

void jr_cc_e_m1(SM83 *cpu, Instruction *instr)
{
    u8 e = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = e;
    idu_increment(&cpu->reg.pc);
}
void jr_cc_e_m2(SM83 *cpu, Instruction *instr)
{
    if ((cpu->reg.f & instr->flags))
    {
        cpu->reg.pc = cpu->reg.pc + signed_8(cpu->io.data);
    }
    else
    {
        fetch(cpu);
    }
}
void jr_cc_e_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void jr_ncc_e_m1(SM83 *cpu, Instruction *instr)
{
    u8 e = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = e;
    idu_increment(&cpu->reg.pc);
}
void jr_ncc_e_m2(SM83 *cpu, Instruction *instr)
{
    if (!(cpu->reg.f & instr->flags))
    {
        cpu->reg.pc = cpu->reg.pc + signed_8(cpu->io.data);
    }
    else
    {
        fetch(cpu);
    }
}
void jr_ncc_e_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void call_nn_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void call_nn_m2(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    idu_increment(&cpu->reg.pc);
}
void call_nn_m3(SM83 *cpu, Instruction *instr)
{
    idu_decrement(&cpu->reg.sp);
}
void call_nn_m4(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->reg.sp, msb(cpu->reg.pc));
    idu_decrement(&cpu->reg.sp);
}
void call_nn_m5(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->reg.sp, lsb(cpu->reg.pc));
    cpu->reg.pc = unsigned_16(cpu->msb, cpu->lsb);
}
void call_nn_m6(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

// TODO: Fix this
void call_cc_nn_m1(SM83 *cpu, Instruction *instr)
{
    cpu->lsb = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    idu_increment(&cpu->reg.pc);
}
void call_cc_nn_m2(SM83 *cpu, Instruction *instr)
{
    cpu->msb = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    idu_increment(&cpu->reg.pc);
}
void call_cc_nn_m3(SM83 *cpu, Instruction *instr)
{
    cpu->nn = unsigned_16(cpu->msb, cpu->lsb);
    if (!(cpu->reg.f & instr->flags))
    {
        fetch(cpu);
    }
}
void call_cc_nn_m4(SM83 *cpu, Instruction *instr)
{
    idu_decrement(&cpu->reg.sp);
    mmu_write_byte(cpu->memory, cpu->reg.sp, msb(cpu->reg.pc));
}
void call_cc_nn_m5(SM83 *cpu, Instruction *instr)
{
    idu_decrement(&cpu->reg.sp);
    mmu_write_byte(cpu->memory, cpu->reg.sp, lsb(cpu->reg.pc));
    cpu->reg.pc = cpu->nn;
    fetch(cpu);
}

void call_ncc_nn_m1(SM83 *cpu, Instruction *instr)
{
    cpu->lsb = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    idu_increment(&cpu->reg.pc);
}
void call_ncc_nn_m2(SM83 *cpu, Instruction *instr)
{
    cpu->msb = mmu_read_byte(cpu->memory, cpu->reg.pc, false);
    idu_increment(&cpu->reg.pc);
}
void call_ncc_nn_m3(SM83 *cpu, Instruction *instr)
{
    cpu->nn = unsigned_16(cpu->msb, cpu->lsb);
    if ((cpu->reg.f & instr->flags))
    {
        fetch(cpu);
    }
}
void call_ncc_nn_m4(SM83 *cpu, Instruction *instr)
{
    idu_decrement(&cpu->reg.sp);
    mmu_write_byte(cpu->memory, cpu->reg.sp, msb(cpu->reg.pc));
}
void call_ncc_nn_m5(SM83 *cpu, Instruction *instr)
{
    idu_decrement(&cpu->reg.sp);
    mmu_write_byte(cpu->memory, cpu->reg.sp, lsb(cpu->reg.pc));
    cpu->reg.pc = cpu->nn;
    fetch(cpu);
}

// Returns
void ret_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void ret_m2(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void ret_m3(SM83 *cpu, Instruction *instr)
{
    cpu->reg.pc = unsigned_16(cpu->msb, cpu->lsb);
}
void ret_m4(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void ret_cc_m1(SM83 *cpu, Instruction *instr)
{
    bool cc = (cpu->reg.f & instr->flags);
    if (!cc)
    {
        fetch(cpu);
    }
}
void ret_cc_m2(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void ret_cc_m3(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void ret_cc_m4(SM83 *cpu, Instruction *instr)
{
    cpu->reg.pc = unsigned_16(cpu->msb, cpu->lsb);
}
void ret_cc_m5(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void ret_ncc_m1(SM83 *cpu, Instruction *instr)
{
    bool cc = !(cpu->reg.f & instr->flags);
    if (!cc)
    {
        fetch(cpu);
    }
}
void ret_ncc_m2(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void ret_ncc_m3(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void ret_ncc_m4(SM83 *cpu, Instruction *instr)
{
    cpu->reg.pc = unsigned_16(cpu->msb, cpu->lsb);
}
void ret_ncc_m5(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void reti_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void reti_m2(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void reti_m3(SM83 *cpu, Instruction *instr)
{
    cpu->reg.pc = unsigned_16(cpu->msb, cpu->lsb);
    cpu->ime = IMEState::Enabled;
}
void reti_m4(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void push_rr_m1(SM83 *cpu, Instruction *instr)
{
    idu_decrement(&cpu->reg.sp);
}
void push_rr_m2(SM83 *cpu, Instruction *instr)
{
    u16 rr = cpu->reg.word_regs[instr->args[0].reg16];
    mmu_write_byte(cpu->memory, cpu->reg.sp, msb(rr));
    idu_decrement(&cpu->reg.sp);
}
void push_rr_m3(SM83 *cpu, Instruction *instr)
{
    u16 rr = cpu->reg.word_regs[instr->args[0].reg16];
    mmu_write_byte(cpu->memory, cpu->reg.sp, lsb(rr));
}
void push_rr_m4(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void pop_rr_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = z;
    cpu->lsb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void pop_rr_m2(SM83 *cpu, Instruction *instr)
{
    u8 w = mmu_read_byte(cpu->memory, cpu->reg.sp, false);
    cpu->io.data = w;
    cpu->msb = cpu->io.data;
    idu_increment(&cpu->reg.sp);
}
void pop_rr_m3(SM83 *cpu, Instruction *instr)
{
    switch (instr->args[0].reg16)
    {
    case REG16_AF:
        cpu->reg.word_regs[instr->args[0].reg16] = unsigned_16(cpu->msb, (cpu->lsb & 0xF0));
        break;
    default:
        cpu->reg.word_regs[instr->args[0].reg16] = unsigned_16(cpu->msb, cpu->lsb);
        break;
    }
    fetch(cpu);
}

void rst_n_m1(SM83 *cpu, Instruction *instr)
{
    idu_decrement(&cpu->reg.sp);
}
void rst_n_m2(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->reg.sp, msb(cpu->reg.pc));
    idu_decrement(&cpu->reg.sp);
}
void rst_n_m3(SM83 *cpu, Instruction *instr)
{
    mmu_write_byte(cpu->memory, cpu->reg.sp, lsb(cpu->reg.pc));
    cpu->reg.pc = instr->args[0].data;
}
void rst_n_m4(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

// CB-prefixed instructions
void rlc_r(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = rotate_left(cpu, cpu->reg.byte_regs[instr->args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rlc_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void rlc_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->io.data = rotate_left_carry(cpu, cpu->io.data);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->io.data);
}
void rlc_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void rrc_r(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = rotate_right_carry(cpu, cpu->reg.byte_regs[instr->args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rrc_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void rrc_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->io.data = rotate_right_carry(cpu, cpu->io.data);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->io.data);
}
void rrc_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void rl_r(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = rotate_left(cpu, cpu->reg.byte_regs[instr->args[0].reg8]);

    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rl_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void rl_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->io.data = rotate_left(cpu, cpu->io.data);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->io.data);
}
void rl_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

void rr_r(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = rotate_right(cpu, cpu->reg.byte_regs[instr->args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rr_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void rr_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->io.data = rotate_right(cpu, cpu->io.data);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->io.data);
}
void rr_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

// Shift left arithmetic
void sla_r(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = sla(cpu, cpu->reg.byte_regs[instr->args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void sla_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void sla_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->io.data = sla(cpu, cpu->io.data);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->io.data);
}
void sla_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

// Shift right arithmetic
void sra_r(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = sra(cpu, cpu->reg.byte_regs[instr->args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void sra_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void sra_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->io.data = sra(cpu, cpu->io.data);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->io.data);
}
void sra_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

// Swap nibbles
void swap_r(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = swap(cpu, cpu->reg.byte_regs[instr->args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void swap_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void swap_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->io.data = swap(cpu, cpu->io.data);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->io.data);
}
void swap_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

// Shift right logical
void srl_r(SM83 *cpu, Instruction *instr)
{
    cpu->reg.byte_regs[instr->args[0].reg8] = srl(cpu, cpu->reg.byte_regs[instr->args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void srl_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void srl_hl_m2(SM83 *cpu, Instruction *instr)
{
    cpu->io.data = srl(cpu, cpu->io.data);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    mmu_write_byte(cpu->memory, cpu->reg.hl, cpu->io.data);
}
void srl_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

// Test bit
void bit_b_r(SM83 *cpu, Instruction *instr)
{
    u8 bit_to_test = instr->args[0].data;
    u8 reg = cpu->reg.byte_regs[instr->args[1].reg8];

    u8 carry_flag = cpu->reg.flags.c;

    cpu->reg.flags.z = ((reg & bit_to_test) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = carry_flag;
    fetch(cpu);
}

void bit_b_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void bit_b_hl_m2(SM83 *cpu, Instruction *instr)
{
    u8 bit_to_test = instr->args[0].data;

    u8 carry_flag = cpu->reg.flags.c;

    cpu->reg.flags.z = ((cpu->io.data & bit_to_test) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = carry_flag;
    fetch(cpu);
}
// Reset bit
void res_b_r(SM83 *cpu, Instruction *instr)
{
    u8 bit = instr->args[0].data;
    u8 reg_index = instr->args[1].reg8;
    cpu->reg.byte_regs[reg_index] = reset_bit(cpu->reg.byte_regs[reg_index], bit);
    fetch(cpu);
}

void res_b_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void res_b_hl_m2(SM83 *cpu, Instruction *instr)
{
    u8 bit = instr->args[0].data;
    u8 z = reset_bit(cpu->io.data, bit);
    mmu_write_byte(cpu->memory, cpu->reg.hl, z);
}
void res_b_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}

// Set bit
void set_b_r(SM83 *cpu, Instruction *instr)
{
    u8 bit = instr->args[0].data;
    u8 reg_index = instr->args[1].reg8;
    cpu->reg.byte_regs[reg_index] = set_bit(cpu->reg.byte_regs[reg_index], bit);
    fetch(cpu);
}

void set_b_hl_m1(SM83 *cpu, Instruction *instr)
{
    u8 z = mmu_read_byte(cpu->memory, cpu->reg.hl, false);
    cpu->io.data = z;
}
void set_b_hl_m2(SM83 *cpu, Instruction *instr)
{
    u8 bit = instr->args[0].data;
    u8 z = set_bit(cpu->io.data, bit);
    mmu_write_byte(cpu->memory, cpu->reg.hl, z);
}
void set_b_hl_m3(SM83 *cpu, Instruction *instr)
{
    fetch(cpu);
}
