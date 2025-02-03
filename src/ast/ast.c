#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/ast.h"

static AstNode* ast_alloc_node(AstNodeType type, const SourceLocation* loc) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    if (!node) return NULL;
    memset(node, 0, sizeof(AstNode));
    node->type = type;
    if (loc) {
        node->loc = *loc; /* copy the location struct */
    }
    return node;
}

/* Utility to duplicate string safely */
static char* ast_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* dup = (char*)malloc(len+1);
    if (!dup) return NULL;
    memcpy(dup, s, len);
    dup[len] = '\0';
    return dup;
}

AstNode* ast_make_literal(const SourceLocation* loc, TokenType literal_type, const char* text) {
    AstNode* node = ast_alloc_node(AST_EXPR_LITERAL, loc);
    node->as.literal.literal_type = literal_type;
    switch (literal_type) {
        case TOKEN_INTEGER:
            node->as.literal.i64_val = atoll(text);
            break;
        case TOKEN_FLOAT:
            node->as.literal.f64_val = atof(text);
            break;
        case TOKEN_STRING:
        case TOKEN_DOCSTRING:
        case TOKEN_REGEX:
            node->as.literal.str_val = ast_strdup(text);
            break;
        case TOKEN_BOOL_TRUE:
            node->as.literal.bool_val = true;
            break;
        case TOKEN_BOOL_FALSE:
            node->as.literal.bool_val = false;
            break;
        default:
            /* If needed, handle other token types. */
            break;
    }
    return node;
}

AstNode* ast_make_binary(const SourceLocation* loc, TokenType op, AstNode* left, AstNode* right) {
    AstNode* node = ast_alloc_node(AST_EXPR_BINARY, loc);
    node->as.binary.op = op;
    node->as.binary.left = left;
    node->as.binary.right = right;
    return node;
}

AstNode* ast_make_unary(const SourceLocation* loc, TokenType op, AstNode* expr) {
    AstNode* node = ast_alloc_node(AST_EXPR_UNARY, loc);
    node->as.unary.op = op;
    node->as.unary.expr = expr;
    return node;
}

AstNode* ast_make_identifier(const SourceLocation* loc, const char* name) {
    AstNode* node = ast_alloc_node(AST_EXPR_IDENTIFIER, loc);
    node->as.ident.name = ast_strdup(name);
    return node;
}

AstNode* ast_make_call(const SourceLocation* loc, AstNode* callee, size_t arg_count) {
    AstNode* node = ast_alloc_node(AST_EXPR_CALL, loc);
    node->as.call.callee = callee;
    node->as.call.arg_count = arg_count;
    if (arg_count > 0) {
        node->as.call.args = (AstNode**)calloc(arg_count, sizeof(AstNode*));
    }
    return node;
}

AstNode* ast_make_interpolation(const SourceLocation* loc, AstNode* expr) {
    AstNode* node = ast_alloc_node(AST_EXPR_INTERPOLATION, loc);
    node->as.interpolation.expr = expr;
    return node;
}

AstNode* ast_make_var_decl(const SourceLocation* loc, const char* var_name, bool is_const, AstNode* init) {
    AstNode* node = ast_alloc_node(AST_NODE_VAR_DECL, loc);
    node->as.var_decl.var_name = ast_strdup(var_name);
    node->as.var_decl.initializer = init;
    node->as.var_decl.is_const = is_const;
    return node;
}

AstNode* ast_make_func_decl(const SourceLocation* loc, const char* func_name,
                            char** params, size_t param_count, AstNode* body) {
    AstNode* node = ast_alloc_node(AST_NODE_FUNC_DECL, loc);
    node->as.func_decl.func_name = ast_strdup(func_name);
    node->as.func_decl.param_names = NULL;
    node->as.func_decl.param_count = param_count;
    if (param_count > 0) {
        node->as.func_decl.param_names = (char**)calloc(param_count, sizeof(char*));
        for (size_t i = 0; i < param_count; i++) {
            node->as.func_decl.param_names[i] = ast_strdup(params[i]);
        }
    }
    node->as.func_decl.body = body;
    return node;
}

