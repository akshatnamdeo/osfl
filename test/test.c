#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/parser/parser.h"
#include "../include/ast.h"
#include "../src/lexer/token.h"
#include "../include/source_location.h"

/* Modified token creation to ensure proper memory management */
static Token create_token(TokenType type, const char* text, unsigned int line, unsigned int column, const char* file) {
    printf("Creating token: type=%d, text='%s'\n", type, text);
    fflush(stdout);
    
    Token token;
    token.type = type;
    memset(token.text, 0, sizeof(token.text));
    strncpy(token.text, text, sizeof(token.text) - 1);
    
    // Create a copy of the file path
    char* file_copy = strdup(file);
    if (!file_copy) {
        fprintf(stderr, "Failed to allocate memory for file path\n");
        exit(EXIT_FAILURE);
    }
    
    token.location.line = line;
    token.location.column = column;
    token.location.file = file_copy;
    
    printf("Token created successfully with file pointer: %p\n", (void*)token.location.file);
    fflush(stdout);
    
    return token;
}

static AstNode* create_ast_node(AstNodeType type, const char* value, const SourceLocation* loc) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    if (!node) return NULL;
    
    node->type = type;
    node->value = (value != NULL) ? strdup(value) : NULL;
    
    // Copy the source location struct but not the file pointer
    if (loc != NULL) {
        node->loc.line = loc->line;
        node->loc.column = loc->column;
        node->loc.file = loc->file;  // Just copy the pointer, don't duplicate the string
    } else {
        node->loc.line = 0;
        node->loc.column = 0;
        node->loc.file = NULL;
    }
    
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    return node;
}

/* Test helper to verify token integrity */
static void verify_token(const Token* token) {
    printf("Verifying token - Type: %d, Text: '%s'\n", token->type, token->text);
    printf("Location - Line: %u, Column: %u, File: %s\n", 
           token->location.line, token->location.column,
           token->location.file ? token->location.file : "NULL");
    printf("File pointer: %p\n", (void*)token->location.file);
    fflush(stdout);
}

static void cleanup_tokens(Token* tokens, size_t count) {
    printf("Starting token cleanup for %zu tokens...\n", count);
    fflush(stdout);
    
    if (!tokens) {
        printf("Tokens pointer is NULL, skipping cleanup\n");
        fflush(stdout);
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        printf("Processing token %zu: type=%d, text='%s'\n", 
               i, tokens[i].type, tokens[i].text);
        fflush(stdout);
        
        if (tokens[i].location.file) {
            const char* file_ptr = tokens[i].location.file;
            printf("Token %zu file pointer: %p\n", i, (void*)file_ptr);
            fflush(stdout);
            
            free((void*)file_ptr);
            tokens[i].location.file = NULL;
            
            printf("Freed file path for token %zu\n", i);
            fflush(stdout);
        }
        
        printf("Token %zu cleanup complete\n", i);
        fflush(stdout);
    }
    
    printf("Token cleanup completed successfully\n");
    fflush(stdout);
}

/* Enhanced AST cleanup function */
static void free_ast(AstNode* node) {
    if (!node) {
        printf("AST node is NULL, skipping cleanup\n");
        fflush(stdout);
        return;
    }
    
    printf("Cleaning up AST node: type=%d\n", node->type);
    fflush(stdout);
    
    if (node->left) {
        printf("Cleaning left child...\n");
        fflush(stdout);
        free_ast(node->left);
    }
    
    if (node->right) {
        printf("Cleaning right child...\n");
        fflush(stdout);
        free_ast(node->right);
    }
    
    if (node->next) {
        printf("Cleaning next sibling...\n");
        fflush(stdout);
        free_ast(node->next);
    }
    
    if (node->value) {
        printf("Freeing node value: %s\n", node->value);
        fflush(stdout);
        free(node->value);
    }
    
    // Note: We no longer free node->loc.file here
    // File paths are owned by tokens and will be freed during token cleanup
    
    printf("Freeing AST node itself\n");
    fflush(stdout);
    free(node);
}

