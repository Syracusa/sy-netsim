#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h> /* abort() */

#include "mq.h"

void mq_flush(int mqid)
{
    MqMsgbuf msg;
    int buf_size = sizeof(msg.text);

    while (1) {
        int msg_size = msgrcv(mqid, &msg, buf_size, 0, IPC_NOWAIT);
        if (msg_size < 0) {
            break;
        }
    }
}

int send_mq(int mqid,
             void *data,
             size_t len,
             long type)
{
    /* The mtype field must have a strictly positive integer value. */
    if (type < 1) {
        fprintf(stderr, "Can't send meg with type %ld\n", type);
        return -1;
    }

    MqMsgbuf msg;
    msg.type = type;

    if (len > MQ_MAX_DATA_LEN) {
        fprintf(stderr, "Can't send data with length %lu\n", len);
    }
    memcpy(msg.text, data, len);
    int ret = msgsnd(mqid, &msg, len, IPC_NOWAIT);
    if (ret < 0) {
        if (errno == EAGAIN) {
            fprintf(stderr, "Message queue full! mqid: %d len: %lu(%s)\n",  mqid, len, strerror(errno));
            // abort();
            return 0;
        } else {
            fprintf(stderr, "Can't send. mqid: %x len: %lu(%s)\n",
                    mqid, len, strerror(errno));
            return -2;
        }
    }

    return 1;
}

ssize_t recv_mq(int mqid,
                MqMsgbuf *msg)
{
    ssize_t res = msgrcv(mqid, msg, sizeof(msg->text), 0, IPC_NOWAIT);
    if (res < 0) {
        if (errno != ENOMSG) 
            fprintf(stderr, "Msgrcv failed(err: %s)\n", strerror(errno));
    }
    return res;
}

