#include <log.h>
#include <r4300i.h>
#include "ir_context.h"

ir_context_t ir_context;

void ir_context_reset() {
    for (int i = 0; i < 32; i++) {
        ir_context.guest_gpr_to_value[i] = NULL;
    }

    memset(ir_context.ir_cache, 0, sizeof(ir_instruction_t) * IR_CACHE_SIZE);
    memset(ir_context.ir_flush_cache, 0, sizeof(ir_instruction_flush_t) * IR_FLUSH_CACHE_SIZE);

    ir_context.ir_cache[0].type = IR_SET_CONSTANT;
    ir_context.ir_cache[0].set_constant.type = VALUE_TYPE_U64;
    ir_context.ir_cache[0].set_constant.value_u64 = 0;
    ir_context.guest_gpr_to_value[0] = &ir_context.ir_cache[0];

    ir_context.ir_cache_index = 1;
    ir_context.ir_flush_cache_index = 0;

    ir_context.ir_cache_head = &ir_context.ir_cache[0];
    ir_context.ir_cache_tail = &ir_context.ir_cache[0];

    ir_context.block_end_pc_compiled = false;
    ir_context.block_end_pc_ir_emitted = false;
}

const char* val_type_to_str(ir_value_type_t type) {
    switch (type) {
        case VALUE_TYPE_U8:
            return "U8";
        case VALUE_TYPE_S8:
            return "S8";
        case VALUE_TYPE_S16:
            return "S16";
        case VALUE_TYPE_U16:
            return "U16";
        case VALUE_TYPE_S32:
            return "S32";
        case VALUE_TYPE_U32:
            return "U32";
        case VALUE_TYPE_U64:
            return "U64";
        case VALUE_TYPE_S64:
            return "S64";
    }
}

const char* cond_to_str(ir_condition_t condition) {
    switch (condition) {
        case CONDITION_NOT_EQUAL:
            return "!=";
        case CONDITION_EQUAL:
            return "==";
        case CONDITION_LESS_THAN_SIGNED:
        case CONDITION_LESS_THAN_UNSIGNED:
            return "<";
        case CONDITION_GREATER_THAN_SIGNED:
        case CONDITION_GREATER_THAN_UNSIGNED:
            return ">";
        case CONDITION_LESS_OR_EQUAL_TO_SIGNED:
        case CONDITION_LESS_OR_EQUAL_TO_UNSIGNED:
            return "<=";
        case CONDITION_GREATER_OR_EQUAL_TO_SIGNED:
        case CONDITION_GREATER_OR_EQUAL_TO_UNSIGNED:
            return ">=";
    }
}

