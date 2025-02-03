#ifndef FRAME_H
#define FRAME_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * A minimal FrameValueType for local variables in frames.
 */
typedef enum {
    FRAME_VAL_INT,
    FRAME_VAL_FLOAT,
    FRAME_VAL_BOOL,
    FRAME_VAL_NONE,
    FRAME_VAL_OBJ,
    FRAME_VAL_STRING
} FrameValueType;

/*
 * A Value in local variables (in frames)
 */
typedef struct {
    FrameValueType type;
    union {
        int64_t int_val;
        double  float_val;
        bool    bool_val;
        void*   obj_ref;  /* If you want to store objects in frames */
    } as;
} FrameValue;

/*
 * A Frame structure for function calls in the VM.
 * Holds a local array of Values.
 */
typedef struct Frame {
    FrameValue* locals;
    size_t local_count;

    struct Frame* parent;  /* link to parent frame if needed */
} Frame;

/* Create/destroy frames */
Frame* frame_create(size_t local_count, Frame* parent);
void  frame_destroy(Frame* frame);

#endif /* FRAME_H */
