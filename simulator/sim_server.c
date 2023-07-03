#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cJSON.h"

#include "log.h"
#include "sim_server.h"

extern void app_exit(int signo);
extern int g_exit;

#define TCP_BUF_SIZE 100000
static void *do_server(void *arg)
{
    SimulatorServerCtx *ssctx = arg;
    uint8_t buf[TCP_BUF_SIZE];

    /* Block SIGINT signal */
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    /* Start TCP Server */
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        TLOGE("Can't create TCP socket\n");
        goto out;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12123);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* Add reuseaddr option */
    int optval = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(tcp_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        TLOGE("Can't bind TCP socket\n");
        goto out;
    }

    if (listen(tcp_sock, 1) < 0) {
        TLOGE("Can't listen TCP socket\n");
        goto out;
    }

    /* Make socket nonblocking */
    int flags = fcntl(tcp_sock, F_GETFL, 0);
    fcntl(tcp_sock, F_SETFL, flags | O_NONBLOCK);

    while (!ssctx->stop) {
        int client_sock = accept(tcp_sock, NULL, NULL);
        if (client_sock < 0) {
            TLOGE("Can't accept TCP socket\n");
            sleep(1);
            continue;
        }

        int last_recv = time(NULL);

        while (!ssctx->stop) {
            usleep(1000);
            int currtime = time(NULL);

            /* Recv data from USER */
            ssize_t len = recv(client_sock, buf, TCP_BUF_SIZE, 0);
            
            if (len < 0) {
                TLOGE("Recv error\n");
                break;
            } else if (len == 0) {
                if (currtime - last_recv > 5) {
                    TLOGE("Recv timeout\n");
                    break; /* Timeout */
                }
            } else {
                last_recv = currtime;
                buf[len] = '\0';
                // TLOGD("Recv %ld bytes\n", len);
                RingBuffer_push(ssctx->recvq, buf, len);
            }

            /* Send data to USER */
            size_t readable = RingBuffer_get_readable_bufsize(ssctx->sendq);
            if (readable == 0)
                continue;

            if (readable > TCP_BUF_SIZE)
                readable = TCP_BUF_SIZE;

            RingBuffer_pop(ssctx->sendq, buf, readable);
            send(client_sock, buf, readable, 0);
            TLOGD("Send %ld bytes\n", readable);
        }

        TLOGE("Disconnected... Wait for new connection\n");
        close(client_sock);
    }
out:
    close(tcp_sock);
    server_end(ssctx);
    TLOGE("TCP Connection end\n");
    return NULL;
}

void start_server(SimulatorServerCtx *ssctx)
{
    if (ssctx->recvq == NULL)
        ssctx->recvq = RingBuffer_new(1024000);

    if (ssctx->sendq == NULL)
        ssctx->sendq = RingBuffer_new(1024000);

    pthread_create(&ssctx->tcp_thread, NULL, (void *)do_server, ssctx);
}

void server_end(SimulatorServerCtx *ssctx)
{
    if (ssctx->recvq != NULL){
        RingBuffer_destroy(ssctx->recvq);
        ssctx->recvq = NULL;
    }

    if (ssctx->sendq != NULL){
        RingBuffer_destroy(ssctx->sendq);
        ssctx->sendq = NULL;
    }
}