#ifndef FRAME_H
#define FRAME_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * A flexible Value type to allow storing multiple data types in locals.
 * For simplicity, we use a union plus a type tag. Expand as needed
 * (e.g., to store strings, references, objects, etc.).
 */
typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_NONE
    /* You can add VAL_STRING, VAL_OBJECT, etc. */
} ValueType;

typedef struct {
    ValueType type;
    union {
        int64_t int_val;
        double float_val;
        bool bool_val;
        /* Add more as needed. e.g., pointer to string or object. */
    } as;
} Value;

/**
 * The Frame structure represents a call frame in the VM.
 * - 'locals': array of Values
 * - 'local_count': number of local variables
 * - 'parent': parent frame for dynamic scoping or nested function contexts
 */
typedef struct Frame {
    Value* locals;           /* Array of local variables (Value type) */
    size_t local_count;      /* Number of local variables */
    struct Frame* parent;    /* Parent frame for dynamic scoping or closures */

    /* Optionally store additional data, like instruction pointer,
       function metadata, closure references, etc. */
} Frame;

/**
 * Create a new frame with a given number of local variables.
 * 'parent' is an optional parent frame. Return NULL on failure.
 */
Frame* frame_create(size_t local_count, Frame* parent);

/**
 * Destroy a frame and free its memory.
 * Does NOT recurse into parent; you typically manage that in the VM’s call stack.
 */
void frame_destroy(Frame* frame);

#endif // FRAME_H
