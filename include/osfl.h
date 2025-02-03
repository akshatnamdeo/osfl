#ifndef OSFL_H
#define OSFL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>    // Added for bool

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------
    Version Information
   ---------------------------------------------------------- */
#define OSFL_VERSION_MAJOR 0
#define OSFL_VERSION_MINOR 1
#define OSFL_VERSION_PATCH 0
#define OSFL_VERSION_STRING "0.1.0"

/* ----------------------------------------------------------
    Configuration Constants
   ---------------------------------------------------------- */
#define OSFL_MAX_STRING_LENGTH 1024
#define OSFL_MAX_ERROR_LENGTH  128
#define OSFL_DEFAULT_TAB_WIDTH 4
#define OSFL_MAX_IDENTIFIER_LENGTH 64

/* ----------------------------------------------------------
    Status Codes and Error Handling
   ---------------------------------------------------------- */
typedef enum OSFLStatus {
    OSFL_SUCCESS = 0,
    OSFL_ERROR_MEMORY_ALLOCATION,
    OSFL_ERROR_INVALID_INPUT,
    OSFL_ERROR_FILE_IO,
    OSFL_ERROR_SYNTAX,
    OSFL_ERROR_LEXER,
    OSFL_ERROR_PARSER,
    OSFL_ERROR_COMPILER,
    OSFL_ERROR_VM,
    OSFL_ERROR_RUNTIME
} OSFLStatus;

/* Error information structure */
typedef struct {
    OSFLStatus code;
    char message[OSFL_MAX_ERROR_LENGTH];
    const char* file;
    size_t line;
    size_t column;
} OSFLError;

/* ----------------------------------------------------------
    Configuration Structures
   ---------------------------------------------------------- */
typedef struct {
    size_t tab_width;           /* Width of tab characters */
    bool include_comments;      /* Whether to include comments in tokens */
    const char* input_file;     /* Input source file */
    const char* output_file;    /* Output file (if any) */
    bool debug_mode;            /* Enable debug output */
    bool optimize;              /* Enable optimizations */
} OSFLConfig;

/* ----------------------------------------------------------
    Core API Functions
   ---------------------------------------------------------- */

/**
 * Initialize the OSFL system
 * @param config Configuration options
 * @return Status code
 */
OSFLStatus osfl_init(const OSFLConfig* config);

/**
 * Clean up and shut down OSFL
 */
void osfl_cleanup(void);

/**
 * Get version information
 * @return Version string
 */
const char* osfl_version(void);

/**
 * Get the last error that occurred
 * @return Error information
 */
const OSFLError* osfl_get_last_error(void);

/**
 * Clear the last error
 */
void osfl_clear_error(void);

/**
 * Run OSFL on a source file
 * @param filename Source file to process
 * @return Status code
 */
OSFLStatus osfl_run_file(const char* filename);

/**
 * Run OSFL on a string of source code
 * @param source Source code string
 * @param length Length of source string
 * @return Status code
 */
OSFLStatus osfl_run_string(const char* source, size_t length);

/**
 * Set configuration options
 * @param config New configuration
 * @return Status code
 */
OSFLStatus osfl_configure(const OSFLConfig* config);

/**
 * Get current configuration
 * @return Current configuration
 */
OSFLConfig osfl_get_config(void);

/**
 * Create default configuration
 * @return Default configuration
 */
OSFLConfig osfl_default_config(void);

#ifdef __cplusplus
}
#endif

#endif /* OSFL_H */
