#include "sm83.h"
#include "memory.h"

// static u8 sm83_read(SM83 *cpu, u16 address);
// static void sm83_write(SM83 *cpu, u16 address, u8 value);

// static void log_cpu_state(SM83 *cpu)
// {
//     char buffer[256];
//     sprintf(buffer,
//             "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
//             cpu->reg.a, cpu->reg.f, cpu->reg.b, cpu->reg.c, cpu->reg.d, cpu->reg.e, cpu->reg.h, cpu->reg.l,
//             cpu->reg.sp, cpu->reg.pc, sm83_read(cpu, cpu->reg.pc), sm83_read(cpu, cpu->reg.pc + 1), sm83_read(cpu,
//             cpu->reg.pc + 2), sm83_read(cpu, cpu->reg.pc + 3));

//     FILE *log = fopen("cpu_log.txt", "a");
//     if (!log)
//     {
//     }
//     fputs(buffer, log);

//     fclose(log);
// }


void sm83_reset(SM83 *cpu)
{
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

    u8 carry_flag = (value & 0x01) ? 1 : 0;
    value = (value >> 1) + (old_carry << 7);

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 rotate_right_carry(SM83 *cpu, u8 value)
{
    u8 carry_flag = (value & 0x01) ? 1 : 0;
    value = (value >> 1) + (carry_flag << 7);

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 sla(SM83 *cpu, u8 value)
{
    u8 carry_flag = (value & 0x80) ? 1 : 0;
    value <<= 1;

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 sra(SM83 *cpu, u8 value)
{
    u8 carry_flag = (value & 0x01) ? 1 : 0;
    u8 sign_bit = (value & 0x80) ? 1 : 0;
    value >>= 1;
    value |= (sign_bit << 7);

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = carry_flag;

    return value;
}

static u8 srl(SM83 *cpu, u8 value)
{
    u8 carry_flag = (value & 0x01) ? 1 : 0;
    value >>= 1;

    cpu->reg.flags.z = (value == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
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

static u8 sm83_read(SM83 *cpu, u16 address)
{
    u8 data = gb_read_memory(cpu->memory, address);

    return data;
}

static void sm83_read_request(SM83 *cpu, u16 address)
{
    if (cpu->io_state == Idle)
    {
        cpu->io_state = Read;
        cpu->memory->address_bus = address;
    }
    // gb_read_request(cpu->memory, address, SM83_CPU);
    // u8 data = gb_read_memory(cpu->memory, address);

    // return data;
}

static void sm83_write(SM83 *cpu, u16 address, u8 value)
{
    cpu->memory->address_bus = address;
    gb_write_memory(cpu->memory, address, value);
}

static void sm83_flush_read(SM83 *cpu)
{
    if (cpu->io_state == Read)
    {
        cpu->memory->data_bus = gb_read_memory(cpu->memory, cpu->memory->address_bus);
        cpu->io_state = Idle;
    }
}

static void sm83_flush_write(SM83 *cpu)
{
    if (cpu->io_state == Write)
    {
        gb_write_memory(cpu->memory, cpu->memory->address_bus, cpu->memory->data_bus);
        cpu->io_state = Idle;
    }
}

static void fetch(SM83 *cpu)
{
    u8 previous_opcode = cpu->reg.ir;

    cpu->fetching = true;
    cpu->instruction.microop_index = 0;
    cpu->instructions = instructions;
    sm83_read_request(cpu, cpu->reg.pc);

    if (cpu->interrupt.state == Serviced)
    {
        cpu->interrupt.state = None;
    }

    idu_increment(&cpu->reg.pc);
}

static void dummy_fetch(SM83 *cpu)
{
    cpu->fetching = true;
    cpu->instruction.microop_index = 0;
    cpu->instructions = instructions;
    sm83_read_request(cpu, cpu->reg.pc);
}

static void service_interrupt(SM83 *cpu)
{
    cpu->interrupt.state = Servicing;
    cpu->instruction.microop_index = 0;
    cpu->instructions = isr;
}

static inline u8 get_pending_interrupts(SM83 *cpu)
{
    return (cpu->memory->io_registers[IO_IF] & cpu->memory->interrupt_enable) & 0x1F;
}

static void check_interrupt(SM83 *cpu)
{
    if (cpu->interrupt.state == None)
    {
        u8 pending_interrupts = 0;
        if ((pending_interrupts = get_pending_interrupts(cpu)))
        {
            cpu->interrupt.state = Pending;
        }
    }
}

static void sm83_tick(SM83 *cpu)
{
    bool temp_halt = cpu->halted;
    if (cpu->interrupt.state == Pending && cpu->instruction.microop_index == 0)
    {
        // Handle interrupt at beginning of instruction
        // TODO: This won't work for a CB instruction, because cb_fetch sets microop_index to 0. But we are not at the
        // beginning of an instruction.

        // Wakeup as soon as interrupt becomes pending
        cpu->halted = false;
        if (cpu->ime == Enabled)
        {
            cpu->interrupt.state = Servicing;
            service_interrupt(cpu);
        }
        else
        {
            cpu->interrupt.state = None;
        }
    }

    if (cpu->fetching)
    {
        cpu->fetching = false;
        cpu->reg.ir = cpu->memory->data_bus;
    }

    // if (!cpu->instructions)
    // {
    //     cpu->instructions = instructions;
    //     cpu->instruction.args = cpu->instructions[0x00].args;
    //     cpu->instructions[0x00].microops[0](cpu, cpu->instruction.args);
    //     return;
    // }

    if (!temp_halt)
    {
        u8 opcode = cpu->interrupt.state == Servicing ? 0 : cpu->reg.ir;

        u8 microop_index = cpu->instruction.microop_index;
        cpu->instruction.microop_index++;

        cpu->instruction.args = cpu->instructions[opcode].args;
        cpu->instructions[opcode].microops[microop_index](cpu, cpu->instruction.args);
    }

    // NOTE: Enabling IME is delayed by one instruction; only enable if requested and at the end of an instruction.
    if (cpu->instruction.microop_index == 0 && cpu->ime == Requested)
    {
        cpu->ime = Enabled;
    }
}

void sm83_tick_t0(SM83 *cpu)
{
    check_interrupt(cpu);
    sm83_tick(cpu);
}

void sm83_tick_t1(SM83 *cpu)
{
    check_interrupt(cpu);
    sm83_flush_write(cpu);
}

void sm83_tick_t2(SM83 *cpu)
{
    check_interrupt(cpu);
}

void sm83_tick_t3(SM83 *cpu)
{
    check_interrupt(cpu);
    sm83_flush_read(cpu);
}

/*********** Instructions ***********/
void unimplemented_instruction(SM83 *cpu, InstructionArg *args)
{
    // TODO: Error handling/logging
}

void isr_m0(SM83 *cpu, InstructionArg *args)
{
    idu_decrement(&cpu->reg.pc);
}
void isr_m1(SM83 *cpu, InstructionArg *args)
{
    idu_decrement(&cpu->reg.sp);
}
void isr_m2(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.sp, msb(cpu->reg.pc));
    idu_decrement(&cpu->reg.sp);
}
void isr_m3(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.sp, lsb(cpu->reg.pc));

    u16 IRQ_VECTOR[] = {0x0000, 0x0040, 0x0048, 0x0000, 0x0050, 0x0000, 0x0000, 0x0000, 0x0058,
                        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0060};

    u8 pending_interrupts = get_pending_interrupts(cpu);
    u8 lsb = (pending_interrupts & -pending_interrupts);

    ASSERT(lsb <= 16);
    cpu->reg.pc = IRQ_VECTOR[lsb];

    // Clear handled interrupt
    cpu->memory->io_registers[IO_IF] ^= lsb;
}
void isr_m4(SM83 *cpu, InstructionArg *args)
{
    cpu->ime = Disabled;
    cpu->interrupt.state = Serviced;
    fetch(cpu);
}

void nop(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void stop(SM83 *cpu, InstructionArg *args)
{
}

void halt(SM83 *cpu, InstructionArg *args)
{
    // // Weakeup as soon as interrupt is pending
    cpu->halted = !get_pending_interrupts(cpu);

    dummy_fetch(cpu);

    if (!cpu->halted && cpu->ime == Disabled)
    {
        // TODO: Handle halt bug?
    }

    // // if (cpu->halted)
    // // {
    // //     idu_increment(&cpu->reg.pc);
    // // }
}

void di(SM83 *cpu, InstructionArg *args)
{
    cpu->ime = Disabled;
    fetch(cpu);
}

void ei(SM83 *cpu, InstructionArg *args)
{
    if (cpu->ime == Disabled)
    {
        cpu->ime = Requested;
    }
    fetch(cpu);
}

void ld_r_rp(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = cpu->reg.byte_regs[args[1].reg8];
    fetch(cpu);
}

void ld_rr_nn_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_rr_nn_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_rr_nn_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->nn = unsigned_16(cpu->memory->data_bus, cpu->lsb);
    u8 reg_index = args[0].reg16;
    cpu->reg.word_regs[reg_index] = cpu->nn;
    fetch(cpu);
}

void ld_bc_a_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.bc, cpu->reg.a);
}
void ld_bc_a_m1(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ld_de_a_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.de, cpu->reg.a);
}
void ld_de_a_m1(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ldi_hl_a_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.hl, cpu->reg.a);
    idu_increment(&cpu->reg.hl);
}
void ldi_hl_a_m1(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ldd_hl_a_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.hl, cpu->reg.a);
    idu_decrement(&cpu->reg.hl);
}
void ldd_hl_a_m1(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ldi_a_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
    idu_increment(&cpu->reg.hl);
}
void ldi_a_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = cpu->memory->data_bus;
    fetch(cpu);
}

