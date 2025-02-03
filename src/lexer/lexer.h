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
 * @brief Maximum line length in source files
 */
#define LEXER_MAX_LINE_LENGTH 1024

/**
 * @brief Size of the lexer's internal buffer
 */
#define LEXER_BUFFER_SIZE 4096

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
    LEXER_ERROR_LINE_TOO_LONG,
    LEXER_ERROR_INVALID_ESCAPE,
    LEXER_ERROR_FILE_IO,
    LEXER_ERROR_BUFFER_OVERFLOW,
    LEXER_ERROR_INVALID_ESCAPE_SEQUENCE,
    LEXER_ERROR_MEMORY,
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
    size_t tab_width;          // Number of spaces per tab
    const char* file_name;      // Name of the source file
} LexerConfig;

/**
 * @brief Lexer state structure
 */
typedef struct Lexer Lexer;

/**
 * @brief Creates a new lexer instance
 * 
 * @param source Pointer to the source code string
 * @param length Length of the source code
 * @param config Lexer configuration
 * @return Lexer* Pointer to the created lexer, or NULL on error
 */
Lexer* lexer_create(const char* source, size_t length, LexerConfig config);

LexerError lexer_get_error(const Lexer* lexer);

/**
 * @brief Destroys a lexer instance and frees all associated memory
 * 
 * @param lexer The lexer to destroy
 */
void lexer_destroy(Lexer* lexer);

void lexer_token_cleanup(Token* token);

/**
 * @brief Advances the lexer and returns the next token
 * 
 * @param lexer The lexer instance
 * @return Token The next token
 */
Token lexer_next_token(Lexer* lexer);

/**
 * @brief Peeks at the next token without advancing the lexer
 * 
 * @param lexer The lexer instance
 * @return Token The next token
 */
Token lexer_peek_token(Lexer* lexer);

/**
 * @brief Gets the current error state of the lexer
 * 
 * @param lexer The lexer instance
 * @return LexerError The current error information
 */
LexerError lexer_get_error(const Lexer* lexer);

/**
 * @brief Gets the current source location of the lexer
 * 
 * @param lexer The lexer instance
 * @return SourceLocation The current location
 */
SourceLocation lexer_get_location(const Lexer* lexer);

/**
 * @brief Creates a default lexer configuration
 * 
 * @return LexerConfig The default configuration
 */
LexerConfig lexer_default_config(void);

/**
 * @brief Returns a string description of a lexer error type
 * 
 * @param type The error type
 * @return const char* Error description
 */
const char* lexer_error_str(LexerErrorType type);

/**
 * @brief Resets the lexer to the beginning of the source
 * 
 * @param lexer The lexer instance
 */
void lexer_reset(Lexer* lexer, const char* new_source);

/**
 * @brief Returns the current line of source code
 * 
 * @param lexer The lexer instance
 * @param length Pointer to store the line length
 * @return const char* The current line
 */
const char* lexer_current_line(const Lexer* lexer, size_t* length);

#endif // OSFL_LEXER_H