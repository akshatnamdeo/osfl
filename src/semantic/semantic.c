#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/semantic.h"

/* Utility for easy string duplication */
static char* sm_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* copy = (char*)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

void semantic_init(SemanticContext* ctx) {
    ctx->current_scope = scope_create(NULL); /* global scope */
    ctx->error_count = 0;
}

void semantic_cleanup(SemanticContext* ctx) {
    if (ctx->current_scope) {
        scope_destroy(ctx->current_scope);
        ctx->current_scope = NULL;
    }
    ctx->error_count = 0;
}

/* Forward declarations for internal usage */
static void analyze_node(AstNode* node, SemanticContext* ctx);
static void analyze_statement(AstNode* node, SemanticContext* ctx);
static void analyze_var_decl(AstNode* node, SemanticContext* ctx);
static void analyze_func_decl(AstNode* node, SemanticContext* ctx);
static void enter_scope(SemanticContext* ctx);
static void exit_scope(SemanticContext* ctx);

void semantic_analyze(AstNode* root, SemanticContext* ctx) {
    if (!root) return;
    /* You might treat root as AST_NODE_PROGRAM or a block. */
    analyze_node(root, ctx);

    /* Optionally do a separate pass for control flow. */
    semantic_control_flow_analysis(root, ctx);

    if (ctx->error_count > 0) {
        fprintf(stderr, "Semantic analysis finished with %d errors.\n", ctx->error_count);
    }
}

/* Example "dispatch" function */
static void analyze_node(AstNode* node, SemanticContext* ctx) {
    while (node) {
        switch (node->type) {
            case AST_NODE_VAR_DECL:
                analyze_var_decl(node, ctx);
                break;
            case AST_NODE_FUNC_DECL:
                analyze_func_decl(node, ctx);
                break;
            case AST_NODE_BLOCK: {
                enter_scope(ctx);
                for (size_t i = 0; i < node->as.block.statement_count; i++) {
                    analyze_node(node->as.block.statements[i], ctx);
                }
                exit_scope(ctx);
                break;
            }
            case AST_NODE_IF_STMT:
            case AST_NODE_WHILE_STMT:
            case AST_NODE_FOR_STMT:
            case AST_NODE_RETURN_STMT:
            case AST_NODE_EXPR_STMT:
                analyze_statement(node, ctx);
                break;
            case AST_NODE_CLASS_DECL:
                /* You can do logic for class members, etc. */
                scope_add_symbol(ctx->current_scope, node->as.class_decl.class_name, SYMBOL_CLASS, -1);
                /* parse class members in a new scope or the same, up to you. */
                break;

            /* Expression nodes might appear in a top-level list for script usage. */
            case AST_EXPR_BINARY:
            case AST_EXPR_UNARY:
            case AST_EXPR_LITERAL:
            case AST_EXPR_IDENTIFIER:
            case AST_EXPR_CALL:
            case AST_EXPR_INDEX:
            case AST_EXPR_MEMBER:
            case AST_EXPR_INTERPOLATION:
                (void)semantic_check_expr(node, ctx);
                break;

            default:
                break;
        }
        node = node->next_sibling; /* move to next sibling if any */
    }
}