void ldd_a_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
    idu_decrement(&cpu->reg.hl);
}
void ldd_a_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = cpu->memory->data_bus;
    fetch(cpu);
}

void ld_nn_sp_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_nn_sp_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_nn_sp_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->address = unsigned_16(cpu->memory->data_bus, cpu->lsb);
    sm83_write(cpu, cpu->address, lsb(cpu->reg.sp));
    idu_increment(&cpu->address);
}
void ld_nn_sp_m3(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->address, msb(cpu->reg.sp));
}
void ld_nn_sp_m4(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ld_sp_hl_m0(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.sp = cpu->reg.hl;
}
void ld_sp_hl_m1(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

// TODO: Test this
void ld_hl_spe_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_hl_spe_m1(SM83 *cpu, InstructionArg *args)
{
    u8 lsb_sp = lsb(cpu->reg.sp);

    u16 result = lsb_sp + cpu->memory->data_bus;

    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (lsb_sp & 0xF) + (cpu->memory->data_bus & 0xF) > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.l = result;
}
void ld_hl_spe_m2(SM83 *cpu, InstructionArg *args)
{
    bool z_sign = get_bit(7, cpu->memory->data_bus);
    u8 adj = z_sign ? 0xFF : 0x00;

    u16 result = msb(cpu->reg.sp) + adj + cpu->reg.flags.c;
    cpu->reg.h = result & 0xFF;
    fetch(cpu);
}

void ld_nn_a_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_nn_a_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_nn_a_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->address = unsigned_16(cpu->memory->data_bus, cpu->lsb);
    sm83_write(cpu, cpu->address, cpu->reg.a);
}
void ld_nn_a_m3(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ld_a_bc_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.bc);
}
void ld_a_bc_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = cpu->memory->data_bus;
    fetch(cpu);
}

