#include "bytecode.h"
#include <stdlib.h>

/**
 * Create an empty Bytecode struct with some default capacity.
 */
Bytecode* bytecode_create(void) {
    Bytecode* bc = (Bytecode*)malloc(sizeof(Bytecode));
    bc->instructions = NULL;
    bc->instruction_count = 0;
    return bc;
}

/**
 * Free the instructions array and the Bytecode struct.
 */
void bytecode_destroy(Bytecode* bc) {
    if (!bc) return;
    free(bc->instructions);
    free(bc);
}

/**
 * Helper to expand the instructions array if needed.
 */
static void grow_instructions(Bytecode* bc, size_t additional) {
    size_t old_count = bc->instruction_count;
    size_t new_count = old_count + additional;
    /* For a simple approach, we just reallocate each time we add. 
       You might do a capacity-based approach instead. */
    bc->instructions = (Instruction*)realloc(bc->instructions, new_count * sizeof(Instruction));
}

/**
 * Add an instruction, reallocate as needed.
 */
void bytecode_add_instruction(Bytecode* bc, VMOpcode opcode, int op1, int op2, int op3) {
    if (!bc) return;
    grow_instructions(bc, 1);
    Instruction instr;
    instr.opcode = opcode;
    instr.operand1 = op1;
    instr.operand2 = op2;
    instr.operand3 = op3;
    bc->instructions[bc->instruction_count] = instr;
    bc->instruction_count++;
}

void bytecode_add_instruction_ex(Bytecode* bc, VMOpcode opcode, int op1, int op2, int op3, int op4) {
    if (!bc) return;
    grow_instructions(bc, 1);
    Instruction instr;
    instr.opcode = opcode;
    instr.operand1 = op1;
    instr.operand2 = op2;
    instr.operand3 = op3;
    instr.operand4 = op4;
    bc->instructions[bc->instruction_count] = instr;
    bc->instruction_count++;
}