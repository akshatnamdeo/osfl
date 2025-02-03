#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------
 * TOKEN HELPER DECLARATIONS
 * ------------------------------------------------------------------ */
static Token parser_peek(Parser* parser);
static Token parser_advance(Parser* parser);
static int   parser_match(Parser* parser, TokenType type);
static void  parser_consume(Parser* parser, TokenType type, const char* error_message);

/* ------------------------------------------------------------------
 * FORWARD DECLARATIONS OF PARSE FUNCTIONS
 * ------------------------------------------------------------------ */
static AstNode* parse_program(Parser* parser);
static AstNode* parse_declaration(Parser* parser);
static AstNode* parse_statement(Parser* parser);
static AstNode* parse_block(Parser* parser);

static AstNode* parse_frame(Parser* parser);
static AstNode* parse_var_decl(Parser* parser);
static AstNode* parse_func_decl(Parser* parser);
static AstNode* parse_class_decl(Parser* parser);
static AstNode* parse_import_decl(Parser* parser);

static AstNode* parse_if_stmt(Parser* parser);
static AstNode* parse_while_stmt(Parser* parser);
static AstNode* parse_for_stmt(Parser* parser);
static AstNode* parse_switch_stmt(Parser* parser);
static AstNode* parse_try_catch_stmt(Parser* parser);
static AstNode* parse_on_error_stmt(Parser* parser);

static AstNode* parse_expression_stmt(Parser* parser);

/* Expression grammar with precedence */
static AstNode* parse_expression(Parser* parser);
static AstNode* parse_assignment(Parser* parser);
static AstNode* parse_logical_or(Parser* parser);
static AstNode* parse_logical_and(Parser* parser);
static AstNode* parse_bitwise_or(Parser* parser);
static AstNode* parse_bitwise_xor(Parser* parser);
static AstNode* parse_bitwise_and(Parser* parser);
static AstNode* parse_equality(Parser* parser);
static AstNode* parse_comparison(Parser* parser);
static AstNode* parse_term(Parser* parser);
static AstNode* parse_factor(Parser* parser);
static AstNode* parse_power(Parser* parser);
static AstNode* parse_unary(Parser* parser);
static AstNode* parse_primary(Parser* parser);

/* 
 * Utility for building dynamic arrays of AstNode*
 * used for block statements, function bodies, etc.
 */
static void append_node(AstNode*** array_ptr, size_t* count_ptr, AstNode* node);

/*
 * Helper to create a block node with an array of statements
 */
static AstNode* make_block_node(const SourceLocation* loc, AstNode** stmts, size_t count);

/*
 * Constructors for expressions 
 */
static AstNode* make_expr_literal(const Token* tok);
static AstNode* make_expr_identifier(const Token* tok);
static AstNode* make_expr_binary(TokenType op, AstNode* left, AstNode* right, const SourceLocation* loc);
static AstNode* make_expr_unary(TokenType op, AstNode* expr, const SourceLocation* loc);

/* ------------------------------------------------------------------
 * PARSER LIFECYCLE
 * ------------------------------------------------------------------ */
Parser* parser_create(Token* tokens, size_t token_count) {
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    if (!parser) return NULL;
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    return parser;
}

void parser_destroy(Parser* parser) {
    if (parser) {
        free(parser);
    }
}

AstNode* parser_parse(Parser* parser) {
    return parse_program(parser);
}

/* ------------------------------------------------------------------
 * TOKEN HELPERS
 * ------------------------------------------------------------------ */
static Token parser_peek(Parser* parser) {
    if (parser->current < parser->token_count) {
        return parser->tokens[parser->current];
    }
    Token eof;
    memset(&eof, 0, sizeof(eof));
    eof.type = TOKEN_EOF;
    return eof;
}

static Token parser_advance(Parser* parser) {
    if (parser->current < parser->token_count) {
        return parser->tokens[parser->current++];
    }
    Token eof;
    memset(&eof, 0, sizeof(eof));
    eof.type = TOKEN_EOF;
    return eof;
}

