#ifndef BYTECODE_H
#define BYTECODE_H

#include "../../include/vm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// ConstantPool for interning string constants (e.g. native function names)
typedef struct {
		char** strings;
		size_t count;
		size_t capacity;
} ConstantPool;

// The Bytecode structure now includes an instructions array with a capacity
// and a constant pool.
typedef struct {
		Instruction* instructions;
		size_t instruction_count;
		size_t instruction_capacity;
		ConstantPool constant_pool;
} Bytecode;

Bytecode* bytecode_create(void);
void bytecode_destroy(Bytecode* bc);
void bytecode_add_instruction(Bytecode* bc, VMOpcode opcode, int op1, int op2, int op3);
void bytecode_add_instruction_ex(Bytecode* bc, VMOpcode opcode, int op1, int op2, int op3, int op4);

// Interns a string into the constant pool and returns its index.
int bytecode_add_constant_str(Bytecode* bc, const char* str);

#ifdef __cplusplus
}
#endif

#endif /* BYTECODE_H */