AstNode* ast_make_block(const SourceLocation* loc, size_t stmt_count) {
    AstNode* node = ast_alloc_node(AST_NODE_BLOCK, loc);
    node->as.block.statement_count = stmt_count;
    if (stmt_count > 0) {
        node->as.block.statements = (AstNode**)calloc(stmt_count, sizeof(AstNode*));
    }
    return node;
}

/* Recursively frees sub-nodes. */
static void ast_destroy_recursive(AstNode* node) {
    if (!node) return;

    switch (node->type) {
        case AST_EXPR_LITERAL:
            if (node->as.literal.literal_type == TOKEN_STRING ||
                node->as.literal.literal_type == TOKEN_DOCSTRING ||
                node->as.literal.literal_type == TOKEN_REGEX) {
                free(node->as.literal.str_val);
            }
            break;
        case AST_EXPR_IDENTIFIER:
            free(node->as.ident.name);
            break;
        case AST_EXPR_CALL:
            if (node->as.call.args) {
                for (size_t i = 0; i < node->as.call.arg_count; i++) {
                    ast_destroy_recursive(node->as.call.args[i]);
                }
                free(node->as.call.args);
            }
            ast_destroy_recursive(node->as.call.callee);
            break;
        case AST_EXPR_BINARY:
            ast_destroy_recursive(node->as.binary.left);
            ast_destroy_recursive(node->as.binary.right);
            break;
        case AST_EXPR_UNARY:
            ast_destroy_recursive(node->as.unary.expr);
            break;
        case AST_EXPR_INDEX:
            ast_destroy_recursive(node->as.index_expr.object);
            ast_destroy_recursive(node->as.index_expr.index);
            break;
        case AST_EXPR_MEMBER:
            free(node->as.member_expr.member_name);
            ast_destroy_recursive(node->as.member_expr.object);
            break;
        case AST_EXPR_INTERPOLATION:
            ast_destroy_recursive(node->as.interpolation.expr);
            break;

        case AST_NODE_VAR_DECL:
            free(node->as.var_decl.var_name);
            ast_destroy_recursive(node->as.var_decl.initializer);
            break;
        case AST_NODE_FUNC_DECL:
            free(node->as.func_decl.func_name);
            if (node->as.func_decl.param_names) {
                for (size_t i = 0; i < node->as.func_decl.param_count; i++) {
                    free(node->as.func_decl.param_names[i]);
                }
                free(node->as.func_decl.param_names);
            }
            ast_destroy_recursive(node->as.func_decl.body);
            break;
        case AST_NODE_CLASS_DECL:
            free(node->as.class_decl.class_name);
            if (node->as.class_decl.members) {
                for (size_t i = 0; i < node->as.class_decl.member_count; i++) {
                    ast_destroy_recursive(node->as.class_decl.members[i]);
                }
                free(node->as.class_decl.members);
            }
            break;
        case AST_NODE_IF_STMT:
            ast_destroy_recursive(node->as.if_stmt.condition);
            ast_destroy_recursive(node->as.if_stmt.then_branch);
            ast_destroy_recursive(node->as.if_stmt.else_branch);
            break;
        case AST_NODE_WHILE_STMT:
            ast_destroy_recursive(node->as.while_stmt.condition);
            ast_destroy_recursive(node->as.while_stmt.body);
            break;
        case AST_NODE_FOR_STMT:
            ast_destroy_recursive(node->as.for_stmt.init);
            ast_destroy_recursive(node->as.for_stmt.condition);
            ast_destroy_recursive(node->as.for_stmt.increment);
            ast_destroy_recursive(node->as.for_stmt.body);
            break;
        case AST_NODE_RETURN_STMT:
            ast_destroy_recursive(node->as.ret_stmt.expr);
            break;
        case AST_NODE_BLOCK:
            if (node->as.block.statements) {
                for (size_t i = 0; i < node->as.block.statement_count; i++) {
                    ast_destroy_recursive(node->as.block.statements[i]);
                }
                free(node->as.block.statements);
            }
            break;
        default:
            /* e.g. AST_NODE_PROGRAM or AST_NODE_EXPR_STMT can appear. */
            break;
    }
    ast_destroy_recursive(node->next_sibling);

    free(node);
}

void ast_destroy(AstNode* node) {
    ast_destroy_recursive(node);
}