static int parser_match(Parser* parser, TokenType type) {
    if (parser_peek(parser).type == type) {
        parser_advance(parser);
        return 1;
    }
    return 0;
}

static void parser_consume(Parser* parser, TokenType type, const char* error_message) {
    if (!parser_match(parser, type)) {
        Token t = parser_peek(parser);
        fprintf(stderr, "Parse error at %s:%d: %s (got token '%s')\n",
                t.location.file, (int)t.location.line, error_message, t.text);
    }
}

/* ------------------------------------------------------------------
 * UTILITY: Appending an AstNode* to a dynamic array
 * ------------------------------------------------------------------ */
static void append_node(AstNode*** array_ptr, size_t* count_ptr, AstNode* node) {
    if (!node) return;
    size_t old_count = *count_ptr;
    *array_ptr = (AstNode**)realloc(*array_ptr, sizeof(AstNode*) * (old_count + 1));
    (*array_ptr)[old_count] = node;
    *count_ptr = old_count + 1;
}

static AstNode* make_block_node(const SourceLocation* loc, AstNode** stmts, size_t count) {
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_NODE_BLOCK;
    if (loc) node->loc = *loc;
    node->as.block.statement_count = count;
    node->as.block.statements = stmts;
    return node;
}

/* ------------------------------------------------------------------
 * PROGRAM & DECLARATIONS
 * ------------------------------------------------------------------ */
static AstNode* parse_program(Parser* parser) {
    Token startTok = parser_peek(parser);

    AstNode** items = NULL;
    size_t item_count = 0;

    while (parser_peek(parser).type != TOKEN_EOF) {
        AstNode* decl = parse_declaration(parser);
        if (!decl) {
            parser_advance(parser); 
            continue;
        }
        append_node(&items, &item_count, decl);
    }

    return make_block_node(&startTok.location, items, item_count);
}

/* parse_declaration => frame, func, class, import, var/const, else statement fallback */
static AstNode* parse_declaration(Parser* parser) {
    Token t = parser_peek(parser);

    switch (t.type) {
        case TOKEN_FRAME:
            return parse_frame(parser);
        case TOKEN_FUNC:
            return parse_func_decl(parser);
        case TOKEN_CLASS:
            return parse_class_decl(parser);
        case TOKEN_IMPORT:
            return parse_import_decl(parser);
        case TOKEN_VAR:
        case TOKEN_CONST:
            return parse_var_decl(parser);
        default:
            /* Not recognized as a decl => parse statement */
            return parse_statement(parser);
    }
}

/* ------------------------------------------------------------------
 * STATEMENTS
 * ------------------------------------------------------------------ */
static AstNode* parse_statement(Parser* parser) {
    Token t = parser_peek(parser);

    switch (t.type) {
        case TOKEN_IF:    return parse_if_stmt(parser);
        case TOKEN_WHILE: return parse_while_stmt(parser);
        case TOKEN_FOR:   return parse_for_stmt(parser);
        case TOKEN_SWITCH:return parse_switch_stmt(parser);
        case TOKEN_TRY:   return parse_try_catch_stmt(parser);
        case TOKEN_ON_ERROR: return parse_on_error_stmt(parser);
        case TOKEN_LBRACE:return parse_block(parser);
        default:
            return parse_expression_stmt(parser);
    }
}

/* parse_block => { statements } => AST_NODE_BLOCK */
static AstNode* parse_block(Parser* parser) {
    Token brace = parser_advance(parser); /* consume '{' */

    AstNode** stmts = NULL;
    size_t stmt_count = 0;

    while (parser_peek(parser).type != TOKEN_RBRACE &&
           parser_peek(parser).type != TOKEN_EOF)
    {
        AstNode* st = parse_statement(parser);
        if (!st) {
            parser_advance(parser);
            continue;
        }
        append_node(&stmts, &stmt_count, st);
    }
    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after block.");

    return make_block_node(&brace.location, stmts, stmt_count);
}