void ld_a_de_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.de);
}
void ld_a_de_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = cpu->memory->data_bus;
    fetch(cpu);
}

void ld_a_nn_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_a_nn_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_a_nn_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->address = unsigned_16(cpu->memory->data_bus, cpu->lsb);
    sm83_read_request(cpu, cpu->address);
}
void ld_a_nn_m3(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = cpu->memory->data_bus;
    fetch(cpu);
}

void ld_r_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_r_n_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = cpu->memory->data_bus;
    fetch(cpu);
}

void ld_r_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void ld_r_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = cpu->memory->data_bus;
    fetch(cpu);
}

void ld_hl_r_m0(SM83 *cpu, InstructionArg *args)
{
    u8 data = cpu->reg.byte_regs[args[0].reg8];
    sm83_write(cpu, cpu->reg.hl, data);
}
void ld_hl_r_m1(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ld_hl_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ld_hl_n_m1(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.hl, cpu->memory->data_bus);
}
void ld_hl_n_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ldh_n_a_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ldh_n_a_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    cpu->address = unsigned_16(0xFF, cpu->lsb);
    sm83_write(cpu, cpu->address, cpu->reg.a);
}
void ldh_n_a_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ldh_a_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void ldh_a_n_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    cpu->address = unsigned_16(0xFF, cpu->lsb);
    sm83_read_request(cpu, cpu->address);
}
void ldh_a_n_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = cpu->memory->data_bus;
    fetch(cpu);
}

