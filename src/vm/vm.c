#include "vm.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_CALL_STACK 1024

struct VM {
    Bytecode* bytecode;
    size_t pc;                
    Value registers[16];      
    int running;
    Frame* call_stack[MAX_CALL_STACK];
    size_t call_stack_top;
    size_t return_addresses[MAX_CALL_STACK];
};

static void vm_init_registers(VM* vm);
static void vm_execute_instruction(VM* vm, Instruction inst);
static void vm_push_frame(VM* vm, Frame* frame, size_t return_address);
static void vm_pop_frame(VM* vm);

VM* vm_create(Bytecode* bytecode) {
    VM* vm = (VM*)malloc(sizeof(VM));
    if (!vm) {
        fprintf(stderr, "Failed to allocate VM\n");
        exit(1);
    }
    
    vm->bytecode = bytecode;
    vm->pc = 0;
    vm->running = 1;
    vm->call_stack_top = 0;

    // Initialize all registers properly
    vm_init_registers(vm);

    for (size_t i = 0; i < MAX_CALL_STACK; i++) {
        vm->call_stack[i] = NULL;
        vm->return_addresses[i] = 0;
    }

    return vm;
}

Value vm_get_register_value(const VM* vm, int reg_index) {
    Value err = {.type = VAL_NONE};
    if (!vm || reg_index < 0 || reg_index >= 16) {
        return err;
    }
    return vm->registers[reg_index];
}

void vm_destroy(VM* vm) {
    if (!vm) return;

    /* Pop any remaining frames. */
    while (vm->call_stack_top > 0) {
        vm_pop_frame(vm);
    }

    free(vm);
}

/**
 * Reset all registers to VAL_NONE.
 */
static void vm_init_registers(VM* vm) {
    if (!vm) return;
    for (int i = 0; i < 16; i++) {
        vm->registers[i].type = VAL_NONE;
        vm->registers[i].as.int_val = 0;  // Initialize integer value to 0
    }
}

/**
 * Main execution loop.
 */
void vm_run(VM* vm) {
    while (vm->running && vm->pc < vm->bytecode->instruction_count) {
        Instruction inst = vm->bytecode->instructions[vm->pc];
        vm_execute_instruction(vm, inst);
    }
}

/**
 * Execute a single instruction.
 */