/* parse_expression_stmt => parse expr, optional semicolon => AST_NODE_EXPR_STMT */
static AstNode* parse_expression_stmt(Parser* parser) {
    AstNode* expr = parse_expression(parser);
    parser_match(parser, TOKEN_SEMICOLON);

    AstNode* stmt = (AstNode*)calloc(1, sizeof(AstNode));
    stmt->type = AST_NODE_EXPR_STMT;
    stmt->loc = expr->loc;
    /* We'll store the expression in the .unary.expr field for convenience (or define your own). */
    stmt->as.unary.expr = expr;
    return stmt;
}

/* Frame => frame <id> { ... } => AST_NODE_FRAME */
static AstNode* parse_frame(Parser* parser) {
    Token frameTok = parser_advance(parser); // consume 'frame'
    Token idTok = parser_advance(parser);    // the name
    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after frame name.");

    AstNode** body = NULL;
    size_t body_count = 0;
    while (parser_peek(parser).type != TOKEN_RBRACE &&
           parser_peek(parser).type != TOKEN_EOF)
    {
        AstNode* s = parse_statement(parser);
        if (!s) {
            parser_advance(parser);
            continue;
        }
        append_node(&body, &body_count, s);
    }
    parser_consume(parser, TOKEN_RBRACE, "Expected '}' at end of frame.");

    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_NODE_FRAME;
    node->loc = frameTok.location;
    node->as.frame_decl.frame_name = strdup(idTok.text);
    node->as.frame_decl.body_statements = body;
    node->as.frame_decl.body_count = body_count;
    return node;
}

/* var|const <id> = expr? ; => AST_NODE_VAR_DECL or AST_NODE_CONST_DECL */
static AstNode* parse_var_decl(Parser* parser) {
    Token declTok = parser_advance(parser);
    bool is_const = (declTok.type == TOKEN_CONST);

    Token nameTok = parser_advance(parser);
    AstNode* init_expr = NULL;
    if (parser_match(parser, TOKEN_ASSIGN)) {
        init_expr = parse_expression(parser);
    }
    parser_match(parser, TOKEN_SEMICOLON);

    AstNode* varNode = (AstNode*)calloc(1, sizeof(AstNode));
    varNode->type = is_const ? AST_NODE_CONST_DECL : AST_NODE_VAR_DECL;
    varNode->loc = declTok.location;
    varNode->as.var_decl.is_const = is_const;
    varNode->as.var_decl.var_name = strdup(nameTok.text);
    varNode->as.var_decl.initializer = init_expr;
    return varNode;
}

/* func <name>(params){body} => AST_NODE_FUNC_DECL */
static AstNode* parse_func_decl(Parser* parser) {
    Token funcTok = parser_advance(parser);
    Token nameTok = parser_advance(parser);

    parser_consume(parser, TOKEN_LPAREN, "Expected '(' after function name.");

    char** params = NULL;
    size_t param_count = 0;
    while (parser_peek(parser).type != TOKEN_RPAREN &&
           parser_peek(parser).type != TOKEN_EOF) 
    {
        Token p = parser_advance(parser);
        params = (char**)realloc(params, sizeof(char*) * (param_count + 1));
        params[param_count] = strdup(p.text);
        param_count++;
        if (!parser_match(parser, TOKEN_COMMA)) {
            break;
        }
    }
    parser_consume(parser, TOKEN_RPAREN, "Expected ')' after parameters.");

    parser_consume(parser, TOKEN_LBRACE, "Expected '{' before function body.");

    AstNode** body_stmts = NULL;
    size_t body_count = 0;
    while (parser_peek(parser).type != TOKEN_RBRACE &&
           parser_peek(parser).type != TOKEN_EOF)
    {
        AstNode* s = parse_statement(parser);
        if (!s) {
            parser_advance(parser);
            continue;
        }
        append_node(&body_stmts, &body_count, s);
    }
    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after function body.");

    /* Build a block node for the body */
    AstNode* bodyBlock = make_block_node(&funcTok.location, body_stmts, body_count);

    AstNode* funcNode = (AstNode*)calloc(1, sizeof(AstNode));
    funcNode->type = AST_NODE_FUNC_DECL;
    funcNode->loc = funcTok.location;
    funcNode->as.func_decl.func_name = strdup(nameTok.text);
    funcNode->as.func_decl.param_count = param_count;
    funcNode->as.func_decl.param_names = params;
    funcNode->as.func_decl.body = bodyBlock;
    return funcNode;
}

