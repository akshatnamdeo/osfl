#ifndef OSFL_TOKEN_H
#define OSFL_TOKEN_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Represents all possible token types in the OSFL language
 */
typedef enum {
    // Keywords
    TOKEN_FRAME,      // frame
    TOKEN_IN,         // in
    TOKEN_VAR,        // var
    TOKEN_CONST,      // const
    TOKEN_FUNC,       // func
    TOKEN_RETURN,     // return
    TOKEN_IF,         // if
    TOKEN_ELSE,       // else
    TOKEN_LOOP,       // loop
    TOKEN_BREAK,      // break
    TOKEN_CONTINUE,   // continue
    TOKEN_ON_ERROR,   // on_error
    TOKEN_RETRY,      // retry
    TOKEN_RESET,      // reset
    TOKEN_NULL,       // null
    
    // Data types
    TOKEN_TYPE_INT,    // int
    TOKEN_TYPE_FLOAT,  // float
    TOKEN_TYPE_BOOL,   // bool
    TOKEN_TYPE_STRING, // string
    TOKEN_TYPE_FRAME,  // frame
    TOKEN_TYPE_REF,    // ref
    
    // Literals
    TOKEN_INTEGER,     // 123
    TOKEN_FLOAT,       // 123.456
    TOKEN_STRING,      // "hello"
    TOKEN_BOOL_TRUE,   // true
    TOKEN_BOOL_FALSE,  // false
    
    // Identifiers
    TOKEN_IDENTIFIER,  // variable_name
    
    // Arithmetic operators
    TOKEN_PLUS,        // +
    TOKEN_MINUS,       // -
    TOKEN_STAR,        // *
    TOKEN_SLASH,       // /
    TOKEN_PERCENT,     // %
    TOKEN_INCREMENT,   // ++
    TOKEN_DECREMENT,   // --
    
    // Logical operators
    TOKEN_AND,         // &&
    TOKEN_OR,          // ||
    TOKEN_NOT,         // !
    
    // Comparison operators
    TOKEN_EQ,          // ==
    TOKEN_NEQ,         // !=
    TOKEN_LT,          // <
    TOKEN_GT,          // >
    TOKEN_LTE,         // <=
    TOKEN_GTE,         // >=
    
    // Assignment operators
    TOKEN_ASSIGN,      // =
    TOKEN_PLUS_ASSIGN, // +=
    TOKEN_MINUS_ASSIGN,// -=
    TOKEN_STAR_ASSIGN, // *=
    TOKEN_SLASH_ASSIGN,// /=
    TOKEN_MOD_ASSIGN,  // %=
    
    // Frame operators
    TOKEN_ARROW,       // ->
    TOKEN_DOUBLE_ARROW,// =>
    TOKEN_DOUBLE_COLON,// ::
    
    // Delimiters
    TOKEN_LPAREN,      // (
    TOKEN_RPAREN,      // )
    TOKEN_LBRACE,      // {
    TOKEN_RBRACE,      // }
    TOKEN_COMMA,       // ,
    TOKEN_DOT,         // .
    TOKEN_SEMICOLON,   // ;
    TOKEN_COLON,       // :
    
    // Special tokens
    TOKEN_NEWLINE,     // Newline
    TOKEN_EOF,         // End of file
    TOKEN_ERROR        // Error token
} TokenType;

/**
 * @brief Source location information
 */
typedef struct {
    uint32_t line;     // Line number (1-based)
    uint32_t column;   // Column number (1-based)
    const char* file;  // Source file name
} SourceLocation;

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
    TokenType type;
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
Token token_create(TokenType type, SourceLocation location, 
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
 */
Token token_create_float(double value, SourceLocation location,
                        const char* lexeme, size_t length);

/**
 * @brief Creates a token for string literals
 */
Token token_create_string(const char* value, size_t str_length,
                         SourceLocation location, const char* lexeme,
                         size_t lex_length);

/**
 * @brief Creates a token for boolean literals
 */
Token token_create_bool(bool value, SourceLocation location,
                       const char* lexeme, size_t length);

/**
 * @brief Returns a string representation of a token type
 * 
 * @param type The token type
 * @return const char* String representation
 */
const char* token_type_str(TokenType type);

/**
 * @brief Checks if a token type is a keyword
 */
bool token_is_keyword(TokenType type);

/**
 * @brief Checks if a token type is an operator
 */
bool token_is_operator(TokenType type);

/**
 * @brief Checks if a token type is a literal
 */
bool token_is_literal(TokenType type);

#endif // OSFL_TOKEN_H