/* Example for statements that are not var/func/class decls */
static void analyze_statement(AstNode* node, SemanticContext* ctx) {
    switch (node->type) {
        case AST_NODE_IF_STMT: {
            TypeInfo cond_type = semantic_check_expr(node->as.if_stmt.condition, ctx);
            if (cond_type.kind != SEMANTIC_TYPE_BOOL && cond_type.kind != SEMANTIC_TYPE_UNKNOWN) {
                fprintf(stderr, "Semantic error: if condition must be bool at %s:%d\n",
                        node->loc.file, node->loc.line);
                ctx->error_count++;
            }
            analyze_node(node->as.if_stmt.then_branch, ctx);
            analyze_node(node->as.if_stmt.else_branch, ctx);
            break;
        }
        case AST_NODE_WHILE_STMT: {
            TypeInfo cond_type = semantic_check_expr(node->as.while_stmt.condition, ctx);
            if (cond_type.kind != SEMANTIC_TYPE_BOOL && cond_type.kind != SEMANTIC_TYPE_UNKNOWN) {
                fprintf(stderr, "Semantic error: while condition must be bool at %s:%d\n",
                        node->loc.file, node->loc.line);
                ctx->error_count++;
            }
            analyze_node(node->as.while_stmt.body, ctx);
            break;
        }
        case AST_NODE_FOR_STMT: {
            if (node->as.for_stmt.init) {
                analyze_node(node->as.for_stmt.init, ctx);
            }
            if (node->as.for_stmt.condition) {
                TypeInfo cond_type = semantic_check_expr(node->as.for_stmt.condition, ctx);
                if (cond_type.kind != SEMANTIC_TYPE_BOOL && cond_type.kind != SEMANTIC_TYPE_UNKNOWN) {
                    fprintf(stderr, "Semantic error: for condition must be bool at %s:%d\n",
                            node->loc.file, node->loc.line);
                    ctx->error_count++;
                }
            }
            if (node->as.for_stmt.increment) {
                analyze_node(node->as.for_stmt.increment, ctx);
            }
            analyze_node(node->as.for_stmt.body, ctx);
            break;
        }
        case AST_NODE_RETURN_STMT:
            if (node->as.ret_stmt.expr) {
                (void)semantic_check_expr(node->as.ret_stmt.expr, ctx);
            }
            break;
        case AST_NODE_EXPR_STMT:
            (void)semantic_check_expr(node, ctx);
            break;
        default:
            /* fallback or error */
            break;
    }
}

static void analyze_var_decl(AstNode* node, SemanticContext* ctx) {
    /* node->as.var_decl has var_name, initializer, is_const */
    if (!scope_add_symbol(ctx->current_scope, node->as.var_decl.var_name,
                          node->as.var_decl.is_const ? SYMBOL_CONST : SYMBOL_VAR, -1))
    {
        fprintf(stderr, "Semantic error: duplicate variable '%s' in scope at %s:%d\n",
                node->as.var_decl.var_name, node->loc.file, node->loc.line);
        ctx->error_count++;
    }
    if (node->as.var_decl.initializer) {
        (void)semantic_check_expr(node->as.var_decl.initializer, ctx);
    }
}

static void analyze_func_decl(AstNode* node, SemanticContext* ctx) {
    /* Add symbol to scope */
    if (!scope_add_symbol(ctx->current_scope, node->as.func_decl.func_name, SYMBOL_FUNC, -1)) {
        fprintf(stderr, "Semantic error: duplicate function '%s' at %s:%d\n",
                node->as.func_decl.func_name, node->loc.file, node->loc.line);
        ctx->error_count++;
    }
    /* Create child scope for function body */
    enter_scope(ctx);
    /* Add parameters as symbols */
    for (size_t i = 0; i < node->as.func_decl.param_count; i++) {
        scope_add_symbol(ctx->current_scope, node->as.func_decl.param_names[i], SYMBOL_VAR, -1);
    }
    analyze_node(node->as.func_decl.body, ctx);
    exit_scope(ctx);
}

/* Enter a new scope (child) */
static void enter_scope(SemanticContext* ctx) {
    ctx->current_scope = scope_create(ctx->current_scope);
}

/* Exit the current scope */
static void exit_scope(SemanticContext* ctx) {
    Scope* old = ctx->current_scope;
    if (!old) return;
    ctx->current_scope = old->parent;
    scope_destroy(old);
}

/* --------------
   Expression Checking
   --------------*/

