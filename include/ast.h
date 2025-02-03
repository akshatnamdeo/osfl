#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "source_location.h"
#include "../lexer/token.h"

/*
 * Enumeration of AST node types for OSFL.
 * We merge your older type (AST_NODE_FRAME, etc.) with
 * the newer approach for expression/statement detail.
 */
typedef enum {
    /* Old OSFL node types */
    AST_NODE_FRAME,          // Frame declaration
    AST_NODE_VAR_DECL,       // Variable declaration
    AST_NODE_CONST_DECL,     // Constant declaration (if you keep it distinct)
    AST_NODE_FUNC_DECL,      // Function declaration
    AST_NODE_IF,             // If statement (older label)
    AST_NODE_ELSE,           // Else clause
    AST_NODE_LOOP,           // Loop statement
    AST_NODE_ERROR_HANDLER,  // Error handling block (on_error)
    AST_NODE_FUNCTION,       // "function" block (alias of AST_NODE_FUNC_DECL)
    AST_NODE_TRY_CATCH,      // "try { ... } catch(...) { ... }"

    /* Newer or more detailed node types */
    AST_NODE_CLASS_DECL,
    AST_NODE_BLOCK,
    AST_NODE_RETURN_STMT,
    AST_NODE_WHILE_STMT,
    AST_NODE_FOR_STMT,
    AST_NODE_IF_STMT,        // More detailed "if" statement
    AST_NODE_EXPR_STMT,      // Expression used as a statement

    /* Expression node types */
    AST_EXPR,                // A generic expr if still needed
    AST_EXPR_BINARY,
    AST_EXPR_UNARY,
    AST_EXPR_LITERAL,
    AST_EXPR_IDENTIFIER,
    AST_EXPR_CALL,
    AST_EXPR_INDEX,
    AST_EXPR_MEMBER,
    AST_EXPR_INTERPOLATION,
    AST_NODE_DOCSTRING,
    AST_NODE_REGEX
} AstNodeType;

/*
 * For a literal expression: we store the token type + union of numeric/string/bool data.
 */
typedef struct {
    TokenType literal_type;  /* e.g. TOKEN_INTEGER, TOKEN_STRING, TOKEN_BOOL_TRUE, etc. */
    union {
        int64_t i64_val;
        double  f64_val;
        bool    bool_val;
        char*   str_val;  /* for strings, docstrings, regex, etc. */
    };
} AstLiteralData;

/*
 * For a binary operation expression: left op right
 */
typedef struct {
    TokenType op;       /* e.g. TOKEN_PLUS, TOKEN_MINUS, etc. */
    struct AstNode* left;
    struct AstNode* right;
} AstBinaryData;

/*
 * For a unary operation expression: op expr
 */
typedef struct {
    TokenType op;  /* e.g. TOKEN_MINUS, TOKEN_NOT, etc. */
    struct AstNode* expr;
} AstUnaryData;

/*
 * For a function call expression: callee(args...)
 */
typedef struct {
    struct AstNode* callee;
    struct AstNode** args; /* dynamic array of ArgCount expressions */
    size_t arg_count;
} AstCallData;

/*
 * For an identifier expression: a name
 */
typedef struct {
    char* name;
} AstIdentifierData;

/*
 * For array or object indexing: object[index]
 */
typedef struct {
    struct AstNode* object;
    struct AstNode* index;
} AstIndexData;

/*
 * For object.member
 */
typedef struct {
    struct AstNode* object;
    char* member_name;
} AstMemberData;

/*
 * For interpolation segments: stores the expr from "${ expr }"
 */
typedef struct {
    struct AstNode* expr;
} AstInterpolationData;

/*
 * If you want a "Frame" from old OSFL design: frame <id> { body... }
 */
typedef struct {
    char* frame_name;
    struct AstNode** body_statements;
    size_t body_count;
} AstFrameDeclData;

/*
 * Var/const declaration
 */
typedef struct {
    char* var_name;
    bool is_const;
    struct AstNode* initializer;
} AstVarDeclData;

/*
 * Function declaration
 */
typedef struct {
    char* func_name;
    char** param_names;
    size_t param_count;
    struct AstNode* body; /* usually a block node */
} AstFuncDeclData;

/*
 * Class declaration
 */
typedef struct {
    char* class_name;
    struct AstNode** members;
    size_t member_count;
} AstClassDeclData;

/*
 * If statement
 */
typedef struct {
    struct AstNode* condition;
    struct AstNode* then_branch;
    struct AstNode* else_branch;
} AstIfStmtData;

/*
 * Loop statement (older label) or unify with while/for
 */
typedef struct {
    struct AstNode* body;
} AstLoopData;

/*
 * While statement
 */
typedef struct {
    struct AstNode* condition;
    struct AstNode* body;
} AstWhileStmtData;

/*
 * For statement
 */
typedef struct {
    struct AstNode* init;
    struct AstNode* condition;
    struct AstNode* increment;
    struct AstNode* body;
} AstForStmtData;

/*
 * Return statement
 */
typedef struct {
    struct AstNode* expr; /* optional, might be null */
} AstReturnStmtData;

/*
 * On_error or try/catch
 */
typedef struct {
    struct AstNode* handler_body;
    /* or store 'tryBlock', 'catchBlock', etc. */
} AstErrorHandlerData;

/*
 * Block of statements
 */
typedef struct {
    struct AstNode** statements;
    size_t statement_count;
} AstBlockData;

/*
 * The main AST node structure, with a union of substructures
 */
typedef struct AstNode {
    AstNodeType type;
    SourceLocation loc;

    union {
        /* Expressions */
        AstLiteralData       literal;
        AstBinaryData        binary;
        AstUnaryData         unary;
        AstCallData          call;
        AstIdentifierData    ident;
        AstIndexData         index_expr;
        AstMemberData        member_expr;
        AstInterpolationData interpolation;

        /* Frame from older OSFL design */
        AstFrameDeclData     frame_decl;

        /* Var/const decl */
        AstVarDeclData       var_decl;

        /* Function decl */
        AstFuncDeclData      func_decl;

        /* Class decl */
        AstClassDeclData     class_decl;

        /* If statement */
        AstIfStmtData        if_stmt;

        /* Loop statement */
        AstLoopData          loop_stmt;

        /* While statement */
        AstWhileStmtData     while_stmt;

        /* For statement */
        AstForStmtData       for_stmt;

        /* Return statement */
        AstReturnStmtData    ret_stmt;

        /* Error handler (on_error or try/catch) */
        AstErrorHandlerData  error_handler;

        /* A block of statements */
        AstBlockData         block;
    } as;

    /* Linked-list pointer for chaining siblings */
    struct AstNode* next_sibling;

} AstNode;

#ifdef __cplusplus
extern "C" {
#endif

/* Example constructor for a Frame node */
AstNode* ast_new_frame(const SourceLocation* loc,
                       const char* frame_name,
                       AstNode** body, size_t body_count);

/* Example destructor to free the entire tree */
void ast_destroy(AstNode* node);

#ifdef __cplusplus
}
#endif

#endif /* AST_H */
