#include "frame.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Frame* frame_create(size_t local_count, Frame* parent) {
    Frame* f = (Frame*)malloc(sizeof(Frame));
    if (!f) {
        fprintf(stderr, "Failed to allocate Frame.\n");
        return NULL;
    }
    f->local_count = local_count;
    f->parent = parent;
    f->locals = (FrameValue*)calloc(local_count, sizeof(FrameValue));
    if (!f->locals) {
        fprintf(stderr, "Failed to allocate Frame locals.\n");
        free(f);
        return NULL;
    }
    /* Initialize all to FRAME_VAL_NONE. */
    for (size_t i = 0; i < local_count; i++) {
        f->locals[i].type = FRAME_VAL_NONE;
    }
    return f;
}

void frame_destroy(Frame* frame) {
    if (!frame) return;
    /* If we had references to objects or strings in these locals,
       we'd release them or handle reference counting. */
    free(frame->locals);
    free(frame);
}
