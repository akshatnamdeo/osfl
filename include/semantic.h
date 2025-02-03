#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"

/**
 * @brief Simple type enumeration for semantic analysis
 */
typedef enum {
    SEMANTIC_TYPE_UNKNOWN,
    SEMANTIC_TYPE_INT,
    SEMANTIC_TYPE_FLOAT,
    SEMANTIC_TYPE_BOOL,
    SEMANTIC_TYPE_STRING,
    SEMANTIC_TYPE_VOID,
    SEMANTIC_TYPE_CUSTOM /* for classes, user-defined */
} SemanticValueType;

/**
 * @brief Basic type info structure
 */
typedef struct {
    SemanticValueType kind;
    char* custom_name; /* if SEMANTIC_TYPE_CUSTOM, e.g. "MyClass" */
} TypeInfo;

/**
 * @brief Holds data for semantic analysis (symbol table, etc.)
 */
typedef struct {
    Scope* current_scope;
    /* You can add error tracking, warnings, etc. */
    int error_count;
} SemanticContext;

/**
 * @brief Initialize the semantic context
 */
void semantic_init(SemanticContext* ctx);

/**
 * @brief Clean up the semantic context
 */
void semantic_cleanup(SemanticContext* ctx);

/**
 * @brief Perform semantic analysis on the AST
 * @param root The root AST node (e.g. AST_NODE_PROGRAM)
 * @param ctx The semantic context
 */
void semantic_analyze(AstNode* root, SemanticContext* ctx);

/**
 * @brief Example function to get the type of an expression
 * If an error occurs (e.g. unknown identifier), it increments ctx->error_count.
 */
TypeInfo semantic_check_expr(AstNode* expr, SemanticContext* ctx);

/**
 * @brief Example function to do simple control flow or data flow analysis
 * Currently just a stub.
 */
void semantic_control_flow_analysis(AstNode* root, SemanticContext* ctx);

#endif /* SEMANTIC_H */
