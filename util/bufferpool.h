#ifndef __BUFFERPOOL_H__
#define __BUFFERPOOL_H__

#include "ringbuffer.h"

typedef struct _bufferpool{
    int buffersize;
    int buffernum;
    RingBuffer* buffer_ptrs;    
} bufferpool;

bufferpool* new_bufferpool(int buffersize, int buffernum);
void* get_buffer(bufferpool* bp);
void return_buffer(bufferpool* bp, void* bufp);

#endif