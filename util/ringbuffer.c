#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ringbuffer.h"

RingBuffer* RingBuffer_new(size_t size)
{
    RingBuffer* ringbuf;
    ringbuf = malloc(sizeof(RingBuffer));

    ringbuf->max_offset = size;
    ringbuf->size = size;
    ringbuf->head = 0;
    ringbuf->tail = 0;
    ringbuf->buf = malloc(size + 1);

    RB_DBG("RingBuffer Created. size : %lu\n", size);
    return ringbuf;
}

void RingBuffer_destroy(RingBuffer* ringbuf)
{
    if (ringbuf)
    {
        if (ringbuf->buf)
            free(ringbuf->buf);
        free(ringbuf);
    }
    RB_DBG("RingBuffer destoryed.\n");
}

size_t RingBuffer_get_remain_bufsize(RingBuffer* ringbuf)
{
    if (ringbuf->head == ringbuf->tail)
    {
        /* buffer is empty */
        return ringbuf->size;
    }
    else if (ringbuf->head < ringbuf->tail)
    {
        int in_use = ringbuf->tail - ringbuf->head;
        return ringbuf->size - in_use;
    }
    else
    {
        int diff = ringbuf->head - ringbuf->tail;
        return diff - 1;
    }
}

size_t RingBuffer_get_readable_bufsize(RingBuffer* ringbuf)
{
    size_t remain = RingBuffer_get_remain_bufsize(ringbuf);
    return ringbuf->size - remain;
}

ssize_t RingBuffer_push(RingBuffer* ringbuf, void* data, int len)
{
    RB_VERBOSE("Try push.. len : %d\n", len);

    int remain = RingBuffer_get_remain_bufsize(ringbuf);
    if (len > remain)
    {
        RB_DBG("Buffer full. remain %d requested %d\n", remain, len);
        return PIUTIL_BUFFER_FULL;
    }

    int cont_remain = ringbuf->max_offset - ringbuf->tail + 1;
    if (cont_remain < len)
    {
        /* Write through each edges of buffer */
        memcpy(&(ringbuf->buf[ringbuf->tail]), data, cont_remain);
        memcpy(ringbuf->buf, &(((uint8_t*)data)[cont_remain]), len - cont_remain);
        ringbuf->tail = len - cont_remain;

        RB_VERBOSE("Push success... head %lu tail %lu\n",
                       ringbuf->head, ringbuf->tail);


        return len; /* Done */
    }

    memcpy(&(ringbuf->buf[ringbuf->tail]), data, len);
    ringbuf->tail += len;
    ringbuf->tail %= ringbuf->max_offset + 1;

    RB_VERBOSE("Push success... head %lu tail %lu\n",
                   ringbuf->head, ringbuf->tail);
    return len;
}

static ssize_t RingBuffer_read_internal(RingBuffer* ringbuf,
                                           void* buf,
                                           int readlen,
                                           int do_pop)
{
    RB_VERBOSE("Try read(pop).. len : %d\n", readlen);
    int readable = RingBuffer_get_readable_bufsize(ringbuf);

    if (readlen > readable)
    {
        return PIUTIL_BUFFER_DATA_NOT_ENOUGH;
    }

    int cont_readable = ringbuf->max_offset - ringbuf->head + 1;
    if (cont_readable < readlen)
    {
        /* Read through each edges of buffer */
        memcpy(buf, &(ringbuf->buf[ringbuf->head]), cont_readable);
        memcpy(&(((uint8_t*)buf)[cont_readable]), ringbuf->buf, readlen - cont_readable);
        if (do_pop)
        {
            ringbuf->head = readlen - cont_readable;
            RB_VERBOSE("Pop success... head %lu tail %lu\n",
                            ringbuf->head, ringbuf->tail);
        }
        else
        {
            RB_VERBOSE("Push success... head %lu tail %lu\n",
                            ringbuf->head, ringbuf->tail);
        }

        return readlen; /* Done */
    }
    
    memcpy(buf, &(ringbuf->buf[ringbuf->head]), readlen);
    if (do_pop)
    {
        ringbuf->head += readlen;
        ringbuf->head %= ringbuf->max_offset + 1;
        RB_VERBOSE("Pop success... head %lu tail %lu\n",
                       ringbuf->head, ringbuf->tail);
    }
    else
    {
        RB_VERBOSE("Push success... head %lu tail %lu\n",
                       ringbuf->head, ringbuf->tail);
    }
    return readlen;
}

ssize_t RingBuffer_read(RingBuffer* ringbuf, void* buf, int readlen)
{
    return RingBuffer_read_internal(ringbuf, buf, readlen, 0);
}

ssize_t RingBuffer_pop(RingBuffer* ringbuf, void* buf, int readlen)
{
    return RingBuffer_read_internal(ringbuf, buf, readlen, 1);
}