void ldh_c_a_m0(SM83 *cpu, InstructionArg *args)
{
    cpu->address = unsigned_16(0xFF, lsb(cpu->reg.c));
    sm83_write(cpu, cpu->address, cpu->reg.a);
}
void ldh_c_a_m1(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ldh_a_c_m0(SM83 *cpu, InstructionArg *args)
{
    cpu->address = unsigned_16(0xFF, lsb(cpu->reg.c));
    sm83_read_request(cpu, cpu->address);
}
void ldh_a_c_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = cpu->memory->data_bus;
    fetch(cpu);
}

void inc_rr_m0(SM83 *cpu, InstructionArg *args)
{
    idu_increment(&cpu->reg.word_regs[args[0].reg16]);
}
void inc_rr_m1(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void inc_r(SM83 *cpu, InstructionArg *args)
{
    u8 carry_flag = cpu->reg.flags.c;

    u8 reg_index = args[0].reg8;
    u8 reg = cpu->reg.byte_regs[reg_index];
    u16 result = reg + 1;
    cpu->reg.byte_regs[reg_index] = result & 0xFF;

    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.h = ((reg & 0xF) + 1) > 0xF;
    cpu->reg.flags.n = 0;
    // cpu->reg.flags.h = (result & 0xF) == 0;
    cpu->reg.flags.c = carry_flag;
    fetch(cpu);
}

void inc_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void inc_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u8 carry_flag = cpu->reg.flags.c;

    u8 result = (cpu->memory->data_bus + 1) & 0xFF;
    sm83_write(cpu, cpu->reg.hl, result);

    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.h = (result & 0xF) == 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.c = carry_flag;
}
void inc_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void dec_rr_m0(SM83 *cpu, InstructionArg *args)
{
    idu_decrement(&cpu->reg.word_regs[args[0].reg16]);
}
void dec_rr_m1(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void dec_r(SM83 *cpu, InstructionArg *args)
{
    u8 carry_flag = cpu->reg.flags.c;

    u8 reg_index = args[0].reg8;
    u8 reg = cpu->reg.byte_regs[reg_index];
    u16 result = reg - 1;
    cpu->reg.byte_regs[reg_index] = result & 0xFF;

    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.byte_regs[reg_index] & 0xF) == 0xF;
    cpu->reg.flags.c = carry_flag;
    fetch(cpu);
}

void dec_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void dec_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u8 carry_flag = cpu->reg.flags.c;

    u8 result = (cpu->memory->data_bus - 1) & 0xFF;
    sm83_write(cpu, cpu->reg.hl, result);

    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (result & 0xF) == 0xF;
    cpu->reg.flags.c = carry_flag;
}
void dec_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void add_r(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a + cpu->reg.byte_regs[args[0].reg8];
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->reg.byte_regs[args[0].reg8] & 0xF) > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void add_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void add_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a + cpu->memory->data_bus;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->memory->data_bus & 0xF) > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

// TODO: Test this
void add_hl_rr_m0(SM83 *cpu, InstructionArg *args)
{
    u8 zero_flag = cpu->reg.flags.z;
    cpu->msb = msb(cpu->reg.word_regs[args[0].reg16]);
    cpu->lsb = lsb(cpu->reg.word_regs[args[0].reg16]);

    u16 result = cpu->reg.l + cpu->lsb;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.c = result > 0xFF;
    cpu->reg.flags.h = ((cpu->reg.l & 0xF) + (cpu->lsb & 0xF) > 0xF);
    cpu->reg.flags.z = zero_flag;

    cpu->reg.l = result & 0xFF;
}
void add_hl_rr_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.h + cpu->msb + cpu->reg.flags.c;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = ((cpu->reg.h & 0xF) + (cpu->msb & 0xF) + cpu->reg.flags.c > 0xF);
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.h = result & 0xFF;
    fetch(cpu);
}

void add_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void add_n_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a + cpu->memory->data_bus;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->memory->data_bus & 0xF) > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

