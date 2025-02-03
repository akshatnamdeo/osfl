#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
    Helper functions for token management.
*/

/**
 * Peek at the current token without consuming it.
 */
static Token parser_peek(Parser* parser) {
    if (parser->current < parser->token_count) {
        return parser->tokens[parser->current];
    }
    Token eof;
    memset(&eof, 0, sizeof(Token));
    eof.type = TOKEN_EOF;
    return eof;
}

/**
 * Consume and return the current token.
 */
static Token parser_advance(Parser* parser) {
    if (parser->current < parser->token_count) {
        return parser->tokens[parser->current++];
    }
    Token eof;
    memset(&eof, 0, sizeof(Token));
    eof.type = TOKEN_EOF;
    return eof;
}

/**
 * If the current token matches the given type, consume it and return 1;
 * otherwise, return 0.
 */
static int parser_match(Parser* parser, TokenType type) {
    if (parser_peek(parser).type == type) {
        parser_advance(parser);
        return 1;
    }
    return 0;
}

/**
 * Create an AST node with the given type and value.
 * The source location is taken from the pointer passed in.
 */
static AstNode* create_ast_node(AstNodeType type, const char* value, const SourceLocation* loc) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    if (!node) return NULL;
    node->type = type;
    node->value = (value != NULL) ? strdup(value) : NULL;
    if (loc != NULL) {
        node->loc = *loc;  // Copy the source location structure.
    } else {
        memset(&node->loc, 0, sizeof(SourceLocation));
    }
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    return node;
}

/* Forward declarations for recursive-descent parsing functions. */
static AstNode* parse_statement(Parser* parser);
static AstNode* parse_expression(Parser* parser);
static AstNode* parse_frame(Parser* parser);
static AstNode* parse_var_decl(Parser* parser);
static AstNode* parse_func_decl(Parser* parser);
static AstNode* parse_if_statement(Parser* parser);
static AstNode* parse_loop_statement(Parser* parser);
static AstNode* parse_error_handler(Parser* parser);

/**
 * Parse a sequence of statements until TOKEN_EOF.
 */
static AstNode* parse_program(Parser* parser) {
    AstNode* head = NULL;
    AstNode* current = NULL;
    while (parser_peek(parser).type != TOKEN_EOF) {
        AstNode* stmt = parse_statement(parser);
        if (!stmt) {
            // Error recovery: skip a token and continue.
            parser_advance(parser);
            continue;
        }
        if (!head) {
            head = stmt;
            current = stmt;
        } else {
            current->next = stmt;
            current = stmt;
        }
    }
    return head;
}

/**
 * Public API: Parse the token stream into an AST.
 */
AstNode* parser_parse(Parser* parser) {
    return parse_program(parser);
}

/**
 * Create a new Parser instance.
 */
Parser* parser_create(Token* tokens, size_t token_count) {
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    if (!parser) return NULL;
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    return parser;
}

/**
 * Destroy the Parser instance.
 */
void parser_destroy(Parser* parser) {
    if (parser) {
        free(parser);
    }
}

/**
 * Parse a single statement.
 * Delegates to specialized functions based on the current token.
 */
static AstNode* parse_statement(Parser* parser) {
    Token token = parser_peek(parser);
    switch (token.type) {
        case TOKEN_FRAME:
            return parse_frame(parser);
        case TOKEN_VAR:
        case TOKEN_CONST:
            return parse_var_decl(parser);
        case TOKEN_FUNC:
            return parse_func_decl(parser);
        case TOKEN_IF:
            return parse_if_statement(parser);
        case TOKEN_LOOP:
            return parse_loop_statement(parser);
        case TOKEN_ON_ERROR:
            return parse_error_handler(parser);
        default:
            return parse_expression(parser);
    }
}

/**
 * Parse a frame declaration.
 * Syntax: frame <Identifier> { <body> }
 */
