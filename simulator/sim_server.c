#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "cJSON.h"

#include "log.h"
#include "sim_server.h"
#include "sim_remote_conf.h"
#include "sim_hdlreport.h"

#define TRX_VERBOSE 0
#define TCP_IO_SIZE 10240
#define TCP_BUFFER_SIZE 819200

/** When simulator recv some signal then this flag will be 1 */
int g_flag_server_mainloop_exit = 0;

/** Thread for handle TCP IO. Recved data will be buffered and will be handled at main thread */
static void *do_server(void *arg)
{
    SimulatorServerCtx *ssctx = arg;
    uint8_t buf[TCP_IO_SIZE];

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

    /* Use REUSEADDR option */
    int optval = 1;
    setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    /* Use TCP_NODELAY option */
    optval = 1;
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));

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
            RingBuffer_drop_buffer(ssctx->recvq);
            RingBuffer_drop_buffer(ssctx->sendq);
            sleep(1);
            continue;
        }

        int last_recv = time(NULL);

        while (!ssctx->stop) {
            /* Make client sock nonblocking */
            int flags = fcntl(client_sock, F_GETFL, 0);
            fcntl(client_sock, F_SETFL, flags | O_NONBLOCK);

            usleep(10000);
            int currtime = time(NULL);

            /* Recv data from frontend */
            ssize_t len = recv(client_sock, buf, TCP_IO_SIZE - 1, 0);

            if (len <= 0) {
                if (currtime - last_recv > 5) {
                    TLOGE("Recv timeout\n");
                    break; /* Timeout */
                }
            } else {
                last_recv = currtime;
                buf[len] = '\0';
                if (TRX_VERBOSE)
                    TLOGD("Recv %ld bytes\n", len);

                /* Just push data to recvq. Main thread will handle it */
                RingBuffer_push(ssctx->recvq, buf, len);
            }

            /* Send buffered data to frontend. */
            int keep_send;
            do {
                keep_send = 0;

                size_t readable = RingBuffer_get_readable_bufsize(ssctx->sendq);
                if (readable == 0)
                    continue;

                if (readable > TCP_IO_SIZE){
                    readable = TCP_IO_SIZE;
                    keep_send = 1;
                }

                RingBuffer_pop(ssctx->sendq, buf, readable);
                send(client_sock, buf, readable, 0);
                if (TRX_VERBOSE)
                    TLOGD("Send %ld bytes\n", readable);
            } while (keep_send);
        }

        TLOGE("Disconnected... Wait for new connection\n");
        RingBuffer_drop_buffer(ssctx->recvq);
        RingBuffer_drop_buffer(ssctx->sendq);
        close(client_sock);
    }
out:
    TLOGE("TCP Connection end\n");
    close(tcp_sock);
    simulator_free_server_buffers(ssctx);
    return NULL;
}

/** Allocate buffer and start tcp server thread */
void simulator_start_server(SimulatorServerCtx *ssctx)
{
    if (ssctx->recvq == NULL)
        ssctx->recvq = RingBuffer_new(TCP_BUFFER_SIZE);

    if (ssctx->sendq == NULL)
        ssctx->sendq = RingBuffer_new(TCP_BUFFER_SIZE);

    pthread_create(&ssctx->tcp_thread, NULL, (void *)do_server, ssctx);
}

/** Free buffers */
void simulator_free_server_buffers(SimulatorServerCtx *ssctx)
{
    if (ssctx->recvq != NULL) {
        RingBuffer_destroy(ssctx->recvq);
        ssctx->recvq = NULL;
    }

    if (ssctx->sendq != NULL) {
        RingBuffer_destroy(ssctx->sendq);
        ssctx->sendq = NULL;
    }
}

/**
 * @brief Parse tcp stream from frontend. 
 * All messages should be 2byte length header + json string.
 * 
 * @param ssctx 
 */
static void parse_client_json(SimulatorServerCtx *ssctx)
{
    if (!ssctx->recvq)
        return;

    while (1) {
        size_t canread = RingBuffer_get_readable_bufsize(ssctx->recvq);
        uint16_t jsonlen;
        if (canread >= 2) {
            RingBuffer_read(ssctx->recvq, &jsonlen, 2);
            jsonlen = ntohs(jsonlen);

            uint16_t tmp;
            if (canread >= 2 + jsonlen) {
                RingBuffer_pop(ssctx->recvq, &tmp, 2);
                char *jsonstr = malloc(jsonlen + 1);
                RingBuffer_pop(ssctx->recvq, jsonstr, jsonlen);
                jsonstr[jsonlen] = '\0';
                handle_remote_conf_msg(get_simulator_context(), jsonstr);
                free(jsonstr);
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

void simulator_server_mainloop(SimulatorCtx *sctx)
{
    while (!g_flag_server_mainloop_exit) {
        parse_client_json(&sctx->server_ctx);
        recv_report(sctx);
        usleep(10 * 1000);
    }
}