/* class <name> { ... } => AST_NODE_CLASS_DECL */
static AstNode* parse_class_decl(Parser* parser) {
    Token classTok = parser_advance(parser);
    Token nameTok = parser_advance(parser);

    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after class name.");

    AstNode** members = NULL;
    size_t member_count = 0;
    while (parser_peek(parser).type != TOKEN_RBRACE &&
           parser_peek(parser).type != TOKEN_EOF)
    {
        AstNode* m = parse_declaration(parser);
        if (!m) {
            parser_advance(parser);
            continue;
        }
        append_node(&members, &member_count, m);
    }
    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after class body.");

    AstNode* classNode = (AstNode*)calloc(1, sizeof(AstNode));
    classNode->type = AST_NODE_CLASS_DECL;
    classNode->loc = classTok.location;
    classNode->as.class_decl.class_name = strdup(nameTok.text);
    classNode->as.class_decl.members = members;
    classNode->as.class_decl.member_count = member_count;
    return classNode;
}

/* import "module"; => AST_EXPR_LITERAL or a custom node */
static AstNode* parse_import_decl(Parser* parser) {
    Token importTok = parser_advance(parser);
    Token modTok = parser_advance(parser);
    parser_match(parser, TOKEN_SEMICOLON);

    /* Let's just treat it as an expression-literal node with literal_type=TOKEN_IMPORT */
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_EXPR_LITERAL; 
    node->loc = importTok.location;
    node->as.literal.literal_type = TOKEN_IMPORT; 
    node->as.literal.str_val = strdup(modTok.text);
    return node;
}

/* if (<cond>) <stmt> (else <stmt>) => AST_NODE_IF */
static AstNode* parse_if_stmt(Parser* parser) {
    Token ifTok = parser_advance(parser); 
    parser_consume(parser, TOKEN_LPAREN, "Expected '(' after 'if'.");
    AstNode* cond = parse_expression(parser);
    parser_consume(parser, TOKEN_RPAREN, "Expected ')' after if condition.");

    AstNode* thenBranch = parse_statement(parser);
    AstNode* elseBranch = NULL;
    if (parser_match(parser, TOKEN_ELSE)) {
        elseBranch = parse_statement(parser);
    }

    AstNode* ifNode = (AstNode*)calloc(1, sizeof(AstNode));
    ifNode->type = AST_NODE_IF;
    ifNode->loc = ifTok.location;
    ifNode->as.if_stmt.condition = cond;
    ifNode->as.if_stmt.then_branch = thenBranch;
    ifNode->as.if_stmt.else_branch = elseBranch;
    return ifNode;
}

/* while (<cond>) <stmt> => AST_NODE_WHILE_STMT */
static AstNode* parse_while_stmt(Parser* parser) {
    Token wTok = parser_advance(parser);
    parser_consume(parser, TOKEN_LPAREN, "Expected '(' after 'while'.");
    AstNode* cond = parse_expression(parser);
    parser_consume(parser, TOKEN_RPAREN, "Expected ')' after while condition.");

    AstNode* body = parse_statement(parser);

    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_NODE_WHILE_STMT;
    node->loc = wTok.location;
    node->as.while_stmt.condition = cond;
    node->as.while_stmt.body = body;
    return node;
}

