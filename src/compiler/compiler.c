#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/ast.h"
#include "../include/vm_common.h"
#include "../include/symbol_table.h"
#include "bytecode.h"

/* Forward declarations of local helper functions: */
static void compile_node(AstNode* node, Bytecode* bc);
static int compile_expression(AstNode* expr, Bytecode* bc);
static void add_function_entry(const char* name, int address);
static int lookup_function_address(const char* name);
static Scope* current_scope = NULL;

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

#define MAX_VARIABLES 64

// A simple mapping from variable name to register number.
// (For production code, you’d use a proper scoped symbol table.)
static char* variable_names[MAX_VARIABLES];
static int variable_count = 0;

// Lookup a variable name in the current mapping.
// Returns its register index if found; otherwise returns -1.
static int lookup_variable_register(const char* name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variable_names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

static int lookup_function_address(const char* name) {
    for (int i = 0; i < function_count; i++) {
        if (strcmp(function_table[i].name, name) == 0) {
            return function_table[i].address;
        }
    }
    return -1;
}

void dump_bytecode(const Bytecode* bc) {
    fprintf(stderr, "---- Bytecode Dump (instruction count: %zu) ----\n", bc->instruction_count);
    for (size_t i = 0; i < bc->instruction_count; i++) {
        const Instruction* inst = &bc->instructions[i];
        fprintf(stderr, "PC %zu: Opcode %d, op1=%d, op2=%d, op3=%d, op4=%d\n",
                i, inst->opcode, inst->operand1, inst->operand2, inst->operand3, inst->operand4);
    }
    fprintf(stderr, "---- Constant Pool Dump (count: %zu) ----\n", bc->constant_pool.count);
    for (size_t i = 0; i < bc->constant_pool.count; i++) {
        fprintf(stderr, "CP[%zu]: '%s'\n", i, bc->constant_pool.strings[i]);
    }
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
    dump_bytecode(bc);
    return bc;
}

/**
 * Recursively compile AST nodes.
 */
static void compile_node(AstNode* node, Bytecode* bc) {
    if (!node) return;

    switch (node->type) {
        case AST_NODE_FRAME: {
            printf("DEBUG: Found frame: %s\n", node->as.frame_decl.frame_name);
            // If this is the Main frame, handle it specially.
            if (strcmp(node->as.frame_decl.frame_name, "Main") == 0) {
                printf("DEBUG: Found Main frame\n");
                // First compile the frame contents normally.
                for (size_t i = 0; i < node->as.frame_decl.body_count; i++) {
                    compile_node(node->as.frame_decl.body_statements[i], bc);
                }
                // After compiling the frame, look up the main function and call it.
                int main_addr = lookup_function_address("main");
                if (main_addr >= 0) {
                    printf("DEBUG: Adding call to main() at address %d\n", main_addr);
                    bytecode_add_instruction(bc, OP_CALL, main_addr, 0, 0);
                    // Add HALT after main returns.
                    bytecode_add_instruction(bc, OP_HALT, 0, 0, 0);
                } else {
                    printf("ERROR: main() function not found in Main frame\n");
                }
            } else {
                // Regular frame compilation.
                for (size_t i = 0; i < node->as.frame_decl.body_count; i++) {
                    compile_node(node->as.frame_decl.body_statements[i], bc);
                }
            }
        } break;
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
            printf("DEBUG: Compiling function node: %s\n", node->as.func_decl.func_name);
            int func_address = (int)bc->instruction_count;
            add_function_entry(node->as.func_decl.func_name, func_address);
            
            // Save the old scope.
            Scope* old_scope = current_scope;
            // Create a new scope for this function.
            current_scope = scope_create(old_scope);
            if (!current_scope) {
                fprintf(stderr, "Failed to create function scope for '%s'.\n", node->as.func_decl.func_name);
                exit(1);
            }
            printf("DEBUG: Setting up symbol table for function '%s' with %d parameter(s).\n",
                   node->as.func_decl.func_name, node->as.func_decl.param_count);
            // For each parameter, add it to the scope with register i.
            for (int i = 0; i < (int)node->as.func_decl.param_count; i++) {
                if (!scope_add_symbol(current_scope, node->as.func_decl.param_names[i], SYMBOL_VAR, i)) {
                    fprintf(stderr, "Failed to add parameter '%s' to symbol table.\n", node->as.func_decl.param_names[i]);
                } else {
                    printf("DEBUG: Mapped parameter '%s' to register %d.\n", node->as.func_decl.param_names[i], i);
                }
            }
            // Reserve registers for parameters.
            int saved_register = next_register;
            next_register = node->as.func_decl.param_count;
            
            compile_node(node->as.func_decl.body, bc);
            bytecode_add_instruction(bc, OP_RET, 0, 0, 0);
            
            // Clean up the function scope.
            scope_destroy(current_scope);
            current_scope = old_scope;
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
                case TOKEN_EQ: {
                    // New case: equality comparison
                    bytecode_add_instruction(bc, OP_EQ, dest_reg, left_reg, right_reg);
                    return dest_reg;
                }
                case TOKEN_NEQ: {
                    // New case: inequality comparison
                    bytecode_add_instruction(bc, OP_NEQ, dest_reg, left_reg, right_reg);
                    return dest_reg;
                }
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
            Symbol* sym = NULL;
            if (current_scope != NULL) {
                sym = scope_lookup(current_scope, id);
            }
            if (sym != NULL) {
                // Found the symbol in the current scope; return its register.
                return sym->reg;
            }
            // Otherwise, fall back on function lookup.
            int func_addr = lookup_function_address(id);
            if (func_addr < 0) {
                fprintf(stderr, "[DEBUG] Identifier '%s' not found in function table or symbol table; returning dummy register.\n", id);
                return next_register++; // dummy; ideally, you would signal an error.
            }
            return func_addr;
        } break;
        case AST_EXPR_CALL: {
            if (expr->as.call.callee->type == AST_EXPR_IDENTIFIER) {
                const char* func_name = expr->as.call.callee->as.ident.name;
                int func_addr = lookup_function_address(func_name);
                if (func_addr < 0) {
                    // Native call branch (unchanged)
                    fprintf(stderr, "[DEBUG] Identifier '%s' not found in function table; treating as native.\n", func_name);
                    for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                        compile_expression(expr->as.call.args[i], bc);
                    }
                    int base_reg = next_register - expr->as.call.arg_count;
                    int dest_reg = next_register++;
                    int native_index = bytecode_add_constant_str(bc, func_name);
                    fprintf(stderr, "[DEBUG] Interned native function '%s' at constant pool index %d.\n", func_name, native_index);
                    bytecode_add_instruction_ex(bc, OP_CALL_NATIVE, dest_reg, native_index, (int)expr->as.call.arg_count, base_reg);
                    return dest_reg;
                } else {
                    // Regular function call.
                    int arg_count = (int)expr->as.call.arg_count;
                    int* arg_regs = (int*)malloc(arg_count * sizeof(int));
                    if (!arg_regs) {
                        fprintf(stderr, "Failed to allocate memory for function call arguments.\n");
                        exit(1);
                    }
                    // Compile each argument; store its register.
                    for (int i = 0; i < arg_count; i++) {
                        arg_regs[i] = compile_expression(expr->as.call.args[i], bc);
                    }
                    // Now, move each argument into the callee's expected registers (0, 1, 2, …).
                    for (int i = 0; i < arg_count; i++) {
                        bytecode_add_instruction(bc, OP_MOVE, i, arg_regs[i], 0);
                    }
                    free(arg_regs);
                    // Emit the call instruction.
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
