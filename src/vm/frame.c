#include "frame.h"
#include <stdlib.h>

Frame* frame_create(size_t local_count, Frame* parent) {
    Frame* frame = (Frame*)malloc(sizeof(Frame));
    if (!frame) {
        return NULL;
    }
    frame->local_count = local_count;
    frame->locals = (int*)calloc(local_count, sizeof(int));
    frame->parent = parent;
    return frame;
}

void frame_destroy(Frame* frame) {
    if (!frame) return;
    free(frame->locals);
    free(frame);
}
