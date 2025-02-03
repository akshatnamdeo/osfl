#ifndef FRAME_H
#define FRAME_H

#include <stddef.h>

/* The Frame structure represents a call frame in the VM.
   It includes a pointer to local variables and a pointer to the parent frame.
*/
typedef struct Frame {
    int* locals;           /* Array of local variables */
    size_t local_count;    /* Number of local variables */
    struct Frame* parent;  /* Parent frame for dynamic scoping */
} Frame;

/* Create a new frame with a given number of local variables.
   'parent' is the frame from which this one inherits.
*/
Frame* frame_create(size_t local_count, Frame* parent);

/* Destroy a frame and free its memory. */
void frame_destroy(Frame* frame);

#endif // FRAME_H
