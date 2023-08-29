#include <stdio.h>
#include <string.h>
#include <errno.h>

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

void send_mq(int mqid,
			 void *data,
			 size_t len,
			 long type)
{
    MqMsgbuf msg;
    msg.type = type;

    if (len > MQ_MAX_DATA_LEN) {
        fprintf(stderr, "Can't send data with length %lu\n", len);
    }
    memcpy(msg.text, data, len);
    int ret = msgsnd(mqid, &msg, len, IPC_NOWAIT);
    if (ret < 0) {
        if (errno == EAGAIN) {
            fprintf(stderr, "Message queue full!\n");
        } else {
            fprintf(stderr, "Can't send config. mqid: %d len: %lu(%s)\n",
                  mqid, len, strerror(errno));
        }
    }
}