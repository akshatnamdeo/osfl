#ifndef RUNTIME_H
#define RUNTIME_H

#include <stddef.h>
#include <stdbool.h>
#include "../../include/value.h"    // Unified Value type
#include "../vm/vm.h"    // For VM types and native function signatures

/**
 * For the runtime, we define OSFL_Value to be our unified Value.
 */
typedef Value OSFL_Value;

#define VALUE_NULL ((OSFL_Value){ .type = VAL_NULL })

/* Built-in OSFL runtime function signatures.
    Each function receives arg_count and a pointer to an array of OSFL_Value and returns an OSFL_Value.
*/

/* String functions */
OSFL_Value osfl_split     (int arg_count, OSFL_Value* args);
OSFL_Value osfl_join      (int arg_count, OSFL_Value* args);
OSFL_Value osfl_substring (int arg_count, OSFL_Value* args);
OSFL_Value osfl_replace   (int arg_count, OSFL_Value* args);
OSFL_Value osfl_to_upper  (int arg_count, OSFL_Value* args);
OSFL_Value osfl_to_lower  (int arg_count, OSFL_Value* args);

/* List/Array functions */
OSFL_Value osfl_len       (int arg_count, OSFL_Value* args);
OSFL_Value osfl_append    (int arg_count, OSFL_Value* args);
OSFL_Value osfl_pop       (int arg_count, OSFL_Value* args);
OSFL_Value osfl_insert    (int arg_count, OSFL_Value* args);
OSFL_Value osfl_remove    (int arg_count, OSFL_Value* args);

/* Math functions */
OSFL_Value osfl_sqrt      (int arg_count, OSFL_Value* args);
OSFL_Value osfl_pow       (int arg_count, OSFL_Value* args);
OSFL_Value osfl_sin       (int arg_count, OSFL_Value* args);
OSFL_Value osfl_cos       (int arg_count, OSFL_Value* args);
OSFL_Value osfl_tan       (int arg_count, OSFL_Value* args);
OSFL_Value osfl_log       (int arg_count, OSFL_Value* args);
OSFL_Value osfl_abs       (int arg_count, OSFL_Value* args);

/* Conversion functions */
OSFL_Value osfl_int       (int arg_count, OSFL_Value* args);
OSFL_Value osfl_float     (int arg_count, OSFL_Value* args);
OSFL_Value osfl_str       (int arg_count, OSFL_Value* args);
OSFL_Value osfl_bool      (int arg_count, OSFL_Value* args);

/* File I/O functions */
OSFL_Value osfl_open      (int arg_count, OSFL_Value* args);
OSFL_Value osfl_read      (int arg_count, OSFL_Value* args);
OSFL_Value osfl_write     (int arg_count, OSFL_Value* args);
OSFL_Value osfl_close     (int arg_count, OSFL_Value* args);

/* System/OS functions */
OSFL_Value osfl_exit      (int arg_count, OSFL_Value* args);
OSFL_Value osfl_time      (int arg_count, OSFL_Value* args);

/* Misc. */
OSFL_Value osfl_type      (int arg_count, OSFL_Value* args);
OSFL_Value osfl_range     (int arg_count, OSFL_Value* args);
OSFL_Value osfl_enumerate (int arg_count, OSFL_Value* args);

/* NEW: Print Function */
OSFL_Value osfl_print     (int arg_count, OSFL_Value* args);

#endif /* RUNTIME_H */
