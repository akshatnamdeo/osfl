#ifndef VM_H
#define VM_H

#include <stddef.h>
#include <stdint.h>

/* Define the opcodes for our VM */
typedef enum {
    OP_NOP,
    OP_LOAD_CONST,    // dest, constant value
    OP_ADD,           // dest, src1, src2
    OP_SUB,           // dest, src1, src2
    OP_MUL,           // dest, src1, src2
    OP_DIV,           // dest, src1, src2
    OP_JUMP,          // jump to address (operand1)
    OP_JUMP_IF_ZERO,  // if register operand2 is zero, jump to operand1
    OP_CALL,          // call function (not fully implemented here)
    OP_RET,           // return from function
    OP_HALT           // stop execution
} VMOpcode;

/* Instruction structure */
typedef struct {
    VMOpcode opcode;
    int operand1;
    int operand2;
    int operand3;
} Instruction;

/* Bytecode structure: an array of instructions */
typedef struct {
    Instruction* instructions;
    size_t instruction_count;
} Bytecode;

/* The VM structure */
typedef struct VM VM;

/* Create a new VM instance given bytecode */
VM* vm_create(Bytecode* bytecode);

/* Destroy a VM instance */
void vm_destroy(VM* vm);

/* Run the loaded bytecode */
void vm_run(VM* vm);

#endif // VM_H
