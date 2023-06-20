#include "bufferpool.h"
#include <stdlib.h>
#include <stdio.h>

#ifndef ssize_t
typedef int64_t ssize_t;
#endif

bufferpool *new_bufferpool(int buffersize, int buffernum)
{
    bufferpool *bp = malloc(sizeof(bufferpool));

    bp->buffersize = buffersize;
    bp->buffernum = buffernum;
    bp->buffer_ptrs = RingBuffer_new(sizeof(void *) * buffernum);
    return bp;
}

void *get_buffer(bufferpool *bp)
{
    void *buffer_ptr = NULL;
    ssize_t readlen = RingBuffer_pop(bp->buffer_ptrs, &buffer_ptr, sizeof(void *));
    if (readlen != sizeof(void *))
        buffer_ptr = malloc(bp->buffersize);
    return buffer_ptr;
}

void return_buffer(bufferpool *bp, void *bufp)
{
    RingBuffer_push(bp->buffer_ptrs, &bufp, sizeof(void *));
}