#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/ast.h"
#include "bytecode.h"

/* Forward declarations of local helper functions: */
static void compile_node(AstNode* node, Bytecode* bc);
static int compile_expression(AstNode* expr, Bytecode* bc);
static void add_function_entry(const char* name, int address);
static int lookup_function_address(const char* name);

/* A naive global for register allocation. */
static int next_register = 0;

/*
    A simple function table so that function declarations record their starting address.
*/
typedef struct {
    char* name;
    int address;
} FunctionEntry;

#define MAX_FUNCTIONS 64
static FunctionEntry function_table[MAX_FUNCTIONS];
static int function_count = 0;

static void add_function_entry(const char* name, int address) {
    if (function_count < MAX_FUNCTIONS) {
        function_table[function_count].name = strdup(name);
        function_table[function_count].address = address;
        function_count++;
    } else {
        fprintf(stderr, "Function table overflow\n");
        exit(1);
    }
}

static int lookup_function_address(const char* name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(function_table[i].name, name) == 0) {
            return function_table[i].address;
        }
    }
    return -1;
}

/**
 * Entry point: compile the AST into Bytecode, then return it.
 */
Bytecode* compiler_compile_ast(AstNode* root) {
    Bytecode* bc = bytecode_create();
    next_register = 0;
    function_count = 0; // reset the function table

    // Prepopulate function table with native functions.
    // Here we add "print" with a special address (-1) to indicate native.
    add_function_entry("print", -1);
    // Add other native functions as needed.

    compile_node(root, bc);

    bytecode_add_instruction(bc, OP_HALT, 0, 0, 0);
    return bc;
}

/**
 * Recursively compile AST nodes.
 */
static void compile_node(AstNode* node, Bytecode* bc) {
    if (!node) return;

    switch (node->type) {
        case AST_NODE_BLOCK: {
            for (size_t i = 0; i < node->as.block.statement_count; i++) {
                compile_node(node->as.block.statements[i], bc);
            }
        } break;
        case AST_NODE_VAR_DECL: {
            if (node->as.var_decl.initializer) {
                int r = compile_expression(node->as.var_decl.initializer, bc);
                (void)r;
            }
        } break;
        case AST_NODE_EXPR_STMT: {
            compile_expression(node->as.unary.expr, bc);
        } break;
        case AST_NODE_IF: {
            int cond_reg = compile_expression(node->as.if_stmt.condition, bc);
            size_t jump_index = bc->instruction_count;
            bytecode_add_instruction(bc, OP_JUMP_IF_ZERO, 0, cond_reg, 0);
            compile_node(node->as.if_stmt.then_branch, bc);
            size_t else_jump_index = (size_t)(-1);
            if (node->as.if_stmt.else_branch) {
                else_jump_index = bc->instruction_count;
                bytecode_add_instruction(bc, OP_JUMP, 0, 0, 0);
            }
            bc->instructions[jump_index].operand1 = (int)bc->instruction_count;
            if (node->as.if_stmt.else_branch) {
                compile_node(node->as.if_stmt.else_branch, bc);
                bc->instructions[else_jump_index].operand1 = (int)bc->instruction_count;
            }
        } break;
        case AST_NODE_WHILE_STMT: {
            size_t loop_start = bc->instruction_count;
            int cond_reg = compile_expression(node->as.while_stmt.condition, bc);
            size_t jump_index = bc->instruction_count;
            bytecode_add_instruction(bc, OP_JUMP_IF_ZERO, 0, cond_reg, 0);
            compile_node(node->as.while_stmt.body, bc);
            bytecode_add_instruction(bc, OP_JUMP, (int)loop_start, 0, 0);
            bc->instructions[jump_index].operand1 = (int)bc->instruction_count;
        } break;
        case AST_NODE_FOR_STMT: {
            compile_node(node->as.for_stmt.init, bc);
            size_t loop_start = bc->instruction_count;
            int cond_reg = compile_expression(node->as.for_stmt.condition, bc);
            size_t jump_index = bc->instruction_count;
            bytecode_add_instruction(bc, OP_JUMP_IF_ZERO, 0, cond_reg, 0);
            compile_node(node->as.for_stmt.body, bc);
            compile_node(node->as.for_stmt.increment, bc);
            bytecode_add_instruction(bc, OP_JUMP, (int)loop_start, 0, 0);
            bc->instructions[jump_index].operand1 = (int)bc->instruction_count;
        } break;
        case AST_NODE_RETURN_STMT: {
            int ret_reg = compile_expression(node->as.ret_stmt.expr, bc);
            (void)ret_reg;
            bytecode_add_instruction(bc, OP_RET, 0, 0, 0);
        } break;
        case AST_NODE_FUNC_DECL: {
            int func_address = (int)bc->instruction_count;
            add_function_entry(node->as.func_decl.func_name, func_address);
            // Optionally reset next_register for the function body.
            int saved_register = next_register;
            next_register = 0;
            compile_node(node->as.func_decl.body, bc);
            bytecode_add_instruction(bc, OP_RET, 0, 0, 0);
            next_register = saved_register;
        } break;
        case AST_NODE_CLASS_DECL: {
            for (size_t i = 0; i < node->as.class_decl.member_count; i++) {
                compile_node(node->as.class_decl.members[i], bc);
            }
        } break;
        default: {
            // For other node types, do nothing.
        } break;
    }

    if (node->next_sibling) {
        compile_node(node->next_sibling, bc);
    }
}

