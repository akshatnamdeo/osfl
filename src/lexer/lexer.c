/************************************************************
 *  File: lexer.c
 *  Description:
 *      OSFL Lexer implementation for tokenizing source code.
 *      This file implements all required functionalities
 *      outlined in the OSFL Lexer Implementation Specification,
 *      with additional changes for memory safety, error handling,
 *      and token location tracking.
 *
 *  Dependencies:
 *      - lexer.h
 *      - token.h
 *      - string.h, stdlib.h, ctype.h, stdio.h
 *
 *  Important Changes Added:
 *      1. SourceLocation setting in all token creation points
 *         (token.location.line, token.location.column, token.location.file).
 *      2. Buffer overflow handling in safe_append_char() helper function.
 *      3. String value memory safety in scan_string (using strdup and
 *         error checking).
 *      4. Additional LexerError enum values: LEXER_ERROR_BUFFER_OVERFLOW,
 *         LEXER_ERROR_MEMORY (must be added to lexer.h).
 *      5. Token memory management with token_cleanup function to free
 *         string data when necessary.
 ************************************************************/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>   /* For va_list in set_error */
#include "lexer.h"
#include "token.h"

/* 4 spaces for indentation throughout the file */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* ----------------------------------------------------------
Forward Declarations
---------------------------------------------------------- */

/* We must declare set_error() before using it in any helper. */
static void set_error(Lexer* lexer, LexerErrorType type, const char* fmt, ...);

/* ----------------------------------------------------------
Lexer Structure Definition
---------------------------------------------------------- */

struct Lexer {
    /* Source code management */
    const char* source;
    size_t      length;
    size_t      position;
    size_t      line;
    size_t      column;
    
    /* Token reading state */
    char        current;
    char        peek;
    
    /* Error handling */
    LexerError  error;
    char        error_buffer[LEXER_MAX_ERROR_LENGTH];
    
    /* Configuration */
    LexerConfig config;
    
    /* Internal buffers */
    char        string_buffer[LEXER_MAX_STRING_LENGTH];
};

/* ----------------------------------------------------------
Prototypes for Internal Functions
---------------------------------------------------------- */

/* Character management */
static void advance(Lexer* lexer);
static int  is_at_end(Lexer* lexer);

/* Token recognition */
static Token lexer_next_token_internal(Lexer* lexer);
static Token scan_identifier(Lexer* lexer, Token token);
static Token scan_number(Lexer* lexer, Token token);
static Token scan_string(Lexer* lexer, Token token);
static Token scan_operator(Lexer* lexer);
static Token scan_token_dispatch(Lexer* lexer, Token token);

/* Helper functions */
static int  is_digit(char c);
static int  is_alpha_(char c);
static int  is_identifier_part(char c);
static void skip_whitespace(Lexer* lexer);
static void skip_comments(Lexer* lexer);
static Token handle_multi_char_operator(Lexer* lexer, TokenType type, const char* text);
static void safe_append_char(Lexer* lexer, char c, size_t* length);

/* Error handling */
static void clear_error(Lexer* lexer);

/* ----------------------------------------------------------
Error Handling (Definition of set_error)
---------------------------------------------------------- */

static void set_error(Lexer* lexer, LexerErrorType type, const char* fmt, ...)
{
    if (!lexer) {
        return;
    }

    lexer->error.type = type;

    va_list args;
    va_start(args, fmt);
    vsnprintf(lexer->error.message, LEXER_MAX_ERROR_LENGTH, fmt, args);
    va_end(args);

    lexer->error.location.line = lexer->line;
    lexer->error.location.column = lexer->column;
    lexer->error.location.file = lexer->config.file_name;
}

/* ----------------------------------------------------------
Keyword Table
---------------------------------------------------------- */

typedef struct {
    const char* keyword;
    TokenType   type;
} KeywordEntry;

