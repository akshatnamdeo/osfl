#ifndef BYTECODE_H
#define BYTECODE_H

#include <stddef.h>
#include "../vm/vm.h"  /* We reuse the Instruction, VMOpcode, Bytecode definitions from vm.h */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create an empty Bytecode object with some initial capacity.
 */
Bytecode* bytecode_create(void);

/**
 * @brief Free the Bytecode object, including its instructions array.
 */
void bytecode_destroy(Bytecode* bc);

/**
 * @brief Add an instruction to the Bytecode array. Automatically grows capacity if needed.
 */
void bytecode_add_instruction(Bytecode* bc, VMOpcode opcode, int op1, int op2, int op3);

/**
 * @brief Add an instruction to the Bytecode array EXTENDED. Automatically grows capacity if needed.
 */
void bytecode_add_instruction_ex(Bytecode* bc, VMOpcode opcode, int op1, int op2, int op3, int op4);


#ifdef __cplusplus
}
#endif

#endif /* BYTECODE_H */
