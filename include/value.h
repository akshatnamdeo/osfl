#ifndef VALUE_H
#define VALUE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * Unified value type for OSFL.
 */
typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_STRING,
    VAL_LIST,
    VAL_FILE,
    VAL_OBJ,
    VAL_NULL
} ValueType;

typedef struct Value {
    ValueType type;
    union {
        int64_t int_val;
        double float_val;
        bool bool_val;
        char* str_val;
        struct {
            struct Value* data;  /* dynamic array of Value */
            size_t length;
            size_t capacity;
        } list_val;
        struct {
            void* native_file;   /* e.g. FILE* */
        } file_val;
        void* obj_ref;
    } as;
    int refcount;
} Value;

#define VALUE_NULL ((Value){ .type = VAL_NULL })

#endif /* VALUE_H */