/**
 * Compile an expression node into bytecode and return the register index holding its result.
 */
static int compile_expression(AstNode* expr, Bytecode* bc) {
    if (!expr) return -1;

    switch (expr->type) {
        case AST_EXPR_LITERAL: {
            switch (expr->as.literal.literal_type) {
                case TOKEN_INTEGER: {
                    int r = next_register++;
                    long long val = expr->as.literal.i64_val;
                    bytecode_add_instruction(bc, OP_LOAD_CONST, r, (int)val, 0);
                    return r;
                }
                case TOKEN_FLOAT: {
                    int r = next_register++;
                    double f = expr->as.literal.f64_val;
                    bytecode_add_instruction(bc, OP_LOAD_CONST_FLOAT, r, 0, 0);
                    return r;
                }
                case TOKEN_STRING:
                case TOKEN_DOCSTRING:
                case TOKEN_REGEX: {
                    int r = next_register++;
                    bytecode_add_instruction(bc, OP_LOAD_CONST_STR, r, (int)(intptr_t)expr->as.literal.str_val, 0);
                    return r;
                }
                case TOKEN_BOOL_TRUE: {
                    int r = next_register++;
                    bytecode_add_instruction(bc, OP_LOAD_CONST, r, 1, 0);
                    return r;
                }
                case TOKEN_BOOL_FALSE: {
                    int r = next_register++;
                    bytecode_add_instruction(bc, OP_LOAD_CONST, r, 0, 0);
                    return r;
                }
                default:
                    break;
            }
        } break;
        case AST_EXPR_BINARY: {
            int left_reg = compile_expression(expr->as.binary.left, bc);
            int right_reg = compile_expression(expr->as.binary.right, bc);
            int dest_reg = next_register++;
            switch (expr->as.binary.op) {
                case TOKEN_PLUS:
                    bytecode_add_instruction(bc, OP_ADD, dest_reg, left_reg, right_reg);
                    return dest_reg;
                case TOKEN_MINUS:
                    bytecode_add_instruction(bc, OP_SUB, dest_reg, left_reg, right_reg);
                    return dest_reg;
                case TOKEN_STAR:
                    bytecode_add_instruction(bc, OP_MUL, dest_reg, left_reg, right_reg);
                    return dest_reg;
                case TOKEN_SLASH:
                    bytecode_add_instruction(bc, OP_DIV, dest_reg, left_reg, right_reg);
                    return dest_reg;
                default:
                    break;
            }
            return dest_reg;
        } break;
        case AST_EXPR_UNARY: {
            int operand_reg = compile_expression(expr->as.unary.expr, bc);
            int dest_reg = next_register++;
            switch (expr->as.unary.op) {
                case TOKEN_MINUS:
                    bytecode_add_instruction(bc, OP_LOAD_CONST, dest_reg, 0, 0);
                    bytecode_add_instruction(bc, OP_SUB, dest_reg, dest_reg, operand_reg);
                    return dest_reg;
                case TOKEN_PLUS:
                    return operand_reg;
                default:
                    return operand_reg;
            }
        } break;
        case AST_EXPR_IDENTIFIER: {
            const char* id = expr->as.ident.name;
            int func_addr = lookup_function_address(id);
            if (func_addr < 0) {
                fprintf(stderr, "[DEBUG] Identifier '%s' not found in function table; treating as native.\n", id);
                // For a native function, return a dummy register.
                return next_register++;
            }
            return func_addr;
        } break;
        case AST_EXPR_CALL: {
            if (expr->as.call.callee->type == AST_EXPR_IDENTIFIER) {
                const char* func_name = expr->as.call.callee->as.ident.name;
                int func_addr = lookup_function_address(func_name);
                if (func_addr < 0) {
                    // This is a native function call.
                    for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                        compile_expression(expr->as.call.args[i], bc);
                    }
                    int base_reg = next_register - expr->as.call.arg_count;
                    int dest_reg = next_register++;
                    // Use the extended instruction (with 4 operands) for native calls.
                    bytecode_add_instruction_ex(bc, OP_CALL_NATIVE, dest_reg, (int)(intptr_t)func_name, (int)expr->as.call.arg_count, base_reg);
                    return dest_reg;
                } else {
                    // Regular function call.
                    for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                        compile_expression(expr->as.call.args[i], bc);
                    }
                    bytecode_add_instruction(bc, OP_CALL, func_addr, 0, 0);
                    int ret_reg = next_register++;
                    return ret_reg;
                }
            } else {
                fprintf(stderr, "Unsupported callee type in function call.\n");
                return -1;
            }
        } break;
        case AST_EXPR_INTERPOLATION: {
            int inner_reg = compile_expression(expr->as.interpolation.expr, bc);
            bytecode_add_instruction(bc, OP_CALL, lookup_function_address("str"), 0, 0);
            int ret_reg = next_register++;
            return ret_reg;
        } break;
        default:
            break;
    }
    return -1;
}