void ir_instr_to_string(ir_instruction_t* instr, char* buf, size_t buf_size) {

    if (instr->type != IR_STORE && instr->type != IR_SET_COND_BLOCK_EXIT_PC && instr->type != IR_SET_BLOCK_EXIT_PC && instr->type != IR_NOP && instr->type != IR_FLUSH_GUEST_REG) {
        int written = snprintf(buf, buf_size, "v%d = ", instr->index);
        buf += written;
        buf_size -= written;
    }

    switch (instr->type) {
        case IR_NOP:
            snprintf(buf, buf_size, "");
            break;
        case IR_SET_CONSTANT:
            switch (instr->set_constant.type) {
                case VALUE_TYPE_U8:
                    snprintf(buf, buf_size, "0x%04X ;%u", (u16)instr->set_constant.value_u8, instr->set_constant.value_u8);
                    break;
                case VALUE_TYPE_S8:
                    snprintf(buf, buf_size, "0x%04X ;%d", (u16)instr->set_constant.value_s8, instr->set_constant.value_s8);
                    break;
                case VALUE_TYPE_S16:
                    snprintf(buf, buf_size, "0x%04X ;%d", (u16)instr->set_constant.value_s16, instr->set_constant.value_s16);
                    break;
                case VALUE_TYPE_U16:
                    snprintf(buf, buf_size, "0x%04X ;%u", instr->set_constant.value_u16, instr->set_constant.value_u16);
                    break;
                case VALUE_TYPE_S32:
                    snprintf(buf, buf_size, "0x%08X ;%d", (u32)instr->set_constant.value_s32, instr->set_constant.value_s32);
                    break;
                case VALUE_TYPE_U32:
                    snprintf(buf, buf_size, "0x%08X ;%u", instr->set_constant.value_u32, instr->set_constant.value_u32);
                    break;
                case VALUE_TYPE_U64:
                    snprintf(buf, buf_size, "0x%016lX ;%lu", instr->set_constant.value_u64, instr->set_constant.value_u64);
                    break;
                case VALUE_TYPE_S64:
                    snprintf(buf, buf_size, "0x%016lX ;%ld", instr->set_constant.value_s64, instr->set_constant.value_s64);
                    break;
            }
            break;
        case IR_OR:
            snprintf(buf, buf_size, "v%d | v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_XOR:
            snprintf(buf, buf_size, "v%d ^ v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_AND:
            snprintf(buf, buf_size, "v%d & v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_NOT:
            snprintf(buf, buf_size, "~v%d", instr->unary_op.operand->index);
            break;
        case IR_ADD:
            snprintf(buf, buf_size, "v%d + v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_STORE:
            snprintf(buf, buf_size, "STORE(type = %s, address = v%d, value = v%d)", val_type_to_str(instr->store.type), instr->store.address->index, instr->store.value->index);
            break;
        case IR_LOAD:
            snprintf(buf, buf_size, "LOAD(type = %s, address = v%d)", val_type_to_str(instr->load.type), instr->load.address->index);
            break;
        case IR_GET_PTR:
            snprintf(buf, buf_size, "GETPTR(type = %s, ptr = %lx)", val_type_to_str(instr->set_ptr.type), instr->set_ptr.ptr);
            break;
        case IR_SET_PTR:
            snprintf(buf, buf_size, "SETPTR(type = %s, ptr = %lx, value = v%d)", val_type_to_str(instr->set_ptr.type), instr->set_ptr.ptr, instr->set_ptr.value->index);
            break;
        case IR_MASK_AND_CAST:
            snprintf(buf, buf_size, "mask_cast(%s, v%d)", val_type_to_str(instr->mask_and_cast.type), instr->mask_and_cast.operand->index);
            break;
        case IR_CHECK_CONDITION:
            snprintf(buf, buf_size, "v%d %s v%d",
                     instr->check_condition.operand1->index,
                     cond_to_str(instr->check_condition.condition),
                     instr->check_condition.operand2->index);
            break;
        case IR_SET_BLOCK_EXIT_PC:
            snprintf(buf, buf_size, "set_block_exit(v%d)", instr->unary_op.operand->index);
            break;
        case IR_SET_COND_BLOCK_EXIT_PC:
            snprintf(buf, buf_size, "set_block_exit(v%d, if_true = v%d, if_false = v%d)", instr->set_cond_exit_pc.condition->index, instr->set_cond_exit_pc.pc_if_true->index, instr->set_cond_exit_pc.pc_if_false->index);
            break;
        case IR_TLB_LOOKUP:
            snprintf(buf, buf_size, "tlb_lookup(v%d)", instr->tlb_lookup.virtual_address->index);
            break;
        case IR_LOAD_GUEST_REG:
            snprintf(buf, buf_size, "guest_gpr[%d]", instr->load_guest_reg.guest_reg);
            break;
        case IR_FLUSH_GUEST_REG:
            snprintf(buf, buf_size, "guest_gpr[%d] = v%d", instr->flush_guest_reg.guest_reg, instr->flush_guest_reg.value->index);
            break;
        case IR_SHIFT:
            switch (instr->shift.direction) {
                case SHIFT_DIRECTION_LEFT:
                    snprintf(buf, buf_size, "v%d << v%d", instr->shift.operand->index, instr->shift.amount->index);
                    break;
                case SHIFT_DIRECTION_RIGHT:
                    snprintf(buf, buf_size, "v%d >> v%d", instr->shift.operand->index, instr->shift.amount->index);
                    break;
            }
            break;
        case IR_COND_BLOCK_EXIT:
            snprintf(buf, buf_size, "exit_block_if(v%d)", instr->cond_block_exit.condition->index);
            break;
        case IR_MULTIPLY:
            snprintf(buf, buf_size, "(%s)v%d * (%s)v%d", val_type_to_str(instr->mult_div.mult_div_type), instr->mult_div.operand1->index, val_type_to_str(instr->mult_div.mult_div_type), instr->mult_div.operand2->index);
            break;
        case IR_DIVIDE:
            snprintf(buf, buf_size, "(%s)v%d / (%s)v%d", val_type_to_str(instr->mult_div.mult_div_type), instr->mult_div.operand1->index, val_type_to_str(instr->mult_div.mult_div_type), instr->mult_div.operand2->index);
            break;
        case IR_SUB:
            snprintf(buf, buf_size, "v%d - v%d", instr->bin_op.operand1->index, instr->bin_op.operand2->index);
            break;
        case IR_ERET:
            snprintf(buf, buf_size, "eret()");
            break;
    }
}

void update_guest_reg_mapping(u8 guest_reg, ir_instruction_t* value) {
    if (guest_reg > 0 && guest_reg < 32) {
        ir_context.guest_gpr_to_value[guest_reg] = value;
    }
}

ir_instruction_t* allocate_ir_instruction(ir_instruction_t instruction) {
    int index = ir_context.ir_cache_index++;
    ir_context.ir_cache[index] = instruction;
    ir_context.ir_cache[index].index = index;
    ir_context.ir_cache[index].dead_code = true; // Will be marked false at the dead code elimination stage
    ir_context.ir_cache[index].reg_alloc.allocated = false;
    ir_context.ir_cache[index].reg_alloc.host_reg = -1;
    ir_context.ir_cache[index].reg_alloc.spilled = false;
    ir_context.ir_cache[index].last_use = -1;
    return &ir_context.ir_cache[index];
}

ir_instruction_flush_t* allocate_ir_flush(ir_instruction_flush_t flush) {
    int index = ir_context.ir_flush_cache_index++;
    ir_context.ir_flush_cache[index] = flush;
    return &ir_context.ir_flush_cache[index];
}

ir_instruction_t* append_ir_instruction(ir_instruction_t instruction, u8 guest_reg) {
    ir_instruction_t* allocation = allocate_ir_instruction(instruction);

    allocation->next = NULL;
    allocation->prev = ir_context.ir_cache_tail;

    ir_context.ir_cache_tail->next = allocation;
    ir_context.ir_cache_tail = allocation;

    update_guest_reg_mapping(guest_reg, allocation);
    return allocation;
}

ir_instruction_t* insert_ir_instruction(ir_instruction_t* after, ir_instruction_t instruction) {
    if (after == NULL) {
        logfatal("insert_ir_instruction with null 'after'");
    }
    ir_instruction_t* old_next = after->next;
    if (old_next == NULL) {
        // Inserting at the end
        return append_ir_instruction(instruction, NO_GUEST_REG);
    } else {
        ir_instruction_t* allocation = allocate_ir_instruction(instruction);

        after->next = allocation;

        allocation->prev = after;
        allocation->next = old_next;

        old_next->prev = allocation;
        return allocation;
    }
}

ir_instruction_t* ir_emit_set_constant(ir_set_constant_t value, u8 guest_reg) {
    if (guest_reg == 0) {
        // v0 is always zero, don't emit anything, reuse
        return ir_context.ir_cache_head;
    }

    bool is_zero = false;
    switch (value.type) {
        case VALUE_TYPE_S8:
            is_zero = value.value_s8 == 0;
            break;
        case VALUE_TYPE_U8:
            is_zero = value.value_u8 == 0;
            break;
        case VALUE_TYPE_S16:
            is_zero = value.value_s16 == 0;
            break;
        case VALUE_TYPE_U16:
            is_zero = value.value_u16 == 0;
            break;
        case VALUE_TYPE_S32:
            is_zero = value.value_s32 == 0;
            break;
        case VALUE_TYPE_U32:
            is_zero = value.value_u32 == 0;
            break;
        case VALUE_TYPE_U64:
            is_zero = value.value_u64 == 0;
            break;
        case VALUE_TYPE_S64:
            is_zero = value.value_s64 == 0;
            break;
    }
    if (is_zero) {
        update_guest_reg_mapping(guest_reg, ir_context.ir_cache_head);
        return ir_context.ir_cache_head;
    }

    ir_instruction_t instruction;
    instruction.type = IR_SET_CONSTANT;
    instruction.set_constant = value;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_load_guest_reg(u8 guest_reg) {
    if (guest_reg > 31) {
        logfatal("ir_emit_load_guest_reg: out of range guest reg value: %d", guest_reg);
    }

    if (ir_context.guest_gpr_to_value[guest_reg] != NULL) {
        return ir_context.guest_gpr_to_value[guest_reg];
    }

    ir_instruction_t instruction;
    instruction.type = IR_LOAD_GUEST_REG;
    instruction.load_guest_reg.guest_reg = guest_reg;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_flush_guest_reg(ir_instruction_t* last_usage, ir_instruction_t* value, u8 guest_reg) {
    if (last_usage == NULL) {
        logfatal("ir_emit_flush_guest_reg with null last_usage");
    }

    if (guest_reg == 0) {
        logfatal("Should never flush r0");
    }
    ir_instruction_t instruction;
    instruction.type = IR_FLUSH_GUEST_REG;
    instruction.flush_guest_reg.guest_reg = guest_reg;
    instruction.flush_guest_reg.value = value;

    return insert_ir_instruction(last_usage, instruction);
}

ir_instruction_t* ir_emit_or(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_OR;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_xor(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_XOR;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_not(ir_instruction_t* operand, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_NOT;
    instruction.unary_op.operand = operand;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_boolean_not(ir_instruction_t* operand, u8 guest_reg) {
    ir_instruction_t* mask = ir_emit_set_constant_u16(1, NO_GUEST_REG);
    ir_instruction_t* notted = ir_emit_not(operand, NO_GUEST_REG);
    return ir_emit_and(notted, mask, guest_reg);
}

ir_instruction_t* ir_emit_and(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_AND;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_sub(ir_instruction_t* minuend, ir_instruction_t* subtrahend, ir_value_type_t type, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_SUB;
    instruction.bin_op.operand1 = minuend;
    instruction.bin_op.operand2 = subtrahend;
    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_add(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_ADD;
    instruction.bin_op.operand1 = operand;
    instruction.bin_op.operand2 = operand2;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_shift(ir_instruction_t* operand, ir_instruction_t* amount, ir_value_type_t value_type, ir_shift_direction_t direction, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_SHIFT;
    instruction.shift.operand = operand;
    instruction.shift.amount = amount;
    instruction.shift.type = value_type;
    instruction.shift.direction = direction;

    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_store(ir_value_type_t type, ir_instruction_t* address, ir_instruction_t* value) {
    ir_instruction_t instruction;
    instruction.type = IR_STORE;
    instruction.store.type = type;
    instruction.store.address = address;
    instruction.store.value = value;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_load(ir_value_type_t type, ir_instruction_t* address, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_LOAD;
    instruction.load.type = type;
    instruction.load.address = address;
    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_get_ptr(ir_value_type_t type, void* ptr, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_GET_PTR;
    instruction.get_ptr.type = type;
    instruction.get_ptr.ptr = (uintptr_t)ptr;
    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_set_ptr(ir_value_type_t type, void* ptr, ir_instruction_t* value) {
    ir_instruction_t instruction;
    instruction.type = IR_SET_PTR;
    instruction.set_ptr.type = type;
    instruction.set_ptr.ptr = (uintptr_t)ptr;
    instruction.set_ptr.value = value;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_mask_and_cast(ir_instruction_t* operand, ir_value_type_t type, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_MASK_AND_CAST;
    instruction.mask_and_cast.type = type;
    instruction.mask_and_cast.operand = operand;
    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_check_condition(ir_condition_t condition, ir_instruction_t* operand1, ir_instruction_t* operand2, u8 guest_reg) {
    ir_instruction_t instruction;
    instruction.type = IR_CHECK_CONDITION;
    instruction.check_condition.condition = condition;
    instruction.check_condition.operand1 = operand1;
    instruction.check_condition.operand2 = operand2;
    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_conditional_set_block_exit_pc(ir_instruction_t* condition, ir_instruction_t* pc_if_true, ir_instruction_t* pc_if_false) {
    ir_context.block_end_pc_ir_emitted = true;
    ir_instruction_t instruction;
    instruction.type = IR_SET_COND_BLOCK_EXIT_PC;
    instruction.set_cond_exit_pc.condition = condition;
    instruction.set_cond_exit_pc.pc_if_true = pc_if_true;
    instruction.set_cond_exit_pc.pc_if_false = pc_if_false;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_conditional_block_exit(ir_instruction_t* condition, int index) {
    ir_instruction_t instruction;
    instruction.type = IR_COND_BLOCK_EXIT;
    instruction.cond_block_exit.condition = condition;
    instruction.cond_block_exit.block_length = index + 1;
    instruction.cond_block_exit.regs_to_flush = NULL;
    unimplemented(!ir_context.block_end_pc_ir_emitted, "Conditionally exiting block without knowing what PC should be");

    for (int i = 1; i < 32; i++) {
        ir_instruction_t* gpr_value = ir_context.guest_gpr_to_value[i];
        if (gpr_value) {
            // If it's just a load, no need to flush it back as it has not been modified
            if (gpr_value->type != IR_LOAD_GUEST_REG && gpr_value->load_guest_reg.guest_reg != i) {
                ir_instruction_flush_t* old_head = instruction.cond_block_exit.regs_to_flush;

                ir_instruction_flush_t flush;
                flush.next = old_head;
                flush.guest_gpr = i;
                flush.item = gpr_value;
                instruction.cond_block_exit.regs_to_flush = allocate_ir_flush(flush);
            }
        }
    }

    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_set_block_exit_pc(ir_instruction_t* address) {
    ir_context.block_end_pc_ir_emitted = true;
    ir_instruction_t instruction;
    instruction.type = IR_SET_BLOCK_EXIT_PC;
    instruction.unary_op.operand = address;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_interpreter_fallback(int num_instructions) {
    logfatal("Unimplemented: Fall back to interpreter for %d instructions", num_instructions);
}

ir_instruction_t* ir_emit_tlb_lookup(ir_instruction_t* virtual_address, u8 guest_reg, bus_access_t bus_access) {
    ir_instruction_t instruction;
    instruction.type = IR_TLB_LOOKUP;
    instruction.tlb_lookup.virtual_address = virtual_address;
    instruction.tlb_lookup.bus_access = bus_access;
    return append_ir_instruction(instruction, guest_reg);
}

ir_instruction_t* ir_emit_multiply(ir_instruction_t* multiplicand1, ir_instruction_t* multiplicand2, ir_value_type_t multiplicand_type) {
    ir_instruction_t instruction;
    instruction.type = IR_MULTIPLY;
    instruction.mult_div.operand1 = multiplicand1;
    instruction.mult_div.operand2 = multiplicand2;
    instruction.mult_div.mult_div_type = multiplicand_type;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_divide(ir_instruction_t* dividend, ir_instruction_t* divisor, ir_value_type_t divide_type) {
    ir_instruction_t instruction;
    instruction.type = IR_DIVIDE;
    instruction.mult_div.operand1 = dividend;
    instruction.mult_div.operand2 = divisor;
    instruction.mult_div.mult_div_type = divide_type;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}

ir_instruction_t* ir_emit_eret() {
    ir_context.block_end_pc_ir_emitted = true;
    ir_instruction_t instruction;
    instruction.type = IR_ERET;
    return append_ir_instruction(instruction, NO_GUEST_REG);
}