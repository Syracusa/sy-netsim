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

static void process_json(char *jsonstr)
{
    /* Get type from json */
    cJSON *json = cJSON_Parse(jsonstr);
    cJSON *type = cJSON_GetObjectItem(json, "type");
    if (type == NULL) {
        TLOGE("Can't find type in json\n");
        return;
    }
    if (cJSON_IsString(type)){
        TLOGD("type: %s\n", type->valuestring);
    }
}

void parse_client_json(SimulatorServerCtx *ssctx)
{
    size_t canread = RingBuffer_get_readable_bufsize(ssctx->recvq);
    uint16_t jsonlen;
    if (canread >= 2){
        RingBuffer_read(ssctx->recvq, &jsonlen, 2);
        jsonlen = ntohs(jsonlen);

        uint16_t tmp;
        if (canread >= 2 + jsonlen){
            RingBuffer_pop(ssctx->recvq, &tmp, 2);
            char *json = malloc(jsonlen + 1);
            RingBuffer_pop(ssctx->recvq, json, jsonlen);
            json[jsonlen] = '\0';
            process_json(json);
            free(json);
        }
    }

}

static void *do_server(void *arg)
{
    SimulatorServerCtx *ssctx = arg;


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

        char buf[1025];
        while (!ssctx->stop) {
            usleep(1000);
            int currtime = time(NULL);

            /* Recv data from USER */
            ssize_t len = recv(client_sock, buf, 1024, 0);
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

            if (readable > 1024)
                readable = 1024;

            RingBuffer_pop(ssctx->sendq, buf, readable);
            send(client_sock, buf, readable, 0);
            TLOGD("Send %ld bytes\n", readable);
        }

        TLOGE("Disconnected... Wait for new connection\n");
        close(client_sock);
    }

out:
    TLOGE("SERVER ERROR!!");
    server_end(ssctx);
    return NULL;
}

void start_server(SimulatorServerCtx *ssctx)
{
    if (ssctx->recvq == NULL)
        ssctx->recvq = RingBuffer_new(10240);

    if (ssctx->sendq == NULL)
        ssctx->sendq = RingBuffer_new(10240);

    pthread_t tcp_thread;
    pthread_create(&tcp_thread, NULL, (void *)do_server, ssctx);
}

void server_end(SimulatorServerCtx *ssctx)
{
    if (ssctx->recvq != NULL)
        RingBuffer_destroy(ssctx->recvq);

    if (ssctx->sendq != NULL)
        RingBuffer_destroy(ssctx->sendq);
}