TypeInfo semantic_check_expr(AstNode* expr, SemanticContext* ctx) {
    TypeInfo result;
    result.kind = SEMANTIC_TYPE_UNKNOWN;
    result.custom_name = NULL;

    if (!expr) {
        return result;
    }

    switch (expr->type) {
        case AST_EXPR_LITERAL: {
            switch (expr->as.literal.literal_type) {
                case TOKEN_INTEGER: result.kind = SEMANTIC_TYPE_INT; break;
                case TOKEN_FLOAT:   result.kind = SEMANTIC_TYPE_FLOAT; break;
                case TOKEN_BOOL_TRUE:
                case TOKEN_BOOL_FALSE:
                    result.kind = SEMANTIC_TYPE_BOOL;
                    break;
                case TOKEN_STRING:
                case TOKEN_DOCSTRING:
                    result.kind = SEMANTIC_TYPE_STRING;
                    break;
                default:
                    result.kind = SEMANTIC_TYPE_UNKNOWN;
                    break;
            }
            return result;
        }
        case AST_EXPR_IDENTIFIER: {
            Symbol* sym = scope_lookup(ctx->current_scope, expr->as.ident.name);
            if (!sym) {
                fprintf(stderr, "Semantic error: undefined identifier '%s' at %s:%d\n",
                        expr->as.ident.name, expr->loc.file, expr->loc.line);
                ctx->error_count++;
                return result;
            }
            /* For demo, we won't store symbol->type, so we just guess. */
            /* Real code: result = sym->typeInfo; */
            return result;
        }
        case AST_EXPR_BINARY: {
            TypeInfo leftType = semantic_check_expr(expr->as.binary.left, ctx);
            TypeInfo rightType = semantic_check_expr(expr->as.binary.right, ctx);
            /* Simple numeric example: if either is float => result float, else int. */
            if (expr->as.binary.op == TOKEN_PLUS ||
                expr->as.binary.op == TOKEN_MINUS ||
                expr->as.binary.op == TOKEN_STAR ||
                expr->as.binary.op == TOKEN_SLASH) {
                if (leftType.kind == SEMANTIC_TYPE_FLOAT || rightType.kind == SEMANTIC_TYPE_FLOAT) {
                    result.kind = SEMANTIC_TYPE_FLOAT;
                } else {
                    result.kind = SEMANTIC_TYPE_INT;
                }
            } else if (expr->as.binary.op == TOKEN_AND ||
                       expr->as.binary.op == TOKEN_OR) {
                /* logical and/or => bool. */
                result.kind = SEMANTIC_TYPE_BOOL;
            }
            return result;
        }
        case AST_EXPR_UNARY: {
            TypeInfo sub = semantic_check_expr(expr->as.unary.expr, ctx);
            /* e.g. unary minus => numeric result, unary not => bool, etc. */
            if (expr->as.unary.op == TOKEN_MINUS && (sub.kind == SEMANTIC_TYPE_FLOAT || sub.kind == SEMANTIC_TYPE_INT)) {
                result.kind = sub.kind; 
            } else if (expr->as.unary.op == TOKEN_NOT) {
                result.kind = SEMANTIC_TYPE_BOOL;
            }
            return result;
        }
        case AST_EXPR_CALL: {
            /* check callee type. If function, check argument count, etc. */
            (void)semantic_check_expr(expr->as.call.callee, ctx);
            for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                (void)semantic_check_expr(expr->as.call.args[i], ctx);
            }
            /* For demonstration, assume calls return unknown or some placeholder. */
            return result;
        }
        case AST_EXPR_INDEX: {
            /* Check object and index expressions. */
            (void)semantic_check_expr(expr->as.index_expr.object, ctx);
            (void)semantic_check_expr(expr->as.index_expr.index, ctx);
            /* Potentially result is SEMANTIC_TYPE_UNKNOWN or the type of an array element, etc. */
            return result;
        }
        case AST_EXPR_MEMBER: {
            (void)semantic_check_expr(expr->as.member_expr.object, ctx);
            /* We might do a type-based lookup of the member. For now, just unknown. */
            return result;
        }
        case AST_EXPR_INTERPOLATION: {
            (void)semantic_check_expr(expr->as.interpolation.expr, ctx);
            /* Usually part of a string-building operation. We can call it string. */
            result.kind = SEMANTIC_TYPE_STRING;
            return result;
        }
        default:
            return result;
    }
}

void semantic_control_flow_analysis(AstNode* root, SemanticContext* ctx) {
    /* For demonstration, we do a minimal approach. */
    /* You can build a CFG or do a DFS on statements checking unreachable code, etc. */
    (void)root;
    (void)ctx;
    /* Stub that does nothing. */
}
