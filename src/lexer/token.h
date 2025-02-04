#ifndef OSFL_TOKEN_H
#define OSFL_TOKEN_H

#include <stdint.h>
#include <stdbool.h>
#include "../../include/source_location.h"

/**
 * @brief Represents all possible token types in the OSFL language
 */
typedef enum {
    // Keywords
    TOKEN_FRAME,              // frame
    TOKEN_IN,                 // in
    TOKEN_VAR,                // var
    TOKEN_CONST,              // const
    TOKEN_FUNC,               // func
    TOKEN_RETURN,             // return
    TOKEN_IF,                 // if
    TOKEN_ELSE,               // else
    TOKEN_LOOP,               // loop
    TOKEN_BREAK,              // break
    TOKEN_CONTINUE,           // continue
    TOKEN_ON_ERROR,           // on_error
    TOKEN_RETRY,              // retry
    TOKEN_RESET,              // reset
    TOKEN_NULL,               // null

    // Additional keywords (for parser expansions)
    TOKEN_FUNCTION,           // "function"
    TOKEN_TRY,                // try
    TOKEN_CATCH,              // catch
    TOKEN_WHILE,              // while
    TOKEN_FOR,                // for
    TOKEN_ELIF,               // elif (Python-style else-if)
    TOKEN_SWITCH,             // switch
    TOKEN_CLASS,              // class
    TOKEN_IMPORT,             // import

    // Data types
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_FRAME,
    TOKEN_TYPE_REF,

    // Literals
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_BOOL_TRUE,
    TOKEN_BOOL_FALSE,

    // Identifiers
    TOKEN_IDENTIFIER,

    // Arithmetic operators
    TOKEN_PLUS,               // +
    TOKEN_MINUS,              // -
    TOKEN_STAR,               // *
    TOKEN_SLASH,              // /
    TOKEN_PERCENT,            // %
    TOKEN_INCREMENT,          // ++
    TOKEN_DECREMENT,          // --
    TOKEN_POW,                // ** (exponent)

    // Bitwise operators
    TOKEN_BIT_NOT,            // ~
    TOKEN_BIT_AND,            // &
    TOKEN_BIT_OR,             // |
    TOKEN_BIT_XOR,            // ^

    // Logical operators
    TOKEN_AND,                // &&
    TOKEN_OR,                 // ||
    TOKEN_NOT,                // !

    // Comparison operators
    TOKEN_EQ,                 // ==
    TOKEN_NEQ,                // !=
    TOKEN_LT,                 // <
    TOKEN_GT,                 // >
    TOKEN_LTE,                // <=
    TOKEN_GTE,                // >=

    // Assignment operators
    TOKEN_ASSIGN,             // =
    TOKEN_PLUS_ASSIGN,        // +=
    TOKEN_MINUS_ASSIGN,       // -=
    TOKEN_STAR_ASSIGN,        // *=
    TOKEN_SLASH_ASSIGN,       // /=
    TOKEN_MOD_ASSIGN,         // %=

    // Frame operators
    TOKEN_ARROW,              // ->
    TOKEN_DOUBLE_ARROW,       // =>
    TOKEN_DOUBLE_COLON,       // ::

    // Delimiters
    TOKEN_LPAREN,             // (
    TOKEN_RPAREN,             // )
    TOKEN_LBRACE,             // {
    TOKEN_RBRACE,             // }
    TOKEN_LBRACKET,           // [
    TOKEN_RBRACKET,           // ]
    TOKEN_COMMA,              // ,
    TOKEN_DOT,                // .
    TOKEN_SEMICOLON,          // ;
    TOKEN_COLON,              // :

    // New/Enhanced tokens for strings and regex
    TOKEN_DOCSTRING,             // """multi-line string"""
    TOKEN_INTERPOLATION_START,   // "${"  (start of interpolation)
    TOKEN_INTERPOLATION_END,     // "}"   (end of interpolation)
    TOKEN_REGEX,                 // /.../ (regex literal)

    // Special tokens
    TOKEN_WHITESPACE,            // Whitespace characters (space, tab)
    TOKEN_NEWLINE,             // Newline
    TOKEN_EOF,                 // End of file
    TOKEN_ERROR                // Error token
} OSFLTokenType;

/**
 * @brief Token value union for different literal types
 */
typedef union {
    int64_t int_value;
    double float_value;
    struct {
        const char* data;
        size_t length;
    } string_value;
    bool bool_value;
} TokenValue;

/**
 * @brief Represents a single token in the source code
 */
typedef struct {
    OSFLTokenType type;
    TokenValue value;
    SourceLocation location;
    char text[64];  // Fixed-size buffer for token text
} Token;

/**
 * @brief Creates a new token with the given type and location
 *
 * @param type The token type
 * @param location Source location information
 * @param lexeme Pointer to the start of the token text
 * @param length Length of the token text
 * @return Token The created token
 */
Token token_create(OSFLTokenType type, SourceLocation location,
                   const char* lexeme, size_t length);

/**
 * @brief Creates a token for integer literals
 *
 * @param value The integer value
 * @param location Source location
 * @param lexeme Original text
 * @param length Length of the text
 * @return Token The created token
 */
Token token_create_int(int64_t value, SourceLocation location,
                       const char* lexeme, size_t length);


/**
 * @brief Creates a token for float literals
 *
 * @param value The float value
 * @param location Source location
 * @param lexeme Original text
 * @param length Length of the text
 * @return Token The created token
 */
Token token_create_float(double value, SourceLocation location,
                         const char* lexeme, size_t length);

/**
 * @brief Creates a token for string literals
 *
 * @param value The string value
 * @param str_length Length of the string value
 * @param location Source location
 * @param lexeme Original token text
 * @param lex_length Length of the lexeme text
 * @return Token The created token
 */
Token token_create_string(const char* value, size_t str_length,
                          SourceLocation location, const char* lexeme,
                          size_t lex_length);

/**
 * @brief Creates a token for boolean literals
 *
 * @param value The boolean value
 * @param location Source location
 * @param lexeme Original token text
 * @param length Length of the token text
 * @return Token The created token
 */
Token token_create_bool(bool value, SourceLocation location,
                        const char* lexeme, size_t length);

/**
 * @brief Returns a string representation of a token type
 *
 * @param type The token type
 * @return const char* String representation
 */
const char* token_type_str(OSFLTokenType type);

/**
 * @brief Checks if a token type is a keyword
 *
 * @param type The token type
 * @return true if it is a keyword, false otherwise
 */
bool token_is_keyword(OSFLTokenType type);

/**
 * @brief Checks if a token type is an operator
 *
 * @param type The token type
 * @return true if it is an operator, false otherwise
 */
bool token_is_operator(OSFLTokenType type);

/**
 * @brief Checks if a token type is a literal
 *
 * @param type The token type
 * @return true if it is a literal, false otherwise
 */
bool token_is_literal(OSFLTokenType type);

#endif // OSFL_TOKEN_H
