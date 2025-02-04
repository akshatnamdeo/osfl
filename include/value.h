// include/value.h
#ifndef VALUE_H
#define VALUE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Unified value type for OSFL.
 */
typedef enum {
    VAL_NULL,
    VAL_INT,
    VAL_FLOAT,
    VAL_BOOL,
    VAL_STRING,
    VAL_LIST,
    VAL_FILE,
    VAL_OBJ
} ValueType;

typedef struct Value {
    ValueType type;
    int refcount;
    union {
        int64_t int_val;
        double float_val;
        bool bool_val;
        char* str_val;
        void* obj_ref;
        struct {
            void* native_file;
        } file_val;
        struct {
            struct Value* data;
            size_t length;
            size_t capacity;
        } list_val;
    } as;
} Value;

// Create aliases for all uses
typedef Value VMValue;
typedef Value OSFL_Value;
typedef Value FrameValue;
typedef ValueType VMValueType;
typedef ValueType OSFL_ValueType;
typedef ValueType FrameValueType;

#define VALUE_NULL ((Value){ .type = VAL_NULL })

#ifdef __cplusplus
}
#endif

#endif /* VALUE_H */