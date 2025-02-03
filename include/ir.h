#ifndef IR_H
#define IR_H

#include <stddef.h>
#include "../include/ast.h"

/**
 * @brief Enumerates a few IR opcodes for demonstration
 */
typedef enum {
    IR_NOP,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_LOAD_CONST,
    IR_STORE,
    IR_LOAD_VAR,
    IR_JUMP,
    IR_JUMP_IF_FALSE,
    /* ... more instructions as needed ... */
} IROpcode;

/**
 * @brief A single IR instruction
 */
typedef struct {
    IROpcode op;
    int dest;   /* destination register or slot */
    int src1;   /* first source register or immediate index */
    int src2;   /* second source, if needed */
    double fval;   /* for constants, store numeric. or store int. or string index. */
    char*  sval;   /* if storing a string or identifier, etc. */
} IRInstr;

/**
 * @brief IR Program container
 */
typedef struct {
    IRInstr* instrs;
    size_t instr_count;
    size_t instr_capacity;
} IRProgram;

/**
 * @brief Create an empty IR program
 */
IRProgram* ir_create_program(void);

/**
 * @brief Destroy an IR program, free instructions
 */
void ir_destroy_program(IRProgram* prog);

/**
 * @brief Generate IR from AST
 * This is a simple example stub for demonstration.
 */
void ir_generate_from_ast(AstNode* root, IRProgram* prog);

/**
 * @brief Perform IR optimizations (stub)
 */
void ir_optimize(IRProgram* prog);

#endif /* IR_H */
