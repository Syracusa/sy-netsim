#include <stdio.h>
#include <time.h>
#include "../util/util.h"
#include "log.h"

void send_mq(){

}

void recv_mq(){

}

void mainloop(){
    struct timespec before, after, diff, req, rem;

    while(1){
        clock_gettime(CLOCK_REALTIME, &before);
        
        TLOGI("TICK!\n");

        send_mq();
        recv_mq();
        
        clock_gettime(CLOCK_REALTIME, &after);
        timespec_sub(&after, &before, &diff);
        if (diff.tv_nsec < 1000 * 1000){
            req.tv_sec = 0;
            req.tv_nsec = 1000 * 1000 - diff.tv_nsec;
            nanosleep(&req, &rem);
        }
    }
}

int main(int argc, char* argv[])
{
    mainloop();
    return 0;
}