/* for (<init>; <cond>; <incr>) <stmt> => AST_NODE_FOR_STMT */
static AstNode* parse_for_stmt(Parser* parser) {
    Token fTok = parser_advance(parser);
    parser_consume(parser, TOKEN_LPAREN, "Expected '(' after 'for'.");
    AstNode* init = parse_expression(parser);
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after for initializer.");
    AstNode* cond = parse_expression(parser);
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after for condition.");
    AstNode* incr = parse_expression(parser);
    parser_consume(parser, TOKEN_RPAREN, "Expected ')' after for clauses.");
    AstNode* body = parse_statement(parser);

    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_NODE_FOR_STMT;
    node->loc = fTok.location;
    node->as.for_stmt.init = init;
    node->as.for_stmt.condition = cond;
    node->as.for_stmt.increment = incr;
    node->as.for_stmt.body = body;
    return node;
}

/* switch (<expr>) { ... } => partial example => store as AST_EXPR_BINARY with op=TOKEN_SWITCH or your custom node */
static AstNode* parse_switch_stmt(Parser* parser) {
    Token swTok = parser_advance(parser);
    parser_consume(parser, TOKEN_LPAREN, "Expected '(' after 'switch'.");
    AstNode* expr = parse_expression(parser);
    parser_consume(parser, TOKEN_RPAREN, "Expected ')' after switch expr.");

    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after switch(...).");
    /* parse cases (we won't show every detail here) */
    AstNode** cases = NULL;
    size_t case_count = 0;
    while (parser_peek(parser).type != TOKEN_RBRACE &&
           parser_peek(parser).type != TOKEN_EOF)
    {
        AstNode* c = parse_statement(parser);
        if (!c) {
            parser_advance(parser);
            continue;
        }
        append_node(&cases, &case_count, c);
    }
    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after switch.");

    /* For demonstration, store them in a block node. */
    AstNode* caseBlock = make_block_node(&swTok.location, cases, case_count);

    /* If you have a dedicated union field, use that. 
       We'll do an AST_EXPR_BINARY with op=TOKEN_SWITCH, left=expr, right=caseBlock. */
    AstNode* switchNode = make_expr_binary(TOKEN_SWITCH, expr, caseBlock, &swTok.location);
    return switchNode;
}

/* try <stmt> catch <stmt>? => AST_NODE_TRY_CATCH */
static AstNode* parse_try_catch_stmt(Parser* parser) {
    Token tryTok = parser_advance(parser); // 'try'
    AstNode* tryBlock = parse_statement(parser);

    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_NODE_TRY_CATCH;
    node->loc = tryTok.location;
    node->as.error_handler.handler_body = tryBlock; 

    if (parser_match(parser, TOKEN_CATCH)) {
        /* parse catch statement => we might store it in node->next_sibling or a new union field. 
           This example won't demonstrate everything. */
        AstNode* catchBlock = parse_statement(parser);
        node->next_sibling = catchBlock;
    }
    return node;
}

/* on_error { ... } => AST_NODE_ERROR_HANDLER */
static AstNode* parse_on_error_stmt(Parser* parser) {
    Token errTok = parser_advance(parser);
    parser_consume(parser, TOKEN_LBRACE, "Expected '{' after on_error.");

    AstNode** stmts = NULL;
    size_t stmt_count = 0;
    while (parser_peek(parser).type != TOKEN_RBRACE &&
           parser_peek(parser).type != TOKEN_EOF)
    {
        AstNode* st = parse_statement(parser);
        if (!st) {
            parser_advance(parser);
            continue;
        }
        append_node(&stmts, &stmt_count, st);
    }
    parser_consume(parser, TOKEN_RBRACE, "Expected '}' after on_error block.");

    /* store them in a block node */
    AstNode* block = make_block_node(&errTok.location, stmts, stmt_count);

    AstNode* errNode = (AstNode*)calloc(1, sizeof(AstNode));
    errNode->type = AST_NODE_ERROR_HANDLER;
    errNode->loc = errTok.location;
    errNode->as.error_handler.handler_body = block;
    return errNode;
}

/* ------------------------------------------------------------------
 * EXPRESSIONS
 * ------------------------------------------------------------------ */
static AstNode* parse_expression(Parser* parser) {
    return parse_assignment(parser);
}

