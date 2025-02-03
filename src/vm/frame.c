#include "frame.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Creates a new frame. Allocates 'local_count' Value slots and initializes them to VAL_NONE.
 */
Frame* frame_create(size_t local_count, Frame* parent) {
    Frame* frame = (Frame*)malloc(sizeof(Frame));
    if (!frame) {
        fprintf(stderr, "Failed to allocate Frame.\n");
        return NULL;
    }

    frame->locals = (Value*)calloc(local_count, sizeof(Value));
    if (!frame->locals) {
        fprintf(stderr, "Failed to allocate Frame locals.\n");
        free(frame);
        return NULL;
    }

    frame->local_count = local_count;
    frame->parent = parent;

    /* Initialize all locals to VAL_NONE (or another default). */
    for (size_t i = 0; i < local_count; i++) {
        frame->locals[i].type = VAL_NONE;
    }

    return frame;
}

/**
 * Destroys a frame by freeing the local array and the frame itself.
 */
void frame_destroy(Frame* frame) {
    if (!frame) return;

    /* If you had strings or objects in Value, you'd free them here or do reference counting. */
    free(frame->locals);
    free(frame);
}
