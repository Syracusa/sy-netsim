#include <sys/msg.h>

#include "mq.h"

void mq_flush(int mqid)
{
	MqMsgbuf msg;
	int buf_size = sizeof(msg.text);

	while (1){
		int msg_size = msgrcv(mqid, &msg, buf_size, 0, IPC_NOWAIT);
		if (msg_size < 0){
			break;
		}
	}
}