/* Test assertion macro */
#define TEST_ASSERT(condition, message, tokens, token_count) \
    do { \
        printf("Testing assertion: %s\n", message); \
        fflush(stdout); \
        if (!(condition)) { \
            fprintf(stderr, "Test assertion failed: %s\n", message); \
            cleanup_tokens(tokens, token_count); \
            exit(EXIT_FAILURE); \
        } \
        printf("Assertion passed.\n"); \
        fflush(stdout); \
    } while(0)

static void test_parse_frame(void) {
    printf("\n=== Starting test_parse_frame ===\n");
    fflush(stdout);
    
    const char* file = "test_input.osfl";
    const size_t token_count = 10;
    Token tokens[10];
    int idx = 0;
    Parser* parser = NULL;
    AstNode* ast = NULL;
    
    printf("Creating frame test tokens...\n");
    fflush(stdout);
    
    tokens[idx++] = create_token(TOKEN_FRAME, "frame", 1, 1, file);
    printf("Created frame token\n"); fflush(stdout);
    
    tokens[idx++] = create_token(TOKEN_IDENTIFIER, "Main", 1, 7, file);
    printf("Created identifier token\n"); fflush(stdout);
    
    tokens[idx++] = create_token(TOKEN_LBRACE, "{", 1, 12, file);
    printf("Created left brace token\n"); fflush(stdout);
    
    tokens[idx++] = create_token(TOKEN_VAR, "var", 2, 3, file);
    tokens[idx++] = create_token(TOKEN_IDENTIFIER, "x", 2, 7, file);
    tokens[idx++] = create_token(TOKEN_ASSIGN, "=", 2, 9, file);
    tokens[idx++] = create_token(TOKEN_INTEGER, "42", 2, 11, file);
    tokens[idx++] = create_token(TOKEN_SEMICOLON, ";", 2, 13, file);
    tokens[idx++] = create_token(TOKEN_RBRACE, "}", 3, 1, file);
    tokens[idx++] = create_token(TOKEN_EOF, "", 3, 2, file);

    for (size_t i = 0; i < token_count; i++) {
        printf("\nVerifying token %zu:\n", i);
        verify_token(&tokens[i]);
    }

    printf("Creating parser...\n");
    fflush(stdout);
    
    parser = parser_create(tokens, token_count);
    if (!parser) {
        fprintf(stderr, "Parser creation failed!\n");
        cleanup_tokens(tokens, token_count);
        exit(1);
    }

    printf("Parsing tokens...\n");
    fflush(stdout);
    
    ast = parser_parse(parser);
    
    printf("Checking parse results...\n");
    fflush(stdout);

    TEST_ASSERT(ast != NULL, "AST should not be NULL for frame declaration.", tokens, token_count);
    TEST_ASSERT(ast->type == AST_NODE_FRAME, "Root AST node should be a frame.", tokens, token_count);
    TEST_ASSERT(strcmp(ast->value, "Main") == 0, "Frame name should be 'Main'.", tokens, token_count);
    TEST_ASSERT(ast->left != NULL, "Frame body should not be empty.", tokens, token_count);
    TEST_ASSERT(ast->left->type == AST_NODE_VAR_DECL, "Frame body first statement should be a var declaration.", tokens, token_count);
    TEST_ASSERT(strcmp(ast->left->value, "x") == 0, "Variable name should be 'x'.", tokens, token_count);

    printf("\nStarting cleanup phase...\n");
    fflush(stdout);
    
    if (parser) {
        printf("Destroying parser...\n");
        fflush(stdout);
        parser_destroy(parser);
        printf("Parser destroyed successfully\n");
        fflush(stdout);
    }
    
    if (ast) {
        printf("Cleaning up AST...\n");
        fflush(stdout);
        free_ast(ast);
        printf("AST cleanup completed\n");
        fflush(stdout);
    }
    
    printf("Starting token cleanup...\n");
    fflush(stdout);
    cleanup_tokens(tokens, token_count);
    
    printf("test_parse_frame completed successfully.\n");
    fflush(stdout);
}

int main(void) {
    printf("=== OSFL Parser Test Suite ===\n\n");
    fflush(stdout);
    
    printf("Starting test_parse_frame...\n");
    fflush(stdout);
    
    test_parse_frame();
    
    printf("\nAll parser tests completed successfully!\n");
    return EXIT_SUCCESS;
}