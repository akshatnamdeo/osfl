#include "bytecode.h"
#include <stdlib.h>
#include <string.h>
#include "../include/vm_common.h"

#define INITIAL_INSTRUCTION_CAPACITY 64
#define INITIAL_CONSTANT_POOL_CAPACITY 8

/**
	 * Create an empty Bytecode struct with preallocated capacities.
	 */
Bytecode* bytecode_create(void) {
		Bytecode* bc = (Bytecode*)malloc(sizeof(Bytecode));
		if (!bc) return NULL;
		bc->instruction_count = 0;
		bc->instruction_capacity = INITIAL_INSTRUCTION_CAPACITY;
		bc->instructions = (Instruction*)malloc(bc->instruction_capacity * sizeof(Instruction));
		// Initialize the constant pool.
		bc->constant_pool.count = 0;
		bc->constant_pool.capacity = INITIAL_CONSTANT_POOL_CAPACITY;
		bc->constant_pool.strings = (char**)malloc(bc->constant_pool.capacity * sizeof(char*));
		return bc;
}

/**
	 * Free the instructions array, free all strings in the constant pool,
	 * free the constant pool array, and then free the Bytecode struct.
	 */
void bytecode_destroy(Bytecode* bc) {
		if (!bc) return;
		free(bc->instructions);
		for (size_t i = 0; i < bc->constant_pool.count; i++) {
				free(bc->constant_pool.strings[i]);
		}
		free(bc->constant_pool.strings);
		free(bc);
}

/**
	 * Helper: Grow the instructions array if needed.
	 */
static void grow_instructions(Bytecode* bc) {
		if (bc->instruction_count >= bc->instruction_capacity) {
				bc->instruction_capacity *= 2;
				bc->instructions = (Instruction*)realloc(bc->instructions, bc->instruction_capacity * sizeof(Instruction));
		}
}

/**
	 * Add an instruction; uses grow_instructions() to double capacity when needed.
	 */
void bytecode_add_instruction(Bytecode* bc, VMOpcode opcode, int op1, int op2, int op3) {
		if (!bc) return;
		grow_instructions(bc);
		Instruction instr;
		instr.opcode = opcode;
		instr.operand1 = op1;
		instr.operand2 = op2;
		instr.operand3 = op3;
		instr.operand4 = 0;  // default value for the fourth operand
		bc->instructions[bc->instruction_count++] = instr;
}

void bytecode_add_instruction_ex(Bytecode* bc, VMOpcode opcode, int op1, int op2, int op3, int op4) {
		if (!bc) return;
		grow_instructions(bc);
		Instruction instr;
		instr.opcode = opcode;
		instr.operand1 = op1;
		instr.operand2 = op2;
		instr.operand3 = op3;
		instr.operand4 = op4;
		bc->instructions[bc->instruction_count++] = instr;
}

/**
	 * Helper: Grow the constant pool array if needed.
	 */
static void grow_constant_pool(ConstantPool* cp) {
		if (cp->count >= cp->capacity) {
				cp->capacity *= 2;
				cp->strings = (char**)realloc(cp->strings, cp->capacity * sizeof(char*));
		}
}

/**
	 * Add a string to the constant pool (interning it) and return its index.
	 */
int bytecode_add_constant_str(Bytecode* bc, const char* str) {
		if (!bc || !str) return -1;
		ConstantPool* cp = &bc->constant_pool;
		grow_constant_pool(cp);
		cp->strings[cp->count] = strdup(str);
		return (int)(cp->count++);
}
