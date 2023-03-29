#ifndef MQ_H
#define MQ_H
#include <sys/msg.h>

#define MQ_MAX_DATA_LEN 2000

typedef struct  {
    long type;       /* message type, must be > 0 */
    char text[MQ_MAX_DATA_LEN];    /* message data */
} MqMsgbuf;

void mq_flush(int mqid);

#endif