#ifndef VM_COMMON_H
#define VM_COMMON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OP_NOP,
    OP_LOAD_CONST,          // load integer constant
    OP_LOAD_CONST_FLOAT,    // load float constant
    OP_LOAD_CONST_STR,      // load string constant
    OP_MOVE,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_EQ,                  // new: compare equality; result is int: 1 if equal, 0 otherwise
    OP_NEQ,
    OP_JUMP,
    OP_JUMP_IF_ZERO,
    OP_CALL,                // regular (bytecode) function call
    OP_CALL_NATIVE,         // native function call (extended instruction)
    OP_RET,
    OP_HALT,
    OP_NEWOBJ,
    OP_SETPROP,
    OP_GETPROP,
    OP_CORO_INIT,
    OP_CORO_YIELD,
    OP_CORO_RESUME
} VMOpcode;

typedef struct {
		VMOpcode opcode;
		int operand1;
		int operand2;
		int operand3;
		int operand4;
} Instruction;

#ifdef __cplusplus
}
#endif

#endif /* VM_COMMON_H */