static AstNode* parse_assignment(Parser* parser) {
    AstNode* left = parse_logical_or(parser);
    Token t = parser_peek(parser);

    if (t.type == TOKEN_ASSIGN    || t.type == TOKEN_PLUS_ASSIGN ||
        t.type == TOKEN_MINUS_ASSIGN || t.type == TOKEN_STAR_ASSIGN ||
        t.type == TOKEN_SLASH_ASSIGN || t.type == TOKEN_MOD_ASSIGN)
    {
        parser_advance(parser);
        AstNode* right = parse_assignment(parser);
        return make_expr_binary(t.type, left, right, &t.location);
    }
    return left;
}

static AstNode* parse_logical_or(Parser* parser) {
    AstNode* node = parse_logical_and(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_OR) {
            parser_advance(parser);
            AstNode* rhs = parse_logical_and(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_logical_and(Parser* parser) {
    AstNode* node = parse_bitwise_or(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_AND) {
            parser_advance(parser);
            AstNode* rhs = parse_bitwise_or(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_bitwise_or(Parser* parser) {
    AstNode* node = parse_bitwise_xor(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_BIT_OR) {
            parser_advance(parser);
            AstNode* rhs = parse_bitwise_xor(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_bitwise_xor(Parser* parser) {
    AstNode* node = parse_bitwise_and(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_BIT_XOR) {
            parser_advance(parser);
            AstNode* rhs = parse_bitwise_and(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_bitwise_and(Parser* parser) {
    AstNode* node = parse_equality(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_BIT_AND) {
            parser_advance(parser);
            AstNode* rhs = parse_equality(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_equality(Parser* parser) {
    AstNode* node = parse_comparison(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_EQ || t.type == TOKEN_NEQ) {
            parser_advance(parser);
            AstNode* rhs = parse_comparison(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_comparison(Parser* parser) {
    AstNode* node = parse_term(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_LT || t.type == TOKEN_GT ||
            t.type == TOKEN_LTE || t.type == TOKEN_GTE)
        {
            parser_advance(parser);
            AstNode* rhs = parse_term(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_term(Parser* parser) {
    AstNode* node = parse_factor(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_PLUS || t.type == TOKEN_MINUS) {
            parser_advance(parser);
            AstNode* rhs = parse_factor(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_factor(Parser* parser) {
    AstNode* node = parse_power(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_STAR || t.type == TOKEN_SLASH || t.type == TOKEN_PERCENT) {
            parser_advance(parser);
            AstNode* rhs = parse_power(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_power(Parser* parser) {
    AstNode* node = parse_unary(parser);
    for (;;) {
        Token t = parser_peek(parser);
        if (t.type == TOKEN_POW) {
            parser_advance(parser);
            AstNode* rhs = parse_unary(parser);
            node = make_expr_binary(t.type, node, rhs, &t.location);
        } else {
            break;
        }
    }
    return node;
}

static AstNode* parse_unary(Parser* parser) {
    Token t = parser_peek(parser);
    if (t.type == TOKEN_MINUS || t.type == TOKEN_PLUS ||
        t.type == TOKEN_NOT   || t.type == TOKEN_BIT_NOT ||
        t.type == TOKEN_INCREMENT || t.type == TOKEN_DECREMENT)
    {
        parser_advance(parser);
        AstNode* sub = parse_unary(parser);
        return make_expr_unary(t.type, sub, &t.location);
    }
    return parse_primary(parser);
}

/* parse_primary => docstring, regex, literal, identifier, interpolation, parentheses, etc. */
static AstNode* parse_primary(Parser* parser) {
    Token t = parser_peek(parser);

    switch (t.type) {
        case TOKEN_DOCSTRING: {
            parser_advance(parser);
            /* If you want a distinct node type, we do AST_NODE_DOCSTRING. 
               Or you treat it as a literal with literal_type=TOKEN_DOCSTRING. */
            AstNode* docNode = (AstNode*)calloc(1, sizeof(AstNode));
            docNode->type = AST_NODE_DOCSTRING;
            docNode->loc = t.location;
            docNode->as.literal.literal_type = TOKEN_DOCSTRING;
            docNode->as.literal.str_val = strdup(t.text);
            return docNode;
        }
        case TOKEN_REGEX: {
            parser_advance(parser);
            AstNode* regNode = (AstNode*)calloc(1, sizeof(AstNode));
            regNode->type = AST_NODE_REGEX;
            regNode->loc = t.location;
            regNode->as.literal.literal_type = TOKEN_REGEX;
            regNode->as.literal.str_val = strdup(t.text);
            return regNode;
        }
        case TOKEN_INTERPOLATION_START: {
            Token startTok = parser_advance(parser); 
            AstNode* innerExpr = parse_expression(parser);
            parser_consume(parser, TOKEN_INTERPOLATION_END, "Expected '}' after interpolation expression.");
            /* We'll create an AST_EXPR_INTERPOLATION node. */
            AstNode* interp = (AstNode*)calloc(1, sizeof(AstNode));
            interp->type = AST_EXPR_INTERPOLATION;
            interp->loc = startTok.location;
            interp->as.interpolation.expr = innerExpr;
            return interp;
        }
        case TOKEN_LPAREN: {
            parser_advance(parser);
            AstNode* e = parse_expression(parser);
            parser_consume(parser, TOKEN_RPAREN, "Expected ')' after parenthesized expr.");
            return e;
        }
        case TOKEN_INTEGER:
        case TOKEN_FLOAT:
        case TOKEN_STRING:
        case TOKEN_BOOL_TRUE:
        case TOKEN_BOOL_FALSE:
        {
            /* treat as a literal expression */
            Token litTok = parser_advance(parser);
            return make_expr_literal(&litTok);
        }
        case TOKEN_IDENTIFIER: {
            Token idTok = parser_advance(parser);
            return make_expr_identifier(&idTok);
        }
        default:
            /* fallback => error node or skip */
            fprintf(stderr, "Parse error at %s:%d: unexpected token '%s'\n",
                    t.location.file, (int)t.location.line, t.text);
            parser_advance(parser);
            return NULL;
    }
}

/* ------------------------------------------------------------------
 * Constructors for expressions
 * ------------------------------------------------------------------ */
static AstNode* make_expr_literal(const Token* tok) {
    /* build an AST_EXPR_LITERAL node */
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_EXPR_LITERAL;
    node->loc = tok->location;
    node->as.literal.literal_type = tok->type;
    switch (tok->type) {
        case TOKEN_INTEGER:
            node->as.literal.i64_val = atoll(tok->text);
            break;
        case TOKEN_FLOAT:
            node->as.literal.f64_val = atof(tok->text);
            break;
        case TOKEN_BOOL_TRUE:
            node->as.literal.bool_val = true;
            break;
        case TOKEN_BOOL_FALSE:
            node->as.literal.bool_val = false;
            break;
        case TOKEN_STRING:
            node->as.literal.str_val = strdup(tok->text);
            break;
        default:
            /* If docstring or regex appear here, handle them, or fallback */
            node->as.literal.str_val = strdup(tok->text);
            break;
    }
    return node;
}

static AstNode* make_expr_identifier(const Token* tok) {
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_EXPR_IDENTIFIER;
    node->loc = tok->location;
    node->as.ident.name = strdup(tok->text);
    return node;
}

static AstNode* make_expr_binary(TokenType op, AstNode* left, AstNode* right, const SourceLocation* loc) {
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_EXPR_BINARY;
    if (loc) node->loc = *loc;
    node->as.binary.op = op;
    node->as.binary.left = left;
    node->as.binary.right = right;
    return node;
}

static AstNode* make_expr_unary(TokenType op, AstNode* expr, const SourceLocation* loc) {
    AstNode* node = (AstNode*)calloc(1, sizeof(AstNode));
    node->type = AST_EXPR_UNARY;
    if (loc) node->loc = *loc;
    node->as.unary.op = op;
    node->as.unary.expr = expr;
    return node;
}
