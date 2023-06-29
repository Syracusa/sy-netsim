#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define RB_DEBUG_LOG 0
#define RB_VERBOSE_LOG 0

#define RB_DBG(...) \
    do { if (RB_DEBUG_LOG) fprintf(stderr, ##__VA_ARGS__); } while (0)

#define RB_VERBOSE(...) \
    do { if (RB_VERBOSE_LOG) fprintf(stderr, ##__VA_ARGS__); } while (0)

#define PIUTIL_BUFFER_FULL -1
#define PIUTIL_BUFFER_DATA_NOT_ENOUGH -2

typedef struct
{
    size_t size;
    size_t max_offset;
    size_t head;
    size_t tail;

    uint8_t* buf;
} RingBuffer;


RingBuffer* RingBuffer_new(size_t size);

void RingBuffer_destroy(RingBuffer* ringbuf);

ssize_t RingBuffer_push(RingBuffer* ringbuf, void* data, int len);

ssize_t RingBuffer_read(RingBuffer* ringbuf, void* buf, int readlen);

ssize_t RingBuffer_pop(RingBuffer* ringbuf, void* buf, int readlen);

size_t RingBuffer_get_remain_bufsize(RingBuffer* ringbuf);

size_t RingBuffer_get_readable_bufsize(RingBuffer* ringbuf);

#endif