/************************************************************
 *  File: lexer.c
 *  Description:
 *      OSFL Lexer implementation for tokenizing source code.
 *      This version is enhanced to handle:
 *        - Unicode/UTF-8 identifiers (simple approach)
 *        - String interpolation with "${...}"
 *        - Docstrings (triple-quoted strings)
 *        - Additional numeric formats (hex, oct, bin, underscores, scientific)
 *        - Regex literal (e.g., /[A-Za-z]+/)
 *        - Bitwise operators (&, |, ^, ~)
 *        - Exponent operator (**)
 *        - New keywords: elif, while, for, switch, class, import
 *
 *  Dependencies:
 *      - lexer.h
 *      - token.h
 *      - string.h, stdlib.h, ctype.h, stdio.h, stdarg.h
 *
 *  Important Changes:
 *      1. Unicode-friendly checks for identifiers (simple approach).
 *      2. Handling of triple quotes (docstrings).
 *      3. Interpolation detection inside strings.
 *      4. Additional numeric scanning logic (hex, bin, oct, underscores, e-notation).
 *      5. Regex literal scanning if a '/' is not a comment or operator.
 *      6. Handling bitwise and exponent operators.
 *      7. Memory safety and error handling remain as before.
 ************************************************************/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
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
Forward Declarations (internal use)
---------------------------------------------------------- */
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
    char        current;  /* current character under examination */
    char        peek;     /* next character (lookahead) */

    /* Error handling */
    LexerError  error;
    char        error_buffer[LEXER_MAX_ERROR_LENGTH];

    /* Configuration */
    LexerConfig config;

    /* Internal buffers for building strings, docstrings, etc. */
    char        string_buffer[LEXER_MAX_STRING_LENGTH];
};

/* ----------------------------------------------------------
Internal Function Prototypes
---------------------------------------------------------- */
static void advance(Lexer* lexer);
static int  is_at_end(Lexer* lexer);

static Token lexer_next_token_internal(Lexer* lexer);
static Token scan_identifier(Lexer* lexer, Token token);
static Token scan_number(Lexer* lexer, Token token);
static Token scan_string(Lexer* lexer, Token token);
static Token scan_docstring(Lexer* lexer, Token token);
static Token scan_regex_literal(Lexer* lexer, Token token);
static Token scan_operator_or_regex(Lexer* lexer);
static Token scan_token_dispatch(Lexer* lexer, Token token);

static void skip_whitespace(Lexer* lexer);
static void skip_comments(Lexer* lexer);
static Token handle_multi_char_operator(Lexer* lexer, TokenType type, const char* text);
static void safe_append_char(Lexer* lexer, char c, size_t* length);

static int  is_digit_10(char c);
static int  is_hex_digit(char c);
static int  is_alpha_ascii(char c);
static int  is_valid_identifier_start(char c);
static int  is_valid_identifier_char(char c);

static void clear_error(Lexer* lexer);

/* ----------------------------------------------------------
Error Handling (Definition)
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
    {"frame",     TOKEN_FRAME},
    {"in",        TOKEN_IN},
    {"var",       TOKEN_VAR},
    {"const",     TOKEN_CONST},
    {"func",      TOKEN_FUNC},
    {"return",    TOKEN_RETURN},
    {"if",        TOKEN_IF},
    {"else",      TOKEN_ELSE},
    {"loop",      TOKEN_LOOP},
    {"break",     TOKEN_BREAK},
    {"continue",  TOKEN_CONTINUE},
    {"on_error",  TOKEN_ON_ERROR},
    {"retry",     TOKEN_RETRY},
    {"reset",     TOKEN_RESET},
    {"null",      TOKEN_NULL},

    /* Additional keywords for test code or new language features */
    {"function",  TOKEN_FUNCTION},
    {"try",       TOKEN_TRY},
    {"catch",     TOKEN_CATCH},

    /* Newly added */
    {"elif",      TOKEN_ELIF},
    {"while",     TOKEN_WHILE},
    {"for",       TOKEN_FOR},
    {"switch",    TOKEN_SWITCH},
    {"class",     TOKEN_CLASS},
    {"import",    TOKEN_IMPORT},

    {NULL,        TOKEN_EOF}  /* Sentinel */
};

