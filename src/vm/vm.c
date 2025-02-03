#include "vm.h"
#include <stdlib.h>
#include <stdio.h>

struct VM {
    Bytecode* bytecode;
    size_t pc;                /* program counter */
    int registers[16];        /* 16 general-purpose registers */
    int running;
    /* In a more complete implementation, you'd have a stack, frame pointer, etc. */
};

VM* vm_create(Bytecode* bytecode) {
    VM* vm = (VM*)malloc(sizeof(VM));
    if (!vm) {
        fprintf(stderr, "Failed to allocate VM\n");
        exit(1);
    }
    vm->bytecode = bytecode;
    vm->pc = 0;
    for (int i = 0; i < 16; i++) {
        vm->registers[i] = 0;
    }
    vm->running = 1;
    return vm;
}

void vm_destroy(VM* vm) {
    if (vm) {
        free(vm);
    }
}

void vm_run(VM* vm) {
    while (vm->running && vm->pc < vm->bytecode->instruction_count) {
        Instruction inst = vm->bytecode->instructions[vm->pc];
        // Uncomment the following line for debugging:
        // printf("PC: %zu, Opcode: %d, op1: %d, op2: %d, op3: %d\n", vm->pc, inst.opcode, inst.operand1, inst.operand2, inst.operand3);
        switch (inst.opcode) {
            case OP_NOP:
                vm->pc++;
                break;
            case OP_LOAD_CONST:
                /* operand1: destination register, operand2: constant value */
                vm->registers[inst.operand1] = inst.operand2;
                vm->pc++;
                break;
            case OP_ADD:
                /* operand1: dest, operand2: src1, operand3: src2 */
                vm->registers[inst.operand1] = vm->registers[inst.operand2] + vm->registers[inst.operand3];
                vm->pc++;
                break;
            case OP_SUB:
                vm->registers[inst.operand1] = vm->registers[inst.operand2] - vm->registers[inst.operand3];
                vm->pc++;
                break;
            case OP_MUL:
                vm->registers[inst.operand1] = vm->registers[inst.operand2] * vm->registers[inst.operand3];
                vm->pc++;
                break;
            case OP_DIV:
                if (vm->registers[inst.operand3] == 0) {
                    fprintf(stderr, "Division by zero at PC %zu\n", vm->pc);
                    vm->running = 0;
                } else {
                    vm->registers[inst.operand1] = vm->registers[inst.operand2] / vm->registers[inst.operand3];
                    vm->pc++;
                }
                break;
            case OP_JUMP:
                vm->pc = inst.operand1;
                break;
            case OP_JUMP_IF_ZERO:
                if (vm->registers[inst.operand2] == 0) {
                    vm->pc = inst.operand1;
                } else {
                    vm->pc++;
                }
                break;
            case OP_CALL:
                /* For simplicity, not fully implemented.
                   Normally, you would push the current frame, update the frame pointer, etc. */
                printf("OP_CALL not implemented in VM\n");
                vm->pc++;
                break;
            case OP_RET:
                printf("OP_RET not implemented in VM\n");
                vm->pc++;
                break;
            case OP_HALT:
                vm->running = 0;
                break;
            default:
                fprintf(stderr, "Unknown opcode %d at PC %zu\n", inst.opcode, vm->pc);
                vm->running = 0;
                break;
        }
    }
}
