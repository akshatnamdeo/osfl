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

/* Test helper to verify token integrity */
static void verify_token(const Token* token) {
    printf("Verifying token - Type: %d, Text: '%s'\n", token->type, token->text);
    printf("Location - Line: %u, Column: %u, File: %s\n", 
           token->location.line, token->location.column,
           token->location.file ? token->location.file : "NULL");
    printf("File pointer: %p\n", (void*)token->location.file);
    fflush(stdout);
}

/* Helper function to free tokens (frees file strings) */
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
    if (node == NULL) {
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
    
    /* We no longer free node->loc.file here because the tokens own that memory */
    
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

/* 
   Test parsing a simple frame declaration:
   OSFL source: frame Main { var x = 42; }
   Tokens:
     1. TOKEN_FRAME         "frame"
     2. TOKEN_IDENTIFIER    "Main"
     3. TOKEN_LBRACE        "{"
     4. TOKEN_VAR           "var"
     5. TOKEN_IDENTIFIER    "x"
     6. TOKEN_ASSIGN        "="
     7. TOKEN_INTEGER       "42"
     8. TOKEN_SEMICOLON     ";"
     9. TOKEN_RBRACE        "}"
    10. TOKEN_EOF           ""
*/
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
        exit(EXIT_FAILURE);
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

/* 
   Test parsing a simple function declaration:
   OSFL source: func add(x,y) { return x+y; }
   (Note: our simple parser treats the expression as a simple expression.)
   Tokens:
     1. TOKEN_FUNC           "func"
     2. TOKEN_IDENTIFIER     "add"
     3. TOKEN_LPAREN         "("
     4. TOKEN_IDENTIFIER     "x"
     5. TOKEN_COMMA          ","
     6. TOKEN_IDENTIFIER     "y"
     7. TOKEN_RPAREN         ")"
     8. TOKEN_LBRACE         "{"
     9. TOKEN_RETURN         "return"
    10. TOKEN_IDENTIFIER     "x+y"  // Simplified expression.
    11. TOKEN_SEMICOLON      ";"
    12. TOKEN_RBRACE         "}"
    13. TOKEN_EOF            ""
   
   Note: Since our parser reuses the function node's value field to store the parameter list,
         we expect that value to be "x,y" (not the function name).
*/
static void test_parse_function(void) {
    printf("\n=== Starting test_parse_function ===\n");
    fflush(stdout);

    const char* file = "test_input.osfl";
    const size_t token_count = 13;
    Token tokens[13];
    int idx = 0;
    Parser* parser = NULL;
    AstNode* ast = NULL;

    printf("Creating function test tokens...\n");
    fflush(stdout);

    tokens[idx++] = create_token(TOKEN_FUNC, "func", 1, 1, file);
    tokens[idx++] = create_token(TOKEN_IDENTIFIER, "add", 1, 6, file);
    tokens[idx++] = create_token(TOKEN_LPAREN, "(", 1, 9, file);
    tokens[idx++] = create_token(TOKEN_IDENTIFIER, "x", 1, 10, file);
    tokens[idx++] = create_token(TOKEN_COMMA, ",", 1, 11, file);
    tokens[idx++] = create_token(TOKEN_IDENTIFIER, "y", 1, 13, file);
    tokens[idx++] = create_token(TOKEN_RPAREN, ")", 1, 14, file);
    tokens[idx++] = create_token(TOKEN_LBRACE, "{", 1, 16, file);
    tokens[idx++] = create_token(TOKEN_RETURN, "return", 2, 3, file);
    tokens[idx++] = create_token(TOKEN_IDENTIFIER, "x+y", 2, 10, file);
    tokens[idx++] = create_token(TOKEN_SEMICOLON, ";", 2, 13, file);
    tokens[idx++] = create_token(TOKEN_RBRACE, "}", 3, 1, file);
    tokens[idx++] = create_token(TOKEN_EOF, "", 3, 2, file);

    for (size_t i = 0; i < token_count; i++) {
        printf("\nVerifying token %zu:\n", i);
        verify_token(&tokens[i]);
    }

    printf("Creating parser for function...\n");
    fflush(stdout);

    parser = parser_create(tokens, token_count);
    if (!parser) {
        fprintf(stderr, "Parser creation failed!\n");
        cleanup_tokens(tokens, token_count);
        exit(EXIT_FAILURE);
    }

    printf("Parsing tokens for function...\n");
    fflush(stdout);

    ast = parser_parse(parser);

    printf("Checking parse results for function...\n");
    fflush(stdout);

    TEST_ASSERT(ast != NULL, "AST should not be NULL for function declaration.", tokens, token_count);
    TEST_ASSERT(ast->type == AST_NODE_FUNC_DECL, "Root AST node should be a function declaration.", tokens, token_count);
    /* Since our parser reuses the 'value' field to store parameters, we expect "x,y". */
    TEST_ASSERT(strcmp(ast->value, "x,y") == 0, "Function parameters should be 'x,y'.", tokens, token_count);
    TEST_ASSERT(ast->left != NULL, "Function body should not be empty.", tokens, token_count);
    TEST_ASSERT(ast->left->type == AST_NODE_EXPR, "Function body first statement should be an expression.", tokens, token_count);

    printf("\nStarting cleanup phase for function test...\n");
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

    printf("Starting token cleanup for function test...\n");
    fflush(stdout);
    cleanup_tokens(tokens, token_count);

    printf("test_parse_function completed successfully.\n");
    fflush(stdout);
}

/* Test parsing an if statement with else:
   OSFL source: if (x > 0) { } else { }
   For simplicity, this test creates tokens only for the condition and uses TOKEN_EOF for the remainder.
   We'll use 7 tokens in this minimal test.
*/
static void test_parse_if_else(void) {
    printf("\n=== Starting test_parse_if_else ===\n");
    fflush(stdout);

    const char* file = "test_input.osfl";
    const size_t token_count = 7;  /* Adjusted count */
    Token tokens[7];
    int idx = 0;
    Parser* parser = NULL;
    AstNode* ast = NULL;

    printf("Creating if-else test tokens...\n");
    fflush(stdout);

    tokens[idx++] = create_token(TOKEN_IF, "if", 1, 1, file);
    tokens[idx++] = create_token(TOKEN_LPAREN, "(", 1, 4, file);
    tokens[idx++] = create_token(TOKEN_IDENTIFIER, "x", 1, 5, file);
    tokens[idx++] = create_token(TOKEN_GT, ">", 1, 7, file);
    tokens[idx++] = create_token(TOKEN_INTEGER, "0", 1, 9, file);
    tokens[idx++] = create_token(TOKEN_RPAREN, ")", 1, 10, file);
    tokens[idx++] = create_token(TOKEN_EOF, "", 1, 11, file);
    /* This minimal test does not include bodies for if or else. */

    for (size_t i = 0; i < token_count; i++) {
        printf("\nVerifying token %zu:\n", i);
        verify_token(&tokens[i]);
    }

    printf("Creating parser for if-else...\n");
    fflush(stdout);

    parser = parser_create(tokens, token_count);
    if (!parser) {
        fprintf(stderr, "Parser creation failed!\n");
        cleanup_tokens(tokens, token_count);
        exit(EXIT_FAILURE);
    }

    printf("Parsing tokens for if-else...\n");
    fflush(stdout);

    ast = parser_parse(parser);

    printf("Checking parse results for if-else...\n");
    fflush(stdout);

    TEST_ASSERT(ast != NULL, "AST should not be NULL for if-else statement.", tokens, token_count);
    TEST_ASSERT(ast->type == AST_NODE_IF, "Root AST node should be an if statement.", tokens, token_count);

    printf("\nStarting cleanup phase for if-else test...\n");
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

    printf("Starting token cleanup for if-else test...\n");
    fflush(stdout);
    cleanup_tokens(tokens, token_count);

    printf("test_parse_if_else completed successfully.\n");
    fflush(stdout);
}

/* Test parsing a loop statement:
   OSFL source: loop { var i = 0; }
   Tokens:
     1. TOKEN_LOOP           "loop"
     2. TOKEN_LBRACE         "{"
     3. TOKEN_VAR            "var"
     4. TOKEN_IDENTIFIER     "i"
     5. TOKEN_ASSIGN         "="
     6. TOKEN_INTEGER        "0"
     7. TOKEN_SEMICOLON      ";"
     8. TOKEN_RBRACE         "}"
     9. TOKEN_EOF            ""
*/
static void test_parse_loop(void) {
    printf("\n=== Starting test_parse_loop ===\n");
    fflush(stdout);

    const char* file = "test_input.osfl";
    const size_t token_count = 9;
    Token tokens[9];
    int idx = 0;
    Parser* parser = NULL;
    AstNode* ast = NULL;

    printf("Creating loop test tokens...\n");
    fflush(stdout);

    tokens[idx++] = create_token(TOKEN_LOOP, "loop", 1, 1, file);
    tokens[idx++] = create_token(TOKEN_LBRACE, "{", 1, 6, file);
    tokens[idx++] = create_token(TOKEN_VAR, "var", 2, 3, file);
    tokens[idx++] = create_token(TOKEN_IDENTIFIER, "i", 2, 7, file);
    tokens[idx++] = create_token(TOKEN_ASSIGN, "=", 2, 9, file);
    tokens[idx++] = create_token(TOKEN_INTEGER, "0", 2, 11, file);
    tokens[idx++] = create_token(TOKEN_SEMICOLON, ";", 2, 12, file);
    tokens[idx++] = create_token(TOKEN_RBRACE, "}", 3, 1, file);
    tokens[idx++] = create_token(TOKEN_EOF, "", 3, 2, file);

    for (size_t i = 0; i < token_count; i++) {
        printf("\nVerifying token %zu:\n", i);
        verify_token(&tokens[i]);
    }

    printf("Creating parser for loop...\n");
    fflush(stdout);

    parser = parser_create(tokens, token_count);
    if (!parser) {
        fprintf(stderr, "Parser creation failed!\n");
        cleanup_tokens(tokens, token_count);
        exit(EXIT_FAILURE);
    }

    printf("Parsing tokens for loop...\n");
    fflush(stdout);

    ast = parser_parse(parser);

    printf("Checking parse results for loop...\n");
    fflush(stdout);

    TEST_ASSERT(ast != NULL, "AST should not be NULL for loop statement.", tokens, token_count);
    TEST_ASSERT(ast->type == AST_NODE_LOOP, "Root AST node should be a loop statement.", tokens, token_count);

    printf("\nStarting cleanup phase for loop test...\n");
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

    printf("Starting token cleanup for loop test...\n");
    fflush(stdout);
    cleanup_tokens(tokens, token_count);

    printf("test_parse_loop completed successfully.\n");
    fflush(stdout);
}

/* Test parsing an error handler block:
   OSFL source: on_error { var err = 1; }
   Tokens:
     1. TOKEN_ON_ERROR       "on_error"
     2. TOKEN_LBRACE         "{"
     3. TOKEN_VAR            "var"
     4. TOKEN_IDENTIFIER     "err"
     5. TOKEN_ASSIGN         "="
     6. TOKEN_INTEGER        "1"
     7. TOKEN_SEMICOLON      ";"
     8. TOKEN_RBRACE         "}"
     9. TOKEN_EOF            ""
*/
static void test_parse_error_handler(void) {
    printf("\n=== Starting test_parse_error_handler ===\n");
    fflush(stdout);

    const char* file = "test_input.osfl";
    const size_t token_count = 9;
    Token tokens[9];
    int idx = 0;
    Parser* parser = NULL;
    AstNode* ast = NULL;

    printf("Creating error handler test tokens...\n");
    fflush(stdout);

    tokens[idx++] = create_token(TOKEN_ON_ERROR, "on_error", 1, 1, file);
    tokens[idx++] = create_token(TOKEN_LBRACE, "{", 1, 10, file);
    tokens[idx++] = create_token(TOKEN_VAR, "var", 2, 3, file);
    tokens[idx++] = create_token(TOKEN_IDENTIFIER, "err", 2, 7, file);
    tokens[idx++] = create_token(TOKEN_ASSIGN, "=", 2, 11, file);
    tokens[idx++] = create_token(TOKEN_INTEGER, "1", 2, 13, file);
    tokens[idx++] = create_token(TOKEN_SEMICOLON, ";", 2, 14, file);
    tokens[idx++] = create_token(TOKEN_RBRACE, "}", 3, 1, file);
    tokens[idx++] = create_token(TOKEN_EOF, "", 3, 2, file);

    for (size_t i = 0; i < token_count; i++) {
        printf("\nVerifying token %zu:\n", i);
        verify_token(&tokens[i]);
    }

    printf("Creating parser for error handler...\n");
    fflush(stdout);

    parser = parser_create(tokens, token_count);
    if (!parser) {
        fprintf(stderr, "Parser creation failed!\n");
        cleanup_tokens(tokens, token_count);
        exit(EXIT_FAILURE);
    }

    printf("Parsing tokens for error handler...\n");
    fflush(stdout);

    ast = parser_parse(parser);

    printf("Checking parse results for error handler...\n");
    fflush(stdout);

    TEST_ASSERT(ast != NULL, "AST should not be NULL for error handler block.", tokens, token_count);
    TEST_ASSERT(ast->type == AST_NODE_ERROR_HANDLER, "Root AST node should be an error handler.", tokens, token_count);
    TEST_ASSERT(ast->left != NULL, "Error handler must have a body.", tokens, token_count);
    TEST_ASSERT(ast->left->type == AST_NODE_VAR_DECL, "Error handler body should be a var declaration.", tokens, token_count);

    printf("\nStarting cleanup phase for error handler test...\n");
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

    printf("Starting token cleanup for error handler test...\n");
    fflush(stdout);
    cleanup_tokens(tokens, token_count);

    printf("test_parse_error_handler completed successfully.\n");
    fflush(stdout);
}

int main(void) {
    printf("=== OSFL Parser Test Suite ===\n\n");
    fflush(stdout);
    
    printf("Starting test_parse_frame...\n");
    fflush(stdout);
    test_parse_frame();

    printf("Starting test_parse_function...\n");
    fflush(stdout);
    test_parse_function();

    printf("Starting test_parse_if_else...\n");
    fflush(stdout);
    test_parse_if_else();

    printf("Starting test_parse_loop...\n");
    fflush(stdout);
    test_parse_loop();

    printf("Starting test_parse_error_handler...\n");
    fflush(stdout);
    test_parse_error_handler();

    printf("\nAll parser tests completed successfully!\n");
    return EXIT_SUCCESS;
}