/* ----------------------------------------------------------
Lexer Lifecycle Management
---------------------------------------------------------- */
LexerConfig lexer_default_config(void)
{
    LexerConfig config;
    memset(&config, 0, sizeof(config));
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

    lexer->config = config;
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
    lexer->error.message[0] = '\0';
    lexer->error.location.line = lexer->line;
    lexer->error.location.column = lexer->column;
    lexer->error.location.file = lexer->config.file_name;
}

/* ----------------------------------------------------------
Character Management
---------------------------------------------------------- */
static void advance(Lexer* lexer)
{
    if (!is_at_end(lexer)) {
        lexer->position++;
        lexer->current = (lexer->position < lexer->length)
            ? lexer->source[lexer->position]
            : '\0';
        lexer->peek = (lexer->position + 1 < lexer->length)
            ? lexer->source[lexer->position + 1]
            : '\0';

        /* If we consumed a newline, increment line counter */
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
Public Token Retrieval
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
    char saved_current = lexer->current;
    char saved_peek = lexer->peek;

    LexerError saved_error = lexer->error;
    char saved_error_buffer[LEXER_MAX_ERROR_LENGTH];
    memcpy(saved_error_buffer, lexer->error_buffer, LEXER_MAX_ERROR_LENGTH);

    Token t = lexer_next_token_internal(lexer);

    /* Restore */
    lexer->position = saved_position;
    lexer->line = saved_line;
    lexer->column = saved_column;
    lexer->current = saved_current;
    lexer->peek = saved_peek;

    lexer->error = saved_error;
    memcpy(lexer->error_buffer, saved_error_buffer, LEXER_MAX_ERROR_LENGTH);

    return t;
}

/* ----------------------------------------------------------
Internal: Get Next Token
---------------------------------------------------------- */
static Token lexer_next_token_internal(Lexer* lexer)
{
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

    token.location.line = (uint32_t)lexer->line;
    token.location.column = (uint32_t)lexer->column;
    token.location.file = lexer->config.file_name;

    /* If we track newlines as tokens and we see a newline, produce TOKEN_NEWLINE */
    if (lexer->current == '\n' && lexer->config.track_line_endings) {
        advance(lexer);
        token.type = TOKEN_NEWLINE;
        strcpy_s(token.text, sizeof(token.text), "\\n");
        return token;
    }

    /* Dispatch based on the first character */
    return scan_token_dispatch(lexer, token);
}

/* ----------------------------------------------------------
Dispatcher: Decide what to scan (identifier, string, docstring, number, regex, operator)
---------------------------------------------------------- */
static Token scan_token_dispatch(Lexer* lexer, Token token)
{
    char c = lexer->current;

    /* Check for triple-quoted docstring */
    if (c == '"' &&
        lexer->peek == '"' &&
        (lexer->position + 2 < lexer->length) &&
        lexer->source[lexer->position + 2] == '"')
    {
        return scan_docstring(lexer, token);
    }

    /* Check normal string if it's just a single '"' */
    if (c == '"') {
        return scan_string(lexer, token);
    }

    /* Check for potential regex: '/' that isn't '//' or '/*' */
    if (c == '/' && lexer->peek != '/' && lexer->peek != '*') {
        return scan_regex_literal(lexer, token);
    }

    /* If valid identifier start, parse an identifier or keyword */
    if (is_valid_identifier_start(c)) {
        return scan_identifier(lexer, token);
    }

    /* If digit or '.' followed by digit => parse a number */
    if (is_digit_10(c) || (c == '.' && is_digit_10(lexer->peek))) {
        return scan_number(lexer, token);
    }

    /* Otherwise, handle operators/punctuation */
    return scan_operator_or_regex(lexer);
}

/* ----------------------------------------------------------
Scan: Docstring (Triple Quoted) => TOKEN_DOCSTRING
---------------------------------------------------------- */
static Token scan_docstring(Lexer* lexer, Token token)
{
    // Skip the initial """
    advance(lexer); // first quote
    advance(lexer); // second quote
    advance(lexer); // third quote

    token.type = TOKEN_DOCSTRING;

    size_t length = 0;
    memset(lexer->string_buffer, 0, LEXER_MAX_STRING_LENGTH);

    while (!is_at_end(lexer)) {
        /* Check if we see a closing """ */
        if (lexer->current == '"' &&
            lexer->peek == '"' &&
            (lexer->position + 2 < lexer->length) &&
            lexer->source[lexer->position + 2] == '"')
        {
            /* consume the three quotes */
            advance(lexer);
            advance(lexer);
            advance(lexer);
            break;
        }

        if (length >= LEXER_MAX_STRING_LENGTH - 1) {
            set_error(lexer, LEXER_ERROR_BUFFER_OVERFLOW,
                      "Docstring exceeds max length of %d",
                      LEXER_MAX_STRING_LENGTH - 1);
            token.type = TOKEN_ERROR;
            return token;
        }

        /* Append current character to string buffer */
        lexer->string_buffer[length++] = lexer->current;
        lexer->string_buffer[length] = '\0';
        advance(lexer);
    }

    /* If we reached the end without closing quotes => error? (Your choice) */

    /* Copy to token text and store it as string_value */
    strncpy_s(token.text, sizeof(token.text),
              lexer->string_buffer, sizeof(token.text) - 1);
    token.value.string_value.data = _strdup(lexer->string_buffer);
    token.value.string_value.length = length;

    return token;
}

/* ----------------------------------------------------------
Scan: String with Interpolation => TOKEN_STRING
---------------------------------------------------------- */
static Token scan_string(Lexer* lexer, Token token)
{
    token.type = TOKEN_STRING;
    /* Skip the opening quote */
    advance(lexer);

    size_t length = 0;
    memset(lexer->string_buffer, 0, LEXER_MAX_STRING_LENGTH);

    while (!is_at_end(lexer) && lexer->current != '"') {
        /* Look for interpolation: "${" */
        if (lexer->current == '$' && lexer->peek == '{') {
            /* If we have accumulated characters so far, return that as a string token first. */
            if (length > 0) {
                strncpy_s(token.text, sizeof(token.text),
                          lexer->string_buffer, sizeof(token.text) - 1);
                token.value.string_value.data = _strdup(lexer->string_buffer);
                token.value.string_value.length = length;
                return token;
            }
            /* Otherwise produce an empty string token or skip directly. */
            /* Let’s produce the interpolation token now. */
            Token interp;
            memset(&interp, 0, sizeof(interp));
            interp.type = TOKEN_INTERPOLATION_START;
            interp.location.line = lexer->line;
            interp.location.column = lexer->column;
            interp.location.file = lexer->config.file_name;
            strcpy_s(interp.text, sizeof(interp.text), "${");

            /* Advance past '$' and '{' */
            advance(lexer);
            advance(lexer);

            return interp;
        }

        /* Handle escape sequences: \n, \t, \", \\ */
        if (lexer->current == '\\') {
            advance(lexer);
            if (is_at_end(lexer)) {
                set_error(lexer, LEXER_ERROR_UNTERMINATED_STRING,
                          "Unterminated string literal (ends after backslash)");
                token.type = TOKEN_ERROR;
                return token;
            }
            switch (lexer->current) {
                case 'n': safe_append_char(lexer, '\n', &length); break;
                case 't': safe_append_char(lexer, '\t', &length); break;
                case '\\':safe_append_char(lexer, '\\', &length); break;
                case '"': safe_append_char(lexer, '"', &length); break;
                default:
                    set_error(lexer, LEXER_ERROR_INVALID_ESCAPE,
                              "Invalid escape sequence \\%c", lexer->current);
                    token.type = TOKEN_ERROR;
                    return token;
            }
        } else {
            safe_append_char(lexer, lexer->current, &length);
        }
        advance(lexer);
    }

    /* If we ended because of EOF => error */
    if (is_at_end(lexer)) {
        set_error(lexer, LEXER_ERROR_UNTERMINATED_STRING,
                  "Unterminated string literal before EOF");
        token.type = TOKEN_ERROR;
        return token;
    }

    /* Found the closing quote => consume it */
    advance(lexer);

    /* Now store the final string in token. */
    strncpy_s(token.text, sizeof(token.text),
              lexer->string_buffer, sizeof(token.text) - 1);
    token.value.string_value.data = _strdup(lexer->string_buffer);
    token.value.string_value.length = length;

    return token;
}

/* ----------------------------------------------------------
Scan: Regex Literal => TOKEN_REGEX
---------------------------------------------------------- */
static Token scan_regex_literal(Lexer* lexer, Token token)
{
    token.type = TOKEN_REGEX;

    /* Advance past the initial '/' */
    advance(lexer);

    size_t length = 0;
    memset(lexer->string_buffer, 0, LEXER_MAX_STRING_LENGTH);

    while (!is_at_end(lexer)) {
        if (lexer->current == '\\') {
            /* Escape sequence in a regex: store the slash plus the next char */
            safe_append_char(lexer, lexer->current, &length);
            advance(lexer);
            if (!is_at_end(lexer)) {
                safe_append_char(lexer, lexer->current, &length);
                advance(lexer);
            }
        } else if (lexer->current == '/') {
            /* End of regex */
            advance(lexer);
            break;
        } else {
            safe_append_char(lexer, lexer->current, &length);
            advance(lexer);
        }
    }

    /* Copy text into token */
    strncpy_s(token.text, sizeof(token.text),
              lexer->string_buffer, sizeof(token.text) - 1);

    /* Store as string_value */
    token.value.string_value.data = _strdup(lexer->string_buffer);
    token.value.string_value.length = length;

    return token;
}

/* ----------------------------------------------------------
Scan: Number with multiple formats
---------------------------------------------------------- */
static Token scan_number(Lexer* lexer, Token token)
{
    // This handles:
    //   - 0xNN for hex
    //   - 0bNN for binary
    //   - 0oNN for octal
    //   - decimal with underscores
    //   - scientific notation
    // We store everything in a small buffer, then parse.

    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    size_t length = 0;
    int is_float = 0;

    // If we see "0x", "0b", or "0o"
    if (lexer->current == '0') {
        if (lexer->peek == 'x' || lexer->peek == 'X') {
            // hex
            buffer[length++] = lexer->current; // '0'
            advance(lexer);
            buffer[length++] = lexer->current; // 'x'
            advance(lexer);

            while (!is_at_end(lexer) && is_hex_digit(lexer->current)) {
                if (length < sizeof(buffer) - 1) {
                    buffer[length++] = lexer->current;
                }
                advance(lexer);
            }
            buffer[length] = '\0';

            token.type = TOKEN_INTEGER;
            token.value.int_value = strtoll(buffer, NULL, 16);
            strcpy_s(token.text, sizeof(token.text), buffer);
            return token;
        }
        else if (lexer->peek == 'b' || lexer->peek == 'B') {
            // binary
            buffer[length++] = lexer->current; // '0'
            advance(lexer);
            buffer[length++] = lexer->current; // 'b'
            advance(lexer);

            while (!is_at_end(lexer) &&
                  (lexer->current == '0' || lexer->current == '1' || lexer->current == '_')) {
                if (lexer->current != '_') {
                    if (length < sizeof(buffer) - 1) {
                        buffer[length++] = lexer->current;
                    }
                }
                advance(lexer);
            }
            buffer[length] = '\0';

            token.type = TOKEN_INTEGER;
            token.value.int_value = strtoll(buffer, NULL, 2);
            strcpy_s(token.text, sizeof(token.text), buffer);
            return token;
        }
        else if (lexer->peek == 'o' || lexer->peek == 'O') {
            // octal
            buffer[length++] = lexer->current; // '0'
            advance(lexer);
            buffer[length++] = lexer->current; // 'o'
            advance(lexer);

            while (!is_at_end(lexer) &&
                   ((lexer->current >= '0' && lexer->current <= '7') || lexer->current == '_')) {
                if (lexer->current != '_') {
                    if (length < sizeof(buffer) - 1) {
                        buffer[length++] = lexer->current;
                    }
                }
                advance(lexer);
            }
            buffer[length] = '\0';

            token.type = TOKEN_INTEGER;
            token.value.int_value = strtoll(buffer, NULL, 8);
            strcpy_s(token.text, sizeof(token.text), buffer);
            return token;
        }
    }

    // Otherwise, parse decimal or float
    int seen_dot = 0;
    while (!is_at_end(lexer)) {
        if (lexer->current == '_') {
            // skip underscores
            advance(lexer);
            continue;
        }
        if (lexer->current == '.') {
            if (seen_dot) {
                // second dot => break
                break;
            }
            seen_dot = 1;
            is_float = 1;
            buffer[length++] = lexer->current;
            advance(lexer);
        }
        else if (is_digit_10(lexer->current)) {
            buffer[length++] = lexer->current;
            advance(lexer);
        }
        else {
            // possibly exponent (e/E)
            if ((lexer->current == 'e' || lexer->current == 'E') && is_digit_10(lexer->peek)) {
                is_float = 1;
                buffer[length++] = lexer->current;
                advance(lexer);

                // optional sign
                if (lexer->current == '+' || lexer->current == '-') {
                    buffer[length++] = lexer->current;
                    advance(lexer);
                }
                while (is_digit_10(lexer->current)) {
                    buffer[length++] = lexer->current;
                    advance(lexer);
                }
            } 
            else {
                // done
                break;
            }
        }

        if (length >= sizeof(buffer) - 1) {
            break;
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

/* ----------------------------------------------------------
Scan: Identifier or Keyword
---------------------------------------------------------- */
static Token scan_identifier(Lexer* lexer, Token token)
{
    char buffer[64];
    memset(buffer, 0, sizeof(buffer));
    size_t length = 0;

    while (!is_at_end(lexer) && is_valid_identifier_char(lexer->current)) {
        if (length < (sizeof(buffer) - 1)) {
            buffer[length++] = lexer->current;
        }
        advance(lexer);
    }
    buffer[length] = '\0';

    token.type = TOKEN_IDENTIFIER;
    strcpy_s(token.text, sizeof(token.text), buffer);

    /* Check if the identifier is a keyword. */
    for (int i = 0; KEYWORDS[i].keyword != NULL; i++) {
        if (strcmp(buffer, KEYWORDS[i].keyword) == 0) {
            token.type = KEYWORDS[i].type;
            break;
        }
    }

    /* Also check for 'true' / 'false' => bool literals */
    if (strcmp(buffer, "true") == 0) {
        token.type = TOKEN_BOOL_TRUE;
        token.value.bool_value = true;
    } else if (strcmp(buffer, "false") == 0) {
        token.type = TOKEN_BOOL_FALSE;
        token.value.bool_value = false;
    }

    return token;
}

/* ----------------------------------------------------------
Scan: Operator or punctuation
This handles multi-char and single-char operators.
Now includes bitwise ops (~, ^, single & and |)
Also exponent (**)
---------------------------------------------------------- */
static Token scan_operator_or_regex(Lexer* lexer)
{
    char c = lexer->current;
    char p = lexer->peek;

    Token token;
    memset(&token, 0, sizeof(token));
    token.location.line = lexer->line;
    token.location.column = lexer->column;
    token.location.file = lexer->config.file_name;

    // Multi-char operators first
    if (c == '*' && p == '*') {
        // exponent: **
        return handle_multi_char_operator(lexer, TOKEN_POW, "**");
    }
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

    // Single-char operators/characters
    switch (c) {
        case '~': token.type = TOKEN_BIT_NOT;       strcpy_s(token.text, sizeof(token.text), "~");  break;
        case '^': token.type = TOKEN_BIT_XOR;       strcpy_s(token.text, sizeof(token.text), "^");  break;
        case '&': token.type = TOKEN_BIT_AND;       strcpy_s(token.text, sizeof(token.text), "&");  break;
        case '|': token.type = TOKEN_BIT_OR;        strcpy_s(token.text, sizeof(token.text), "|");  break;
        case '[': token.type = TOKEN_LBRACKET;      strcpy_s(token.text, sizeof(token.text), "[");  break;
        case ']': token.type = TOKEN_RBRACKET;      strcpy_s(token.text, sizeof(token.text), "]");  break;

        case '+': token.type = TOKEN_PLUS;          strcpy_s(token.text, sizeof(token.text), "+");  break;
        case '-': token.type = TOKEN_MINUS;         strcpy_s(token.text, sizeof(token.text), "-");  break;
        case '*': token.type = TOKEN_STAR;          strcpy_s(token.text, sizeof(token.text), "*");  break;
        case '/': token.type = TOKEN_SLASH;         strcpy_s(token.text, sizeof(token.text), "/");  break;
        case '%': token.type = TOKEN_PERCENT;       strcpy_s(token.text, sizeof(token.text), "%");  break;
        case '=': token.type = TOKEN_ASSIGN;        strcpy_s(token.text, sizeof(token.text), "=");  break;
        case '!': token.type = TOKEN_NOT;           strcpy_s(token.text, sizeof(token.text), "!");  break;
        case '<': token.type = TOKEN_LT;            strcpy_s(token.text, sizeof(token.text), "<");  break;
        case '>': token.type = TOKEN_GT;            strcpy_s(token.text, sizeof(token.text), ">");  break;
        case '(': token.type = TOKEN_LPAREN;        strcpy_s(token.text, sizeof(token.text), "(");  break;
        case ')': token.type = TOKEN_RPAREN;        strcpy_s(token.text, sizeof(token.text), ")");  break;
        case '{': token.type = TOKEN_LBRACE;        strcpy_s(token.text, sizeof(token.text), "{");  break;
        case '}': token.type = TOKEN_RBRACE;        strcpy_s(token.text, sizeof(token.text), "}");  break;
        case ';': token.type = TOKEN_SEMICOLON;     strcpy_s(token.text, sizeof(token.text), ";");  break;
        case ':': token.type = TOKEN_COLON;         strcpy_s(token.text, sizeof(token.text), ":");  break;
        case ',': token.type = TOKEN_COMMA;         strcpy_s(token.text, sizeof(token.text), ",");  break;
        case '.': token.type = TOKEN_DOT;           strcpy_s(token.text, sizeof(token.text), ".");  break;
        default:
            token.type = TOKEN_ERROR;
            token.text[0] = c;
            token.text[1] = '\0';
            set_error(lexer, LEXER_ERROR_INVALID_CHAR,
                      "Invalid character '%c' at line %zu, column %zu",
                      c, (size_t)lexer->line, (size_t)lexer->column);
            break;
    }

    advance(lexer);
    return token;
}

/* ----------------------------------------------------------
Helper: Handle multi-char operator
---------------------------------------------------------- */
static Token handle_multi_char_operator(Lexer* lexer, TokenType type, const char* text)
{
    Token token;
    memset(&token, 0, sizeof(Token));
    token.type = type;
    token.location.line = lexer->line;
    token.location.column = lexer->column;
    token.location.file = lexer->config.file_name;
    strcpy_s(token.text, sizeof(token.text), text);

    size_t len = strlen(text);
    lexer->position += len;

    lexer->current = (lexer->position < lexer->length)
        ? lexer->source[lexer->position]
        : '\0';
    lexer->peek = (lexer->position + 1 < lexer->length)
        ? lexer->source[lexer->position + 1]
        : '\0';

    lexer->column += (uint32_t)len;

    return token;
}

/* ----------------------------------------------------------
Helper: safe_append_char
Appends one character to lexer->string_buffer if there's space.
---------------------------------------------------------- */
static void safe_append_char(Lexer* lexer, char c, size_t* length)
{
    if (*length < LEXER_MAX_STRING_LENGTH - 1) {
        lexer->string_buffer[*length] = c;
        (*length)++;
        lexer->string_buffer[*length] = '\0';
    } else {
        set_error(lexer, LEXER_ERROR_BUFFER_OVERFLOW,
                  "String literal buffer overflow at line %zu, column %zu.",
                  (size_t)lexer->line, (size_t)lexer->column);
    }
}

/* ----------------------------------------------------------
Skipping Whitespace & Comments
---------------------------------------------------------- */
static void skip_whitespace(Lexer* lexer)
{
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
        // Single-line comment: "//"
        if (lexer->current == '/' && lexer->peek == '/') {
            if (lexer->config.include_comments) {
                /* If you want to produce TOKEN_COMMENT, you'd do that here. */
                return; 
            }
            /* Otherwise, skip until we hit newline or EOF. */
            while (!is_at_end(lexer) && lexer->current != '\n') {
                advance(lexer);
            }
        }
        // Multi-line comment: "/*"
        else if (lexer->current == '/' && lexer->peek == '*') {
            if (lexer->config.include_comments) {
                /* Optionally produce a TOKEN_COMMENT or skip. */
                return;
            }
            advance(lexer);
            advance(lexer);
            int closed = 0;
            while (!is_at_end(lexer)) {
                if (lexer->current == '*' && lexer->peek == '/') {
                    advance(lexer);
                    advance(lexer);
                    closed = 1;
                    break;
                }
                advance(lexer);
            }
            if (!closed) {
                set_error(lexer, LEXER_ERROR_UNTERMINATED_COMMENT,
                          "Unterminated multi-line comment at line %zu, col %zu.",
                          (size_t)lexer->line, (size_t)lexer->column);
            }
        }
        else {
            // Not a comment => stop
            return;
        }
        skip_whitespace(lexer);
    }
}

/* ----------------------------------------------------------
Error Handling
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
Token Cleanup
---------------------------------------------------------- */
void lexer_token_cleanup(Token* token)
{
    /* Free any allocated string memory from string_value.data */
    if ((token->type == TOKEN_STRING ||
         token->type == TOKEN_DOCSTRING ||
         token->type == TOKEN_REGEX) &&
        token->value.string_value.data)
    {
        free((void*)token->value.string_value.data);
        token->value.string_value.data = NULL;
    }
}

/* ----------------------------------------------------------
Helper Functions for Identifiers / Unicode
---------------------------------------------------------- */
static int is_digit_10(char c)
{
    return (c >= '0' && c <= '9');
}

static int is_hex_digit(char c)
{
    return ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f'));
}

static int is_alpha_ascii(char c)
{
    return ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z'));
}

/**
 * Simple approach to allow ASCII letters or underscore or any high-bit char >=128
 * for a Unicode-friendly start character. Real-world usage might decode UTF-8 
 * properly to check if it's a letter category, but this is a minimal approach.
 */
static int is_valid_identifier_start(char c)
{
    if (is_alpha_ascii(c) || c == '_' || ((unsigned char)c >= 128)) {
        return TRUE;
    }
    return FALSE;
}

static int is_valid_identifier_char(char c)
{
    if (is_alpha_ascii(c) || is_digit_10(c) || c == '_' || ((unsigned char)c >= 128)) {
        return TRUE;
    }
    return FALSE;
}

/* ----------------------------------------------------------
Get the current lexer error
---------------------------------------------------------- */
LexerError lexer_get_error(const Lexer* lexer)
{
    return lexer->error;
}
