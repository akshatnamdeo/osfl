// src/vm/frame.h
#ifndef FRAME_H
#define FRAME_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../include/value.h"

/*
 * A Frame structure for function calls in the VM.
 * Holds a local array of Values.
 */
typedef struct Frame {
    Value* locals;
    size_t local_count;
    struct Frame* parent;  /* link to parent frame if needed */
} Frame;

/* Create/destroy frames */
Frame* frame_create(size_t local_count, Frame* parent);
void frame_destroy(Frame* frame);

#endif /* FRAME_H */