// TODO: Test this
void add_sp_e_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void add_sp_e_m1(SM83 *cpu, InstructionArg *args)
{
    s8 e = signed_8(cpu->memory->data_bus);
    u16 result = cpu->reg.sp + e;

    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = ((cpu->reg.sp & 0xF) + (e & 0xF)) > 0xF;
    cpu->reg.flags.c = ((cpu->reg.sp & 0xFF) + (e & 0xFF)) > 0xFF;

    cpu->reg.sp = result;
}
void add_sp_e_m2(SM83 *cpu, InstructionArg *args)
{
}
void add_sp_e_m3(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void adc_r(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a + cpu->reg.byte_regs[args[0].reg8] + cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->reg.byte_regs[args[0].reg8] & 0xF) + cpu->reg.flags.c > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void adc_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void adc_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a + cpu->memory->data_bus + cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->memory->data_bus & 0xF) + cpu->reg.flags.c > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void adc_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void adc_n_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a + cpu->memory->data_bus + cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) + (cpu->memory->data_bus & 0xF) + cpu->reg.flags.c > 0xF;
    cpu->reg.flags.c = result > 0xFF;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sub_r(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a - cpu->reg.byte_regs[args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->reg.byte_regs[args[0].reg8] & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->reg.byte_regs[args[0].reg8];

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sub_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void sub_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a - cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->memory->data_bus & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->memory->data_bus;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sub_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void sub_n_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a - cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->memory->data_bus & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->memory->data_bus;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sbc_r(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a - cpu->reg.byte_regs[args[0].reg8] - cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) - cpu->reg.flags.c < (cpu->reg.byte_regs[args[0].reg8] & 0xF);
    cpu->reg.flags.c = cpu->reg.a - cpu->reg.flags.c < cpu->reg.byte_regs[args[0].reg8];

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sbc_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void sbc_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a - cpu->memory->data_bus - cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) - cpu->reg.flags.c < (cpu->memory->data_bus & 0xF);
    cpu->reg.flags.c = cpu->reg.a - cpu->reg.flags.c < cpu->memory->data_bus;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void sbc_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void sbc_n_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a - cpu->memory->data_bus - cpu->reg.flags.c;
    cpu->reg.flags.z = ((result & 0xFF) == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) - cpu->reg.flags.c < (cpu->memory->data_bus & 0xF);
    cpu->reg.flags.c = cpu->reg.a - cpu->reg.flags.c < cpu->memory->data_bus;

    cpu->reg.a = result & 0xFF;
    fetch(cpu);
}

