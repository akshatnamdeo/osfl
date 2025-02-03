#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include "../lexer/token.h"
#include "../../include/ast.h"

/**
 * Parser structure holds the token stream and current index.
 */
typedef struct Parser {
    Token* tokens;
    size_t token_count;
    size_t current;
    // Additional fields (e.g., error messages) can be added as needed.
} Parser;

/**
 * Create a new Parser instance.
 *
 * @param tokens Pointer to an array of tokens.
 * @param token_count Number of tokens in the array.
 * @return Pointer to a new Parser instance.
 */
Parser* parser_create(Token* tokens, size_t token_count);

/**
 * Parse the token stream into an AST.
 *
 * @param parser The Parser instance.
 * @return The root AST node of the parsed program.
 */
AstNode* parser_parse(Parser* parser);

/**
 * Destroy the Parser instance and free associated memory.
 *
 * @param parser The Parser instance to destroy.
 */
void parser_destroy(Parser* parser);

#endif // PARSER_H
