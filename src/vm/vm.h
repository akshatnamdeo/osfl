#ifndef VM_H
#define VM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "frame.h"

/* Define the opcodes for our VM. */
typedef enum {
    OP_NOP,
    OP_LOAD_CONST,     // dest reg, immediate value
    OP_ADD,            // dest, src1, src2
    OP_SUB,            // dest, src1, src2
    OP_MUL,            // dest, src1, src2
    OP_DIV,            // dest, src1, src2
    OP_JUMP,           // jump to PC = operand1
    OP_JUMP_IF_ZERO,   // if registers[operand2] == 0, jump to operand1
    OP_CALL,           // call function at operand1 (address or index)
    OP_RET,            // return from function
    OP_HALT            // stop execution
} VMOpcode;

/* Instruction structure */
typedef struct {
    VMOpcode opcode;
    int operand1;
    int operand2;
    int operand3;
    /* Potentially store additional metadata or debugging info. */
} Instruction;

/* Bytecode: an array of instructions plus a count. */
typedef struct {
    Instruction* instructions;
    size_t instruction_count;
} Bytecode;

/* 
 * The VM is a register-based machine with:
 * - Up to 16 registers (each storing a Value)
 * - A call stack of Frames for function calls
 * - A separate "pc" for the program counter
 * - A "running" flag
 */
typedef struct VM VM;

VM* vm_create(Bytecode* bytecode);
void vm_destroy(VM* vm);
void vm_run(VM* vm);

/* Debugging / utility: print the registers. */
void vm_dump_registers(const VM* vm);
Value vm_get_register_value(const VM* vm, int reg_index);

#endif // VM_H
