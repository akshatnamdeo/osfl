#include "frame.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../include/value.h"

Frame* frame_create(size_t local_count, Frame* parent) {
    Frame* f = (Frame*)malloc(sizeof(Frame));
    if (!f) {
        fprintf(stderr, "Failed to allocate Frame.\n");
        return NULL;
    }
    f->local_count = local_count;
    f->parent = parent;
    f->locals = (Value*)calloc(local_count, sizeof(Value));
    if (!f->locals) {
        fprintf(stderr, "Failed to allocate Frame locals.\n");
        free(f);
        return NULL;
    }
    
    /* Initialize all to VAL_NULL */
    for (size_t i = 0; i < local_count; i++) {
        f->locals[i] = VALUE_NULL;  // Using the VALUE_NULL macro from value.h
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