static const KeywordEntry KEYWORDS[] = {
    {"frame",    TOKEN_FRAME},
    {"in",       TOKEN_IN},
    {"var",      TOKEN_VAR},
    {"const",    TOKEN_CONST},
    {"func",     TOKEN_FUNC},
    {"return",   TOKEN_RETURN},
    {"if",       TOKEN_IF},
    {"else",     TOKEN_ELSE},
    {"loop",     TOKEN_LOOP},
    {"break",    TOKEN_BREAK},
    {"continue", TOKEN_CONTINUE},
    {"on_error", TOKEN_ON_ERROR},
    {"retry",    TOKEN_RETRY},
    {"reset",    TOKEN_RESET},
    {"null",     TOKEN_NULL},
    {NULL,       TOKEN_EOF}  /* Sentinel */
};

/* ----------------------------------------------------------
Lexer Lifecycle Management
---------------------------------------------------------- */

LexerConfig lexer_default_config(void)
{
    LexerConfig config = {0};
    config.skip_whitespace = true;
    config.include_comments = false;
    config.track_line_endings = true;
    config.tab_width = 4;
    config.file_name = "input.osfl";
    return config;
}

Lexer* lexer_create(const char* source, size_t length, LexerConfig config)
{
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    if (!lexer) {
        return NULL;  /* Allocation failed */
    }

    /* Initialize fields */
    lexer->source = source;
    lexer->length = (source == NULL) ? 0 : length;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->current = (lexer->length > 0) ? source[0] : '\0';
    lexer->peek = (lexer->length > 1) ? source[1] : '\0';
    lexer->error.type = LEXER_ERROR_NONE;
    lexer->error.message[0] = '\0';
    lexer->error.location.line = lexer->line;
    lexer->error.location.column = lexer->column;
    lexer->error.location.file = config.file_name;

    /* Copy entire config */
    lexer->config = config;

    /* Clear buffers */
    memset(lexer->string_buffer, 0, LEXER_MAX_STRING_LENGTH);

    return lexer;
}

void lexer_destroy(Lexer* lexer)
{
    if (lexer) {
        free(lexer);
    }
}

void lexer_reset(Lexer* lexer, const char* new_source)
{
    if (!lexer) {
        return;
    }
    clear_error(lexer);

    if (new_source) {
        lexer->source = new_source;
        lexer->length = strlen(new_source);
    } else {
        lexer->source = NULL;
        lexer->length = 0;
    }

    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->current = (lexer->length > 0) ? lexer->source[0] : '\0';
    lexer->peek = (lexer->length > 1) ? lexer->source[1] : '\0';
    memset(lexer->string_buffer, 0, LEXER_MAX_STRING_LENGTH);
    memset(lexer->error_buffer, 0, LEXER_MAX_ERROR_LENGTH);
    lexer->error.type = LEXER_ERROR_NONE;
    lexer->error.message[0] = '\0'; // Clear the error message
    lexer->error.location.line = lexer->line;
    lexer->error.location.column = lexer->column;
    lexer->error.location.file = lexer->config.file_name;
}

/* ----------------------------------------------------------
Character Management
---------------------------------------------------------- */

