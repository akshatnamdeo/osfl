#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "source_location.h"  // Include the common definition

/* 
 * Enumeration of AST node types for OSFL. 
 * Updated to include the identifiers used in the tests:
 *   AST_NODE_FUNCTION  (for "function" declarations)
 *   AST_NODE_TRY_CATCH (for "try/catch" blocks)
 */
typedef enum {
    AST_NODE_FRAME,          // Frame declaration
    AST_NODE_VAR_DECL,       // Variable declaration
    AST_NODE_CONST_DECL,     // Constant declaration
    AST_NODE_FUNC_DECL,      // Function declaration
    AST_NODE_EXPR,           // Expression (literal, identifier, or binary/unary expr)
    AST_NODE_IF,             // If statement
    AST_NODE_ELSE,           // Else clause
    AST_NODE_LOOP,           // Loop statement
    AST_NODE_ERROR_HANDLER,  // Error handling block (on_error)

    /*
     * New AST node types used in your test code.
     * If you prefer to unify them with existing types,
     * simply redirect AST_NODE_FUNCTION to AST_NODE_FUNC_DECL, etc.
     */
    AST_NODE_FUNCTION,       // "function" block (alternative name to AST_NODE_FUNC_DECL)
    AST_NODE_TRY_CATCH       // "try { ... } catch(...) { ... }"
} AstNodeType;

/* AST node structure for OSFL. */
typedef struct AstNode {
    AstNodeType type;         // The type of this AST node
    char* value;              // For identifiers, literals, or parameter lists
    SourceLocation loc;       // Source location (line, column, file)
    struct AstNode* left;     // First child (e.g., frame body, function body)
    struct AstNode* right;    // Second child (e.g., else block, catch block)
    struct AstNode* next;     // Next node in a sequence (e.g., statement list)
} AstNode;

#endif // AST_H