void and_r(SM83 *cpu, InstructionArg *args)
{
    u8 result = cpu->reg.a & cpu->reg.byte_regs[args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void and_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void and_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u8 result = cpu->reg.a & cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void and_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void and_n_m1(SM83 *cpu, InstructionArg *args)
{
    u8 result = cpu->reg.a & cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void or_r(SM83 *cpu, InstructionArg *args)
{
    u8 result = cpu->reg.a | cpu->reg.byte_regs[args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void or_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void or_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u8 result = cpu->reg.a | cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void or_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void or_n_m1(SM83 *cpu, InstructionArg *args)
{
    u8 result = cpu->reg.a | cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void xor_r(SM83 *cpu, InstructionArg *args)
{
    u8 result = cpu->reg.a ^ cpu->reg.byte_regs[args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void xor_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void xor_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u8 result = cpu->reg.a ^ cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void xor_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void xor_n_m1(SM83 *cpu, InstructionArg *args)
{
    u8 result = cpu->reg.a ^ cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.c = 0;

    cpu->reg.a = result;
    fetch(cpu);
}

void cp_r(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a - cpu->reg.byte_regs[args[0].reg8];
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->reg.byte_regs[args[0].reg8] & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->reg.byte_regs[args[0].reg8];
    fetch(cpu);
}

void cp_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void cp_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a - cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->memory->data_bus & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->memory->data_bus;
    fetch(cpu);
}

void cp_n_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void cp_n_m1(SM83 *cpu, InstructionArg *args)
{
    u16 result = cpu->reg.a - cpu->memory->data_bus;
    cpu->reg.flags.z = (result == 0);
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = (cpu->reg.a & 0xF) < (cpu->memory->data_bus & 0xF);
    cpu->reg.flags.c = cpu->reg.a < cpu->memory->data_bus;
    fetch(cpu);
}

// Rotate, shift, and bit operations
void rla(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = rotate_left(cpu, cpu->reg.a);

    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rra(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = rotate_right(cpu, cpu->reg.a);
    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rrca(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = rotate_right_carry(cpu, cpu->reg.a);
    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rlca(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = rotate_left_carry(cpu, cpu->reg.a);
    cpu->reg.flags.z = 0;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void cpl(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.a = ~cpu->reg.a;
    cpu->reg.flags.n = 1;
    cpu->reg.flags.h = 1;
    fetch(cpu);
}

void scf(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.flags.c = 1;
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void ccf(SM83 *cpu, InstructionArg *args)
{
    // cpu->reg.flags.c = ~cpu->reg.flags.c;
    cpu->reg.flags.c = !cpu->reg.flags.c;
    cpu->reg.flags.h = 0;
    cpu->reg.flags.n = 0;
    fetch(cpu);
}

void daa(SM83 *cpu, InstructionArg *args)
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
void cb_fetch(SM83 *cpu, InstructionArg *args)
{
    ASSERT(cpu->instruction.microop_index == 1);

    fetch(cpu);
    cpu->instructions = cb_instructions;
}

void jp_nn_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void jp_nn_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void jp_nn_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.pc = unsigned_16(cpu->memory->data_bus, cpu->lsb);
}
void jp_nn_m3(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void jp_cc_nn_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void jp_cc_nn_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void jp_cc_nn_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->msb = cpu->memory->data_bus;
    u16 nn = unsigned_16(cpu->msb, cpu->lsb);
    if ((cpu->reg.f & args[0].flags))
    {
        cpu->reg.pc = nn;
    }
    else
    {
        fetch(cpu);
    }
}
void jp_cc_nn_m3(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void jp_ncc_nn_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void jp_ncc_nn_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void jp_ncc_nn_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->msb = cpu->memory->data_bus;
    u16 nn = unsigned_16(cpu->msb, cpu->lsb);
    if (!(cpu->reg.f & args[0].flags))
    {
        cpu->reg.pc = nn;
    }
    else
    {
        fetch(cpu);
    }
}
void jp_ncc_nn_m3(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void jp_hl(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.pc = cpu->reg.hl;
    fetch(cpu);
}

// TODO: Test this
void jr_e_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void jr_e_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.pc = (s16)cpu->reg.pc + signed_8(cpu->memory->data_bus);
}
void jr_e_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void jr_cc_e_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void jr_cc_e_m1(SM83 *cpu, InstructionArg *args)
{
    if ((cpu->reg.f & args[0].flags))
    {
        cpu->reg.pc = cpu->reg.pc + signed_8(cpu->memory->data_bus);
    }
    else
    {
        fetch(cpu);
    }
}
void jr_cc_e_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void jr_ncc_e_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void jr_ncc_e_m1(SM83 *cpu, InstructionArg *args)
{
    if (!(cpu->reg.f & args[0].flags))
    {
        cpu->reg.pc = cpu->reg.pc + signed_8(cpu->memory->data_bus);
    }
    else
    {
        fetch(cpu);
    }
}
void jr_ncc_e_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void call_nn_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void call_nn_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void call_nn_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->msb = cpu->memory->data_bus;
    idu_decrement(&cpu->reg.sp);
}
void call_nn_m3(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.sp, msb(cpu->reg.pc));
    idu_decrement(&cpu->reg.sp);
}
void call_nn_m4(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.sp, lsb(cpu->reg.pc));
    cpu->reg.pc = unsigned_16(cpu->msb, cpu->lsb);
}
void call_nn_m5(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

// TODO: Fix this
void call_cc_nn_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void call_cc_nn_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void call_cc_nn_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->msb = cpu->memory->data_bus;
    cpu->nn = unsigned_16(cpu->msb, cpu->lsb);
    if (!(cpu->reg.f & args[0].flags))
    {
        fetch(cpu);
    }
}
void call_cc_nn_m3(SM83 *cpu, InstructionArg *args)
{
    idu_decrement(&cpu->reg.sp);
}
void call_cc_nn_m4(SM83 *cpu, InstructionArg *args)
{
    // idu_decrement(&cpu->reg.sp);
    sm83_write(cpu, cpu->reg.sp, msb(cpu->reg.pc));
}
void call_cc_nn_m5(SM83 *cpu, InstructionArg *args)
{
    idu_decrement(&cpu->reg.sp);
    sm83_write(cpu, cpu->reg.sp, lsb(cpu->reg.pc));
    cpu->reg.pc = cpu->nn;
    fetch(cpu);
}

void call_ncc_nn_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void call_ncc_nn_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.pc);
    idu_increment(&cpu->reg.pc);
}
void call_ncc_nn_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->msb = cpu->memory->data_bus;
    cpu->nn = unsigned_16(cpu->msb, cpu->lsb);
    if ((cpu->reg.f & args[0].flags))
    {
        fetch(cpu);
    }
}
void call_ncc_nn_m3(SM83 *cpu, InstructionArg *args)
{
    idu_decrement(&cpu->reg.sp);
}
void call_ncc_nn_m4(SM83 *cpu, InstructionArg *args)
{
    // idu_decrement(&cpu->reg.sp);
    sm83_write(cpu, cpu->reg.sp, msb(cpu->reg.pc));
}
void call_ncc_nn_m5(SM83 *cpu, InstructionArg *args)
{
    idu_decrement(&cpu->reg.sp);
    sm83_write(cpu, cpu->reg.sp, lsb(cpu->reg.pc));
    cpu->reg.pc = cpu->nn;
    fetch(cpu);
}

// Returns
void ret_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.sp);
    idu_increment(&cpu->reg.sp);
}
void ret_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.sp);
    idu_increment(&cpu->reg.sp);
}
void ret_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.pc = unsigned_16(cpu->memory->data_bus, cpu->lsb);
}
void ret_m3(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ret_cc_m0(SM83 *cpu, InstructionArg *args)
{
    cpu->b = (cpu->reg.f & args[0].flags);
}
void ret_cc_m1(SM83 *cpu, InstructionArg *args)
{
    if (!cpu->b)
    {
        fetch(cpu);
    }
    else
    {
        sm83_read_request(cpu, cpu->reg.sp);
        idu_increment(&cpu->reg.sp);
    }
}
void ret_cc_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.sp);
    idu_increment(&cpu->reg.sp);
}
void ret_cc_m3(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.pc = unsigned_16(cpu->memory->data_bus, cpu->lsb);
}
void ret_cc_m4(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void ret_ncc_m0(SM83 *cpu, InstructionArg *args)
{
    cpu->b = !(cpu->reg.f & args[0].flags);
}
void ret_ncc_m1(SM83 *cpu, InstructionArg *args)
{
    if (!cpu->b)
    {
        fetch(cpu);
    }
    else
    {
        sm83_read_request(cpu, cpu->reg.sp);
        idu_increment(&cpu->reg.sp);
    }
}
void ret_ncc_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.sp);
    idu_increment(&cpu->reg.sp);
}
void ret_ncc_m3(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.pc = unsigned_16(cpu->memory->data_bus, cpu->lsb);
}
void ret_ncc_m4(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void reti_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.sp);
    idu_increment(&cpu->reg.sp);
}
void reti_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.sp);
    idu_increment(&cpu->reg.sp);
}
void reti_m2(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.pc = unsigned_16(cpu->memory->data_bus, cpu->lsb);
    cpu->ime = Requested;
}
void reti_m3(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void push_rr_m0(SM83 *cpu, InstructionArg *args)
{
    idu_decrement(&cpu->reg.sp);
}
void push_rr_m1(SM83 *cpu, InstructionArg *args)
{
    u16 rr = cpu->reg.word_regs[args[0].reg16];
    sm83_write(cpu, cpu->reg.sp, msb(rr));
    idu_decrement(&cpu->reg.sp);
}
void push_rr_m2(SM83 *cpu, InstructionArg *args)
{
    u16 rr = cpu->reg.word_regs[args[0].reg16];
    sm83_write(cpu, cpu->reg.sp, lsb(rr));
}
void push_rr_m3(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void pop_rr_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.sp);
    idu_increment(&cpu->reg.sp);
}
void pop_rr_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->lsb = cpu->memory->data_bus;
    sm83_read_request(cpu, cpu->reg.sp);
    idu_increment(&cpu->reg.sp);
}
void pop_rr_m2(SM83 *cpu, InstructionArg *args)
{
    switch (args[0].reg16)
    {
    case REG16_AF:
        cpu->reg.word_regs[args[0].reg16] = unsigned_16(cpu->memory->data_bus, (cpu->lsb & 0xF0));
        break;
    default:
        cpu->reg.word_regs[args[0].reg16] = unsigned_16(cpu->memory->data_bus, cpu->lsb);
        break;
    }
    fetch(cpu);
}