static AstNode* parse_frame(Parser* parser) {
    Token frameToken = parser_advance(parser); // consume 'frame'
    const SourceLocation* frameLoc = &frameToken.location;
    Token id = parser_advance(parser); // expect identifier
    AstNode* frame_node = create_ast_node(AST_NODE_FRAME, id.text, &id.location);
    // Expect '{'
    if (!parser_match(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: expected '{' after frame declaration at %s:%d\n", id.location.file, id.location.line);
        return frame_node;
    }
    // Parse frame body (a sequence of statements)
    AstNode* body_head = NULL;
    AstNode* body_current = NULL;
    while (parser_peek(parser).type != TOKEN_RBRACE && parser_peek(parser).type != TOKEN_EOF) {
        AstNode* stmt = parse_statement(parser);
        if (!stmt) {
            parser_advance(parser);
            continue;
        }
        if (!body_head) {
            body_head = stmt;
            body_current = stmt;
        } else {
            body_current->next = stmt;
            body_current = stmt;
        }
    }
    // Consume '}'
    if (!parser_match(parser, TOKEN_RBRACE)) {
        fprintf(stderr, "Error: expected '}' at end of frame declaration at %s:%d\n", id.location.file, id.location.line);
    }
    frame_node->left = body_head;
    return frame_node;
}

/**
 * Parse a variable (or constant) declaration.
 * Syntax: var|const <Identifier> (= <expression>)? ;
 */
static AstNode* parse_var_decl(Parser* parser) {
    Token declType = parser_advance(parser); // consume 'var' or 'const'
    const SourceLocation* declLoc = &declType.location;
    Token id = parser_advance(parser); // identifier
    AstNode* var_node = create_ast_node(AST_NODE_VAR_DECL, id.text, &id.location);
    // Check for assignment operator.
    if (parser_match(parser, TOKEN_ASSIGN)) {
        AstNode* init_expr = parse_expression(parser);
        var_node->right = init_expr;
    }
    // Optionally, consume semicolon.
    if (parser_peek(parser).type == TOKEN_SEMICOLON) {
        parser_advance(parser);
    }
    return var_node;
}

/**
 * Parse a function declaration.
 * Syntax: func <Identifier> ( <parameters> ) { <body> }
 */
static AstNode* parse_func_decl(Parser* parser) {
    Token funcToken = parser_advance(parser); // consume 'func'
    const SourceLocation* funcLoc = &funcToken.location;
    Token id = parser_advance(parser); // function name
    AstNode* func_node = create_ast_node(AST_NODE_FUNC_DECL, id.text, &id.location);
    // Expect '(' for parameters.
    if (!parser_match(parser, TOKEN_LPAREN)) {
        fprintf(stderr, "Error: expected '(' after function name %s at %s:%d\n", id.text, id.location.file, id.location.line);
        return func_node;
    }
    // Parse parameters into a comma-separated string.
    char param_buffer[256] = {0};
    while (parser_peek(parser).type != TOKEN_RPAREN && parser_peek(parser).type != TOKEN_EOF) {
        Token param = parser_advance(parser);
        strncat(param_buffer, param.text, sizeof(param_buffer) - strlen(param_buffer) - 1);
        if (parser_peek(parser).type == TOKEN_COMMA) {
            parser_advance(parser);
            strncat(param_buffer, ",", sizeof(param_buffer) - strlen(param_buffer) - 1);
        }
    }
    // Consume ')'
    parser_match(parser, TOKEN_RPAREN);
    /* Store parameters in the function node's value.
       (Reusing the 'value' field for simplicity.) */
    free(func_node->value);
    func_node->value = strdup(param_buffer);
    // Expect '{' for the function body.
    if (!parser_match(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: expected '{' after function declaration for %s at %s:%d.\n", id.text, id.location.file, id.location.line);
        return func_node;
    }
    // Parse the function body.
    AstNode* body_head = NULL;
    AstNode* body_current = NULL;
    while (parser_peek(parser).type != TOKEN_RBRACE && parser_peek(parser).type != TOKEN_EOF) {
        AstNode* stmt = parse_statement(parser);
        if (!stmt) {
            parser_advance(parser);
            continue;
        }
        if (!body_head) {
            body_head = stmt;
            body_current = stmt;
        } else {
            body_current->next = stmt;
            body_current = stmt;
        }
    }
    // Consume '}'
    if (!parser_match(parser, TOKEN_RBRACE)) {
        fprintf(stderr, "Error: expected '}' at end of function %s at %s:%d.\n", id.text, id.location.file, id.location.line);
    }
    func_node->left = body_head;
    return func_node;
}

/**
 * Parse an if statement.
 * Syntax: if (<condition>) { <then_body> } (else { <else_body> })?
 */
static AstNode* parse_if_statement(Parser* parser) {
    Token ifToken = parser_advance(parser); // consume 'if'
    const SourceLocation* ifLoc = &ifToken.location;
    // Expect '(' for condition.
    if (!parser_match(parser, TOKEN_LPAREN)) {
        fprintf(stderr, "Error: expected '(' after 'if' at %s:%d\n", ifLoc->file, ifLoc->line);
    }
    AstNode* condition = parse_expression(parser);
    if (!parser_match(parser, TOKEN_RPAREN)) {
        fprintf(stderr, "Error: expected ')' after if condition at %s:%d\n", ifLoc->file, ifLoc->line);
    }
    // Expect '{' for then-body.
    if (!parser_match(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: expected '{' for if body at %s:%d\n", ifLoc->file, ifLoc->line);
    }
    AstNode* then_body = NULL;
    AstNode* then_current = NULL;
    while (parser_peek(parser).type != TOKEN_RBRACE && parser_peek(parser).type != TOKEN_EOF) {
        AstNode* stmt = parse_statement(parser);
        if (!stmt) {
            parser_advance(parser);
            continue;
        }
        if (!then_body) {
            then_body = stmt;
            then_current = stmt;
        } else {
            then_current->next = stmt;
            then_current = stmt;
        }
    }
    if (!parser_match(parser, TOKEN_RBRACE)) {
        fprintf(stderr, "Error: expected '}' at end of if body at %s:%d\n", ifLoc->file, ifLoc->line);
    }
    // Create the if node.
    AstNode* if_node = create_ast_node(AST_NODE_IF, NULL, ifLoc);
    if_node->left = condition;
    if_node->right = then_body;
    // Optionally, parse an else clause.
    if (parser_peek(parser).type == TOKEN_ELSE) {
        parser_advance(parser); // consume 'else'
        if (!parser_match(parser, TOKEN_LBRACE)) {
            fprintf(stderr, "Error: expected '{' for else body at %s:%d\n", ifLoc->file, ifLoc->line);
        }
        AstNode* else_body = NULL;
        AstNode* else_current = NULL;
        while (parser_peek(parser).type != TOKEN_RBRACE && parser_peek(parser).type != TOKEN_EOF) {
            AstNode* stmt = parse_statement(parser);
            if (!stmt) {
                parser_advance(parser);
                continue;
            }
            if (!else_body) {
                else_body = stmt;
                else_current = stmt;
            } else {
                else_current->next = stmt;
                else_current = stmt;
            }
        }
        if (!parser_match(parser, TOKEN_RBRACE)) {
            fprintf(stderr, "Error: expected '}' at end of else body at %s:%d\n", ifLoc->file, ifLoc->line);
        }
        // Attach else branch to the if node using the 'next' pointer.
        if_node->next = else_body;
    }
    return if_node;
}

/**
 * Parse a loop statement.
 * Syntax: loop { <body> }
 */
static AstNode* parse_loop_statement(Parser* parser) {
    Token loopToken = parser_advance(parser); // consume 'loop'
    const SourceLocation* loopLoc = &loopToken.location;
    // Expect '{'
    if (!parser_match(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: expected '{' after 'loop' at %s:%d\n", loopLoc->file, loopLoc->line);
    }
    AstNode* body_head = NULL;
    AstNode* body_current = NULL;
    while (parser_peek(parser).type != TOKEN_RBRACE && parser_peek(parser).type != TOKEN_EOF) {
        AstNode* stmt = parse_statement(parser);
        if (!stmt) {
            parser_advance(parser);
            continue;
        }
        if (!body_head) {
            body_head = stmt;
            body_current = stmt;
        } else {
            body_current->next = stmt;
            body_current = stmt;
        }
    }
    if (!parser_match(parser, TOKEN_RBRACE)) {
        fprintf(stderr, "Error: expected '}' at end of loop body at %s:%d\n", loopLoc->file, loopLoc->line);
    }
    AstNode* loop_node = create_ast_node(AST_NODE_LOOP, NULL, loopLoc);
    loop_node->left = body_head;
    return loop_node;
}

/**
 * Parse an error handling block.
 * Syntax: on_error { <handler_body> }
 */
static AstNode* parse_error_handler(Parser* parser) {
    Token errToken = parser_advance(parser); // consume 'on_error'
    const SourceLocation* errLoc = &errToken.location;
    // Expect '{'
    if (!parser_match(parser, TOKEN_LBRACE)) {
        fprintf(stderr, "Error: expected '{' after 'on_error' at %s:%d\n", errLoc->file, errLoc->line);
    }
    AstNode* handler_body = NULL;
    AstNode* handler_current = NULL;
    while (parser_peek(parser).type != TOKEN_RBRACE && parser_peek(parser).type != TOKEN_EOF) {
        AstNode* stmt = parse_statement(parser);
        if (!stmt) {
            parser_advance(parser);
            continue;
        }
        if (!handler_body) {
            handler_body = stmt;
            handler_current = stmt;
        } else {
            handler_current->next = stmt;
            handler_current = stmt;
        }
    }
    if (!parser_match(parser, TOKEN_RBRACE)) {
        fprintf(stderr, "Error: expected '}' at end of on_error block at %s:%d\n", errLoc->file, errLoc->line);
    }
    AstNode* error_node = create_ast_node(AST_NODE_ERROR_HANDLER, NULL, errLoc);
    error_node->left = handler_body;
    return error_node;
}

/**
 * Parse a simple expression.
 * For now, this implementation only handles literal or identifier expressions.
 */
static AstNode* parse_expression(Parser* parser) {
    Token token = parser_advance(parser);
    return create_ast_node(AST_NODE_EXPR, token.text, &token.location);
}
