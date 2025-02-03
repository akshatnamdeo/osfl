#ifndef OSFL_LEXER_H
#define OSFL_LEXER_H

#include "token.h"
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Maximum length for error messages
 */
#define LEXER_MAX_ERROR_LENGTH 128

/**
 * @brief Maximum length for string literals
 */
#undef LEXER_MAX_STRING_LENGTH
#define LEXER_MAX_STRING_LENGTH 64

/**
 * @brief Error types that can occur during lexing
 */
typedef enum {
    LEXER_ERROR_NONE,
    LEXER_ERROR_INVALID_CHAR,
    LEXER_ERROR_INVALID_STRING,
    LEXER_ERROR_INVALID_NUMBER,
    LEXER_ERROR_INVALID_IDENTIFIER,
    LEXER_ERROR_UNTERMINATED_COMMENT,
    LEXER_ERROR_UNTERMINATED_STRING,
    LEXER_ERROR_STRING_TOO_LONG,
    LEXER_ERROR_INVALID_ESCAPE,
    LEXER_ERROR_BUFFER_OVERFLOW,
    LEXER_ERROR_MEMORY,
    LEXER_ERROR_FILE_IO
} LexerErrorType;

/**
 * @brief Error information structure
 */
typedef struct {
    LexerErrorType type;
    char message[LEXER_MAX_ERROR_LENGTH];
    SourceLocation location;
} LexerError;

/**
 * @brief Configuration options for the lexer
 */
typedef struct {
    bool skip_whitespace;       // Whether to skip whitespace tokens
    bool include_comments;      // Whether to include comment tokens
    bool track_line_endings;    // Whether to emit newline tokens
    size_t tab_width;           // Number of spaces per tab
    const char* file_name;      // Name of the source file
} LexerConfig;

/**
 * @brief Forward declaration of the Lexer structure
 */
typedef struct Lexer Lexer;

/**
 * @brief Creates a default lexer configuration
 */
LexerConfig lexer_default_config(void);

/**
 * @brief Creates a new lexer instance
 */
Lexer* lexer_create(const char* source, size_t length, LexerConfig config);

/**
 * @brief Resets the lexer to read from a new source
 */
void lexer_reset(Lexer* lexer, const char* new_source);

/**
 * @brief Destroys the lexer instance
 */
void lexer_destroy(Lexer* lexer);

/**
 * @brief Returns the next token from the lexer
 */
Token lexer_next_token(Lexer* lexer);

/**
 * @brief Peeks at the next token without consuming it
 */
Token lexer_peek_token(Lexer* lexer);

/**
 * @brief Retrieves the current lexer error state
 */
LexerError lexer_get_error(const Lexer* lexer);

/**
 * @brief Cleans up any allocated memory in a token (e.g. strings)
 */
void lexer_token_cleanup(Token* token);

#endif /* OSFL_LEXER_H */