static void advance(Lexer* lexer) {
    if (!is_at_end(lexer)) {
        // First increment position
        lexer->position++;
        
        // Update current and peek
        lexer->current = (lexer->position < lexer->length) 
                      ? lexer->source[lexer->position] 
                      : '\0';
        lexer->peek = (lexer->position + 1 < lexer->length)
                    ? lexer->source[lexer->position + 1]
                    : '\0';

        // Update column for the character we're moving past
        if (lexer->position > 0 && lexer->source[lexer->position - 1] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
    }
}

static int is_at_end(Lexer* lexer)
{
    return (lexer->position >= lexer->length);
}

/* ----------------------------------------------------------
Token Recognition
---------------------------------------------------------- */

Token lexer_next_token(Lexer* lexer)
{
    clear_error(lexer);
    return lexer_next_token_internal(lexer);
}

Token lexer_peek_token(Lexer* lexer)
{
    size_t saved_position = lexer->position;
    size_t saved_line = lexer->line;
    size_t saved_column = lexer->column;
    char   saved_current = lexer->current;
    char   saved_peek = lexer->peek;
    LexerError saved_error = lexer->error;
    char saved_error_buffer[LEXER_MAX_ERROR_LENGTH];
    memcpy(saved_error_buffer, lexer->error_buffer, LEXER_MAX_ERROR_LENGTH);

    Token t = lexer_next_token_internal(lexer);

    /* Restore the lexer state */
    lexer->position = saved_position;
    lexer->line = saved_line;
    lexer->column = saved_column;
    lexer->current = saved_current;
    lexer->peek = saved_peek;
    lexer->error = saved_error;
    memcpy(lexer->error_buffer, saved_error_buffer, LEXER_MAX_ERROR_LENGTH);

    return t;
}

static Token lexer_next_token_internal(Lexer* lexer) {
    Token token;
    memset(&token, 0, sizeof(Token));

    skip_whitespace(lexer);
    skip_comments(lexer);

    if (is_at_end(lexer)) {
        token.type = TOKEN_EOF;
        token.location.line = (uint32_t)lexer->line;
        token.location.column = (uint32_t)lexer->column;
        token.location.file = lexer->config.file_name;
        token.text[0] = '\0';
        return token;
    }

    // Set token location BEFORE we process the token
    token.location.line = (uint32_t)lexer->line;
    token.location.column = (uint32_t)lexer->column;
    token.location.file = lexer->config.file_name;

    // Handle newline token
    if (lexer->current == '\n') {
        // Consume the newline and update the lexer's state.
        advance(lexer);
        token.type = TOKEN_NEWLINE;
        strcpy_s(token.text, sizeof(token.text), "\n");
        token.location.line = lexer->line;    // Now line has been incremented.
        token.location.column = lexer->column;  // Now column is 1.
        token.location.file = lexer->config.file_name;
        return token;
    }

    return scan_token_dispatch(lexer, token);
}

static Token scan_token_dispatch(Lexer* lexer, Token token)
{
    char c = lexer->current;
    if (is_alpha_(c)) {
            return scan_identifier(lexer, token);
    } else if (is_digit(c)) {
            return scan_number(lexer, token);
    } else if (c == '"') {
            return scan_string(lexer, token);
    } else {
            return scan_operator(lexer);
    }
}

/* ----------------------------------------------------------
Specific Token Scanning
---------------------------------------------------------- */

static Token scan_identifier(Lexer* lexer, Token token) {
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t length = 0;

    while (!is_at_end(lexer) && is_identifier_part(lexer->current)) {
        if (length < 63) {
            buffer[length++] = lexer->current;
            buffer[length] = '\0';
        }
        advance(lexer);
    }

    // Check for keywords
    token.type = TOKEN_IDENTIFIER;
    for (int i = 0; KEYWORDS[i].keyword != NULL; i++) {
        if (strcmp(buffer, KEYWORDS[i].keyword) == 0) {
            token.type = KEYWORDS[i].type;
            break;
        }
    }

    // Handle boolean literals specially
    if (strcmp(buffer, "true") == 0) {
        token.type = TOKEN_BOOL_TRUE;
        token.value.bool_value = true;
    } else if (strcmp(buffer, "false") == 0) {
        token.type = TOKEN_BOOL_FALSE;
        token.value.bool_value = false;
    }

    strcpy_s(token.text, sizeof(token.text), buffer);
    return token;
}

static Token scan_number(Lexer* lexer, Token token) {
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t length = 0;
    bool is_float = false;

    // Collect digits before decimal
    while (!is_at_end(lexer) && is_digit(lexer->current)) {
        if (length < sizeof(buffer) - 1) {
            buffer[length++] = lexer->current;
        }
        advance(lexer);
    }

    // Check for decimal point
    if (!is_at_end(lexer) && lexer->current == '.' && !is_at_end(lexer) && is_digit(lexer->peek)) {
        is_float = true;
        if (length < sizeof(buffer) - 1) {
            buffer[length++] = lexer->current;
        }
        advance(lexer);

        // Collect digits after decimal
        while (!is_at_end(lexer) && is_digit(lexer->current)) {
            if (length < sizeof(buffer) - 1) {
                buffer[length++] = lexer->current;
            }
            advance(lexer);
        }
    }

    buffer[length] = '\0';

    if (is_float) {
        token.type = TOKEN_FLOAT;
        token.value.float_value = atof(buffer);
    } else {
        token.type = TOKEN_INTEGER;
        token.value.int_value = atoll(buffer);
    }

    strcpy_s(token.text, sizeof(token.text), buffer);
    return token;
}

static Token scan_string(Lexer* lexer, Token token) {
    token.type = TOKEN_STRING;
    size_t start_column = lexer->column;

    // Skip opening quote
    advance(lexer);

    size_t length = 0;
    memset(lexer->string_buffer, 0, LEXER_MAX_STRING_LENGTH);

    while (!is_at_end(lexer) && lexer->current != '"') {
        if (length >= LEXER_MAX_STRING_LENGTH - 1) {
            set_error(lexer, LEXER_ERROR_BUFFER_OVERFLOW,
                     "String literal exceeds maximum length of %d at line %zu, column %zu.",
                     LEXER_MAX_STRING_LENGTH - 1, lexer->line, start_column);
            token.type = TOKEN_ERROR;
            return token;
        }

        if (lexer->current == '\\') {
            advance(lexer);
            if (is_at_end(lexer)) {
                set_error(lexer, LEXER_ERROR_UNTERMINATED_STRING,
                         "Unterminated string literal at line %zu, col %zu.",
                         lexer->line, start_column);
                token.type = TOKEN_ERROR;
                return token;
            }

            // Handle escape sequences
            switch (lexer->current) {
                case 'n': safe_append_char(lexer, '\n', &length); break;
                case 't': safe_append_char(lexer, '\t', &length); break;
                case '\\': safe_append_char(lexer, '\\', &length); break;
                case '"': safe_append_char(lexer, '"', &length); break;
                default:
                    set_error(lexer, LEXER_ERROR_INVALID_ESCAPE,
                             "Invalid escape sequence \\%c at line %zu, col %zu",
                             lexer->current, lexer->line, lexer->column);
                    token.type = TOKEN_ERROR;
                    return token;
            }
        } else {
            safe_append_char(lexer, lexer->current, &length);
        }
        advance(lexer);
    }

    if (is_at_end(lexer)) {
        set_error(lexer, LEXER_ERROR_UNTERMINATED_STRING,
                 "Unterminated string literal at line %zu, col %zu.",
                 lexer->line, start_column);
        token.type = TOKEN_ERROR;
        return token;
    }

    // Skip closing quote
    advance(lexer);

    strncpy_s(token.text, sizeof(token.text), lexer->string_buffer, sizeof(token.text) - 1);
    token.value.string_value.data = _strdup(lexer->string_buffer);
    if (!token.value.string_value.data) {
        set_error(lexer, LEXER_ERROR_MEMORY, "Failed to allocate string memory");
        token.type = TOKEN_ERROR;
        return token;
    }
    token.value.string_value.length = length;

    return token;
}

static Token scan_operator(Lexer* lexer) {
    char c = lexer->current;
    char p = lexer->peek;

    Token token;
    memset(&token, 0, sizeof(Token));
    token.location.line = lexer->line;
    token.location.column = lexer->column;
    token.location.file = lexer->config.file_name;

    // Multi-char operators
    if (c == '+' && p == '+') return handle_multi_char_operator(lexer, TOKEN_INCREMENT, "++");
    if (c == '-' && p == '-') return handle_multi_char_operator(lexer, TOKEN_DECREMENT, "--");
    if (c == '=' && p == '=') return handle_multi_char_operator(lexer, TOKEN_EQ, "==");
    if (c == '!' && p == '=') return handle_multi_char_operator(lexer, TOKEN_NEQ, "!=");
    if (c == '<' && p == '=') return handle_multi_char_operator(lexer, TOKEN_LTE, "<=");
    if (c == '>' && p == '=') return handle_multi_char_operator(lexer, TOKEN_GTE, ">=");
    if (c == '&' && p == '&') return handle_multi_char_operator(lexer, TOKEN_AND, "&&");
    if (c == '|' && p == '|') return handle_multi_char_operator(lexer, TOKEN_OR, "||");
    if (c == '+' && p == '=') return handle_multi_char_operator(lexer, TOKEN_PLUS_ASSIGN, "+=");
    if (c == '-' && p == '=') return handle_multi_char_operator(lexer, TOKEN_MINUS_ASSIGN, "-=");
    if (c == '*' && p == '=') return handle_multi_char_operator(lexer, TOKEN_STAR_ASSIGN, "*=");
    if (c == '/' && p == '=') return handle_multi_char_operator(lexer, TOKEN_SLASH_ASSIGN, "/=");
    if (c == '%' && p == '=') return handle_multi_char_operator(lexer, TOKEN_MOD_ASSIGN, "%=");
    if (c == '-' && p == '>') return handle_multi_char_operator(lexer, TOKEN_ARROW, "->");
    if (c == '=' && p == '>') return handle_multi_char_operator(lexer, TOKEN_DOUBLE_ARROW, "=>");
    if (c == ':' && p == ':') return handle_multi_char_operator(lexer, TOKEN_DOUBLE_COLON, "::");

    // Single-char operators
    switch (c) {
        case '+': token.type = TOKEN_PLUS; strcpy_s(token.text, sizeof(token.text), "+"); break;
        case '-': token.type = TOKEN_MINUS; strcpy_s(token.text, sizeof(token.text), "-"); break;
        case '*': token.type = TOKEN_STAR; strcpy_s(token.text, sizeof(token.text), "*"); break;
        case '/': token.type = TOKEN_SLASH; strcpy_s(token.text, sizeof(token.text), "/"); break;
        case '%': token.type = TOKEN_PERCENT; strcpy_s(token.text, sizeof(token.text), "%"); break;
        case '=': token.type = TOKEN_ASSIGN; strcpy_s(token.text, sizeof(token.text), "="); break;
        case '!': token.type = TOKEN_NOT; strcpy_s(token.text, sizeof(token.text), "!"); break;
        case '<': token.type = TOKEN_LT; strcpy_s(token.text, sizeof(token.text), "<"); break;
        case '>': token.type = TOKEN_GT; strcpy_s(token.text, sizeof(token.text), ">"); break;
        case '(': token.type = TOKEN_LPAREN; strcpy_s(token.text, sizeof(token.text), "("); break;
        case ')': token.type = TOKEN_RPAREN; strcpy_s(token.text, sizeof(token.text), ")"); break;
        case '{': token.type = TOKEN_LBRACE; strcpy_s(token.text, sizeof(token.text), "{"); break;
        case '}': token.type = TOKEN_RBRACE; strcpy_s(token.text, sizeof(token.text), "}"); break;
        case ';': token.type = TOKEN_SEMICOLON; strcpy_s(token.text, sizeof(token.text), ";"); break;
        case ':': token.type = TOKEN_COLON; strcpy_s(token.text, sizeof(token.text), ":"); break;
        case ',': token.type = TOKEN_COMMA; strcpy_s(token.text, sizeof(token.text), ","); break;
        case '.': token.type = TOKEN_DOT; strcpy_s(token.text, sizeof(token.text), "."); break;
        default:
            token.type = TOKEN_ERROR;
            token.text[0] = c;
            token.text[1] = '\0';
            set_error(lexer, LEXER_ERROR_INVALID_CHAR,
                     "Invalid character '%c' at line %zu, column %zu",
                     c, lexer->line, lexer->column);
            break;
    }
    
    advance(lexer);
    return token;
}

/* ----------------------------------------------------------
Helper Functions
---------------------------------------------------------- */

static Token handle_multi_char_operator(Lexer* lexer, TokenType type, const char* text) {
    Token token;
    memset(&token, 0, sizeof(Token));
    
    // Save token information
    token.type = type;
    token.location.line = lexer->line;
    token.location.column = lexer->column;
    token.location.file = lexer->config.file_name;
    strcpy_s(token.text, sizeof(token.text), text);
    
    // Calculate length once
    size_t len = strlen(text);
    
    // First advance - saves initial position
    lexer->position += len;
    
    // Update lexer state
    lexer->current = (lexer->position < lexer->length) 
                   ? lexer->source[lexer->position] 
                   : '\0';
    lexer->peek = (lexer->position + 1 < lexer->length)
                ? lexer->source[lexer->position + 1]
                : '\0';
                
    // Single column update for entire operator
    lexer->column += len;
    
    return token;
}

static void safe_append_char(Lexer* lexer, char c, size_t* length) {
    if (*length < LEXER_MAX_STRING_LENGTH - 1) {
        lexer->string_buffer[(*length)++] = c;
        lexer->string_buffer[*length] = '\0';
    }
}

static int is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

LexerError lexer_get_error(const Lexer* lexer)
{
    return lexer->error;
}

static int is_alpha_(char c)
{
    return ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
             c == '_');
}