void rst_n_m0(SM83 *cpu, InstructionArg *args)
{
    idu_decrement(&cpu->reg.sp);
}
void rst_n_m1(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.sp, msb(cpu->reg.pc));
    idu_decrement(&cpu->reg.sp);
}
void rst_n_m2(SM83 *cpu, InstructionArg *args)
{
    sm83_write(cpu, cpu->reg.sp, lsb(cpu->reg.pc));
    cpu->reg.pc = args[0].data;
}
void rst_n_m3(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

// CB-prefixed instructions
void rlc_r(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = rotate_left_carry(cpu, cpu->reg.byte_regs[args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rlc_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void rlc_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->memory->data_bus = rotate_left_carry(cpu, cpu->memory->data_bus);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    sm83_write(cpu, cpu->reg.hl, cpu->memory->data_bus);
}
void rlc_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void rrc_r(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = rotate_right_carry(cpu, cpu->reg.byte_regs[args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rrc_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void rrc_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->memory->data_bus = rotate_right_carry(cpu, cpu->memory->data_bus);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    sm83_write(cpu, cpu->reg.hl, cpu->memory->data_bus);
}
void rrc_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void rl_r(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = rotate_left(cpu, cpu->reg.byte_regs[args[0].reg8]);

    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rl_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void rl_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->memory->data_bus = rotate_left(cpu, cpu->memory->data_bus);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    sm83_write(cpu, cpu->reg.hl, cpu->memory->data_bus);
}
void rl_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

void rr_r(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = rotate_right(cpu, cpu->reg.byte_regs[args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void rr_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void rr_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->memory->data_bus = rotate_right(cpu, cpu->memory->data_bus);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    sm83_write(cpu, cpu->reg.hl, cpu->memory->data_bus);
}
void rr_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

// Shift left arithmetic
void sla_r(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = sla(cpu, cpu->reg.byte_regs[args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void sla_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void sla_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->memory->data_bus = sla(cpu, cpu->memory->data_bus);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    sm83_write(cpu, cpu->reg.hl, cpu->memory->data_bus);
}
void sla_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

// Shift right arithmetic
void sra_r(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = sra(cpu, cpu->reg.byte_regs[args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void sra_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void sra_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->memory->data_bus = sra(cpu, cpu->memory->data_bus);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    sm83_write(cpu, cpu->reg.hl, cpu->memory->data_bus);
}
void sra_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

// Swap nibbles
void swap_r(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = swap(cpu, cpu->reg.byte_regs[args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void swap_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void swap_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->memory->data_bus = swap(cpu, cpu->memory->data_bus);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    sm83_write(cpu, cpu->reg.hl, cpu->memory->data_bus);
}
void swap_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

// Shift right logical
void srl_r(SM83 *cpu, InstructionArg *args)
{
    cpu->reg.byte_regs[args[0].reg8] = srl(cpu, cpu->reg.byte_regs[args[0].reg8]);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    fetch(cpu);
}

void srl_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void srl_hl_m1(SM83 *cpu, InstructionArg *args)
{
    cpu->memory->data_bus = srl(cpu, cpu->memory->data_bus);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 0;
    sm83_write(cpu, cpu->reg.hl, cpu->memory->data_bus);
}
void srl_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

// Test bit
void bit_b_r(SM83 *cpu, InstructionArg *args)
{
    u8 bit_to_test = args[0].data;
    u8 reg = cpu->reg.byte_regs[args[1].reg8];

    u8 carry_flag = cpu->reg.flags.c;

    cpu->reg.flags.z = ((reg & bit_to_test) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = carry_flag;
    fetch(cpu);
}

void bit_b_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void bit_b_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u8 bit_to_test = args[0].data;

    u8 carry_flag = cpu->reg.flags.c;

    cpu->reg.flags.z = ((cpu->memory->data_bus & bit_to_test) == 0);
    cpu->reg.flags.n = 0;
    cpu->reg.flags.h = 1;
    cpu->reg.flags.c = carry_flag;
    fetch(cpu);
}
// Reset bit
void res_b_r(SM83 *cpu, InstructionArg *args)
{
    u8 bit = args[0].data;
    u8 reg_index = args[1].reg8;
    cpu->reg.byte_regs[reg_index] = reset_bit(cpu->reg.byte_regs[reg_index], bit);
    fetch(cpu);
}

void res_b_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void res_b_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u8 bit = args[0].data;
    u8 z = reset_bit(cpu->memory->data_bus, bit);
    sm83_write(cpu, cpu->reg.hl, z);
}
void res_b_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}

// Set bit
void set_b_r(SM83 *cpu, InstructionArg *args)
{
    u8 bit = args[0].data;
    u8 reg_index = args[1].reg8;
    cpu->reg.byte_regs[reg_index] = set_bit(cpu->reg.byte_regs[reg_index], bit);
    fetch(cpu);
}

void set_b_hl_m0(SM83 *cpu, InstructionArg *args)
{
    sm83_read_request(cpu, cpu->reg.hl);
}
void set_b_hl_m1(SM83 *cpu, InstructionArg *args)
{
    u8 bit = args[0].data;
    u8 z = set_bit(cpu->memory->data_bus, bit);
    sm83_write(cpu, cpu->reg.hl, z);
}
void set_b_hl_m2(SM83 *cpu, InstructionArg *args)
{
    fetch(cpu);
}
