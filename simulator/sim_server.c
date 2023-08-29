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
#define TCP_BUF_SIZE 10240

int g_flag_server_mainloop_exit = 0;

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

            /* Recv data from USER */
            ssize_t len = recv(client_sock, buf, TCP_BUF_SIZE - 1, 0);

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
                RingBuffer_push(ssctx->recvq, buf, len);
            }

            /* Send data to USER */

            int keep_send;
            do {
                keep_send = 0;

                size_t readable = RingBuffer_get_readable_bufsize(ssctx->sendq);
                if (readable == 0)
                    continue;

                if (readable > TCP_BUF_SIZE){
                    readable = TCP_BUF_SIZE;
                    keep_send = 1;
                }

                RingBuffer_pop(ssctx->sendq, buf, readable);
                send(client_sock, buf, readable, 0);
                if (TRX_VERBOSE)
                    TLOGD("Send %ld bytes\n", readable);
            } while (keep_send);
        }

        RingBuffer_drop_buffer(ssctx->recvq);
        RingBuffer_drop_buffer(ssctx->sendq);

        TLOGE("Disconnected... Wait for new connection\n");
        close(client_sock);
    }
out:
    close(tcp_sock);
    simulator_stop_server(ssctx);
    TLOGE("TCP Connection end\n");
    return NULL;
}

void simulator_start_server(SimulatorServerCtx *ssctx)
{
    if (ssctx->recvq == NULL)
        ssctx->recvq = RingBuffer_new(819200);

    if (ssctx->sendq == NULL)
        ssctx->sendq = RingBuffer_new(819200);

    pthread_create(&ssctx->tcp_thread, NULL, (void *)do_server, ssctx);
}

void simulator_stop_server(SimulatorServerCtx *ssctx)
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