static int is_identifier_part(char c)
{
    return is_alpha_(c) || is_digit(c);
}

/**
 * skip_whitespace:
 * Each whitespace character (space, tab, carriage return)
 * increments column by 1. For newline, line++ and column++.
 */
static void skip_whitespace(Lexer* lexer) {
    while (!is_at_end(lexer)) {
        switch (lexer->current) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            default:
                return;
        }
    }
}

static void skip_comments(Lexer* lexer)
{
        while (!is_at_end(lexer)) {
                /* Single-line comment // */
                if (lexer->current == '/' && lexer->peek == '/') {
                        if (lexer->config.include_comments) {
                                return; /* If including comments as tokens, don't skip them. */
                        }
                        /* Otherwise, skip until newline */
                        while (!is_at_end(lexer) && lexer->current != '\n') {
                                lexer->column++;
                                advance(lexer);
                        }
                }
                else if (lexer->current == '/' && lexer->peek == '*') {
                        if (lexer->config.include_comments) {
                                return; /* Return if you want them as tokens. */
                        }
                        lexer->column += 2;
                        advance(lexer);
                        advance(lexer);

                        int closed = 0;
                        while (!is_at_end(lexer)) {
                                if (lexer->current == '\n') {
                                        /* Newline => line++ and reset column=1. */
                                        lexer->line++;
                                        lexer->column = 1;
                                        advance(lexer);
                                        continue;
                                }
                                if (lexer->current == '*' && lexer->peek == '/') {
                                        lexer->column += 2;
                                        advance(lexer);
                                        advance(lexer);
                                        closed = 1;
                                        break;
                                }
                                lexer->column++;
                                advance(lexer);
                        }
                        if (!closed) {
                                /* Unterminated comment error */
                                set_error(lexer, LEXER_ERROR_UNTERMINATED_COMMENT,
                                          "Unterminated multi-line comment at line %zu, col %zu.",
                                          lexer->line, lexer->column);
                        }
                }
                else {
                        /* No comment prefix => stop skipping */
                        return;
                }

                /* After skipping a comment, also skip trailing whitespace. */
                skip_whitespace(lexer);
        }
}

/* ----------------------------------------------------------
Error Handling (Clear)
---------------------------------------------------------- */

static void clear_error(Lexer* lexer)
{
    lexer->error.type = LEXER_ERROR_NONE;
    lexer->error.message[0] = '\0';
    lexer->error.location.line = lexer->line;
    lexer->error.location.column = lexer->column;
    lexer->error.location.file = lexer->config.file_name;
    lexer->error_buffer[0] = '\0';
}

/* ----------------------------------------------------------
Token Memory Management
---------------------------------------------------------- */

void lexer_token_cleanup(Token* token) {
    if (token->type == TOKEN_STRING && token->value.string_value.data) {
        free((void*)token->value.string_value.data);
        token->value.string_value.data = NULL;
    }
    // No cleanup needed for newline tokens
}