static void vm_execute_instruction(VM* vm, Instruction inst) {
    switch (inst.opcode) {
        case OP_NOP:
            vm->pc++;
            break;

        case OP_LOAD_CONST: {
            int reg_index = inst.operand1;
            if (reg_index < 0 || reg_index >= 16) {
                fprintf(stderr, "Invalid register index %d\n", reg_index);
                vm->running = 0;
                return;
            }
            vm->registers[reg_index].type = VAL_INT;
            vm->registers[reg_index].as.int_val = inst.operand2;
            vm->pc++;
        } break;

        case OP_ADD: {
            /* operand1: dest, operand2: src1, operand3: src2 */
            int rd = inst.operand1;
            int rs1 = inst.operand2;
            int rs2 = inst.operand3;
            if (rd < 0 || rd >= 16 || rs1 < 0 || rs1 >= 16 || rs2 < 0 || rs2 >= 16) {
                fprintf(stderr, "Invalid register index in OP_ADD\n");
                vm->running = 0;
                return;
            }
            /* For simplicity, we assume they are VAL_INT. In a real VM, you'd check. */
            if (vm->registers[rs1].type == VAL_INT && vm->registers[rs2].type == VAL_INT) {
                vm->registers[rd].type = VAL_INT;
                vm->registers[rd].as.int_val =
                    vm->registers[rs1].as.int_val + vm->registers[rs2].as.int_val;
            } else {
                /* Handle type errors or handle floats, etc. */
                fprintf(stderr, "OP_ADD type mismatch.\n");
                vm->running = 0;
            }
            vm->pc++;
        } break;

        case OP_SUB: {
            int rd = inst.operand1;
            int rs1 = inst.operand2;
            int rs2 = inst.operand3;
            if (rd < 0 || rd >= 16 || rs1 < 0 || rs1 >= 16 || rs2 < 0 || rs2 >= 16) {
                fprintf(stderr, "Invalid register index in OP_SUB\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[rs1].type == VAL_INT && vm->registers[rs2].type == VAL_INT) {
                vm->registers[rd].type = VAL_INT;
                vm->registers[rd].as.int_val =
                    vm->registers[rs1].as.int_val - vm->registers[rs2].as.int_val;
            } else {
                fprintf(stderr, "OP_SUB type mismatch.\n");
                vm->running = 0;
            }
            vm->pc++;
        } break;

        case OP_MUL: {
            int rd = inst.operand1;
            int rs1 = inst.operand2;
            int rs2 = inst.operand3;
            if (rd < 0 || rd >= 16 || rs1 < 0 || rs1 >= 16 || rs2 < 0 || rs2 >= 16) {
                fprintf(stderr, "Invalid register index in OP_MUL\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[rs1].type == VAL_INT && vm->registers[rs2].type == VAL_INT) {
                vm->registers[rd].type = VAL_INT;
                vm->registers[rd].as.int_val =
                    vm->registers[rs1].as.int_val * vm->registers[rs2].as.int_val;
            } else {
                fprintf(stderr, "OP_MUL type mismatch.\n");
                vm->running = 0;
            }
            vm->pc++;
        } break;

        case OP_DIV: {
            int rd = inst.operand1;
            int rs1 = inst.operand2;
            int rs2 = inst.operand3;
            if (rd < 0 || rd >= 16 || rs1 < 0 || rs1 >= 16 || rs2 < 0 || rs2 >= 16) {
                fprintf(stderr, "Invalid register index in OP_DIV\n");
                vm->running = 0;
                return;
            }
            if (vm->registers[rs2].type == VAL_INT && vm->registers[rs2].as.int_val == 0) {
                fprintf(stderr, "Division by zero at PC %zu\n", vm->pc);
                vm->running = 0;
            } else if (vm->registers[rs1].type == VAL_INT && vm->registers[rs2].type == VAL_INT) {
                vm->registers[rd].type = VAL_INT;
                vm->registers[rd].as.int_val =
                    vm->registers[rs1].as.int_val / vm->registers[rs2].as.int_val;
            } else {
                fprintf(stderr, "OP_DIV type mismatch.\n");
                vm->running = 0;
            }
            vm->pc++;
        } break;

        case OP_JUMP:
            vm->pc = (size_t)inst.operand1;
            break;

        case OP_JUMP_IF_ZERO: {
            int rs = inst.operand2;
            if (rs < 0 || rs >= 16) {
                fprintf(stderr, "Invalid register index in OP_JUMP_IF_ZERO\n");
                vm->running = 0;
                return;
            }
            
            // Ensure register contains an integer
            if (vm->registers[rs].type != VAL_INT) {
                fprintf(stderr, "Type error: JUMP_IF_ZERO requires integer register (got type %d)\n", 
                        vm->registers[rs].type);
                vm->running = 0;
                return;
            }
            
            if (vm->registers[rs].as.int_val == 0) {
                // Important: inst.operand1 is the absolute target PC
                size_t target = (size_t)inst.operand1;
                if (target >= vm->bytecode->instruction_count) {
                    fprintf(stderr, "Jump target out of bounds: %zu >= %zu\n", 
                            target, vm->bytecode->instruction_count);
                    vm->running = 0;
                    return;
                }
                vm->pc = target;
            } else {
                vm->pc++;
            }
        } break;

        case OP_CALL: {
            /* operand1 might be the function address or index in the instruction array. */
            size_t func_addr = (size_t)inst.operand1;
            if (func_addr >= vm->bytecode->instruction_count) {
                fprintf(stderr, "Invalid function address in OP_CALL: %zu\n", func_addr);
                vm->running = 0;
                return;
            }
            /* For demonstration, we'll create a new frame with 4 locals. 
               In a real language, you'd figure out the number from the function's signature. */
            Frame* new_frame = frame_create(4, 
                vm->call_stack_top > 0 ? vm->call_stack[vm->call_stack_top - 1] : NULL);
            vm_push_frame(vm, new_frame, vm->pc + 1);

            /* Jump to function entry. */
            vm->pc = func_addr;
        } break;

        case OP_RET: {
            /* Pop the current frame, jump back to the return address. */
            vm_pop_frame(vm);
        } break;

        case OP_HALT:
            vm->running = 0;
            break;

        default:
            fprintf(stderr, "Unknown opcode %d at PC %zu\n", inst.opcode, vm->pc);
            vm->running = 0;
            break;
    }
}

/**
 * Push a new frame onto the call stack along with a return address.
 */
static void vm_push_frame(VM* vm, Frame* frame, size_t return_address) {
    if (vm->call_stack_top >= MAX_CALL_STACK) {
        fprintf(stderr, "Call stack overflow!\n");
        vm->running = 0;
        return;
    }
    vm->call_stack[vm->call_stack_top] = frame;
    vm->return_addresses[vm->call_stack_top] = return_address;
    vm->call_stack_top++;
}

/**
 * Pop the top frame from the call stack and set the PC to the stored return address.
 */
static void vm_pop_frame(VM* vm) {
    if (vm->call_stack_top == 0) {
        /* No frame to pop => error or just halt. */
        fprintf(stderr, "Call stack underflow in OP_RET!\n");
        vm->running = 0;
        return;
    }
    vm->call_stack_top--;

    Frame* top_frame = vm->call_stack[vm->call_stack_top];
    size_t ret_addr = vm->return_addresses[vm->call_stack_top];

    /* Destroy the popped frame. */
    frame_destroy(top_frame);
    vm->call_stack[vm->call_stack_top] = NULL;

    /* Return to the saved address. */
    vm->pc = ret_addr;
}

/**
 * Optional debugging function to show the contents of the 16 registers.
 */
void vm_dump_registers(const VM* vm) {
    for (int i = 0; i < 16; i++) {
        const Value* v = &vm->registers[i];
        switch (v->type) {
            case VAL_INT:
                printf("R%d: INT(%lld)\n", i, (long long)v->as.int_val);
                break;
            case VAL_FLOAT:
                printf("R%d: FLOAT(%f)\n", i, v->as.float_val);
                break;
            case VAL_BOOL:
                printf("R%d: BOOL(%s)\n", i, v->as.bool_val ? "true" : "false");
                break;
            case VAL_NONE:
                printf("R%d: NONE\n", i);
                break;
        }
    }
}
