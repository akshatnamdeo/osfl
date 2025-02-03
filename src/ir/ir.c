#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/ir.h"

static void ir_grow_if_needed(IRProgram* prog) {
    if (prog->instr_count >= prog->instr_capacity) {
        size_t newcap = (prog->instr_capacity == 0) ? 16 : prog->instr_capacity * 2;
        IRInstr* newarr = (IRInstr*)realloc(prog->instrs, newcap * sizeof(IRInstr));
        if (!newarr) {
            fprintf(stderr, "IR allocation failed.\n");
            return;
        }
        prog->instrs = newarr;
        prog->instr_capacity = newcap;
    }
}

static void ir_emit(IRProgram* prog, IROpcode op, int dest, int src1, int src2, double fval, const char* sval) {
    ir_grow_if_needed(prog);
    IRInstr* instr = &prog->instrs[prog->instr_count++];
    instr->op = op;
    instr->dest = dest;
    instr->src1 = src1;
    instr->src2 = src2;
    instr->fval = fval;
    instr->sval = NULL;
    if (sval) {
        size_t len = strlen(sval);
        instr->sval = (char*)malloc(len+1);
        memcpy(instr->sval, sval, len);
        instr->sval[len] = '\0';
    }
}

IRProgram* ir_create_program(void) {
    IRProgram* prog = (IRProgram*)malloc(sizeof(IRProgram));
    if (!prog) return NULL;
    prog->instrs = NULL;
    prog->instr_count = 0;
    prog->instr_capacity = 0;
    return prog;
}

void ir_destroy_program(IRProgram* prog) {
    if (!prog) return;
    for (size_t i = 0; i < prog->instr_count; i++) {
        if (prog->instrs[i].sval) {
            free(prog->instrs[i].sval);
        }
    }
    free(prog->instrs);
    free(prog);
}

/* Forward declarations for AST -> IR */
static void ir_gen_node(AstNode* node, IRProgram* prog);

void ir_generate_from_ast(AstNode* root, IRProgram* prog) {
    if (!root || !prog) return;
    ir_gen_node(root, prog);
}

static void ir_gen_expr(AstNode* expr, IRProgram* prog) {
    if (!expr) return;
    switch (expr->type) {
        case AST_EXPR_LITERAL:
            if (expr->as.literal.literal_type == TOKEN_INTEGER) {
                ir_emit(prog, IR_LOAD_CONST, 0, 0, 0, (double)expr->as.literal.i64_val, NULL);
            } else if (expr->as.literal.literal_type == TOKEN_FLOAT) {
                ir_emit(prog, IR_LOAD_CONST, 0, 0, 0, expr->as.literal.f64_val, NULL);
            } else if (expr->as.literal.literal_type == TOKEN_STRING) {
                ir_emit(prog, IR_LOAD_CONST, 0, 0, 0, 0.0, expr->as.literal.str_val);
            } else {
                ir_emit(prog, IR_NOP, 0, 0, 0, 0.0, NULL);
            }
            break;
        case AST_EXPR_BINARY:
            ir_gen_expr(expr->as.binary.left, prog);
            ir_gen_expr(expr->as.binary.right, prog);
            if (expr->as.binary.op == TOKEN_PLUS) {
                ir_emit(prog, IR_ADD, 0, 0, 0, 0.0, NULL);
            } else if (expr->as.binary.op == TOKEN_MINUS) {
                ir_emit(prog, IR_SUB, 0, 0, 0, 0.0, NULL);
            } else if (expr->as.binary.op == TOKEN_STAR) {
                ir_emit(prog, IR_MUL, 0, 0, 0, 0.0, NULL);
            } else if (expr->as.binary.op == TOKEN_SLASH) {
                ir_emit(prog, IR_DIV, 0, 0, 0, 0.0, NULL);
            } else {
                ir_emit(prog, IR_NOP, 0, 0, 0, 0.0, NULL);
            }
            break;
        default:
            ir_emit(prog, IR_NOP, 0, 0, 0, 0.0, NULL);
            break;
    }
}

static void ir_gen_node(AstNode* node, IRProgram* prog) {
    while (node) {
        switch (node->type) {
            case AST_NODE_BLOCK:
                for (size_t i = 0; i < node->as.block.statement_count; i++) {
                    ir_gen_node(node->as.block.statements[i], prog);
                }
                break;
            case AST_NODE_VAR_DECL:
                /* For demonstration, generate IR_LOAD_CONST from initializer if any, then IR_STORE. */
                if (node->as.var_decl.initializer) {
                    ir_gen_expr(node->as.var_decl.initializer, prog);
                    ir_emit(prog, IR_STORE, 0, 0, 0, 0.0, node->as.var_decl.var_name);
                } else {
                    ir_emit(prog, IR_LOAD_CONST, 0, 0, 0, 0.0, NULL);
                    ir_emit(prog, IR_STORE, 0, 0, 0, 0.0, node->as.var_decl.var_name);
                }
                break;
            case AST_NODE_FUNC_DECL:
                /* In a real compiler, we might generate a function label, push a new IR block, etc. */
                ir_emit(prog, IR_NOP, 0, 0, 0, 0.0, "func_decl");
                ir_gen_node(node->as.func_decl.body, prog);
                break;
            case AST_NODE_IF_STMT:
                /* In real code, generate IR_JUMP_IF_FALSE to skip the then branch, etc. */
                ir_gen_expr(node->as.if_stmt.condition, prog);
                ir_emit(prog, IR_JUMP_IF_FALSE, 0, 0, 0, 0.0, "ELSE_LABEL");
                ir_gen_node(node->as.if_stmt.then_branch, prog);
                if (node->as.if_stmt.else_branch) {
                    ir_emit(prog, IR_JUMP, 0, 0, 0, 0.0, "END_IF");
                    /* define ELSE_LABEL */
                    ir_gen_node(node->as.if_stmt.else_branch, prog);
                    /* define END_IF */
                }
                break;
            case AST_NODE_EXPR_STMT:
            case AST_EXPR_BINARY:
            case AST_EXPR_UNARY:
            case AST_EXPR_LITERAL:
            case AST_EXPR_IDENTIFIER:
            case AST_EXPR_CALL:
            case AST_EXPR_INDEX:
            case AST_EXPR_MEMBER:
            case AST_EXPR_INTERPOLATION:
                ir_gen_expr(node, prog);
                break;
            default:
                ir_emit(prog, IR_NOP, 0, 0, 0, 0.0, "unhandled_node");
                break;
        }
        node = node->next_sibling;
    }
}

void ir_optimize(IRProgram* prog) {
    /* Very basic stub that does no real optimization. */
    (void)prog;
    /* You could run constant folding, dead code elimination, etc. */
}
