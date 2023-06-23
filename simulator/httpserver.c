#include <libwebsockets.h>

static int http_callback(struct lws* wsi,
                         enum lws_callback_reasons reason,
                         void* user,
                         void* in,
                         size_t len)
{
    switch (reason)
    {
        case LWS_CALLBACK_RECEIVE:
            fprintf(stderr, "Receive callback. len %lu\n", len);
            lws_callback_on_writable_all_protocol(lws_get_context(wsi),
                                                  lws_get_protocol(wsi));
            break;
        case LWS_CALLBACK_SERVER_WRITEABLE:
            fprintf(stderr, "Server Writable!\n");
            /* TODO */
            break;
        case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
            fprintf(stderr, "LWS_CALLBACK_FILTER_NETWORK_CONNECTION\n");
            break;
        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
            fprintf(stderr, "LWS_CALLBACK_WS_PEER_INITIATED_CLOSE\n");
            break;
        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "LWS_CALLBACK_CLOSED\n");
            break;
        case LWS_CALLBACK_WSI_DESTROY:
            fprintf(stderr, "LWS_CALLBACK_WSI_DESTROY\n");
            break;
        default:
            fprintf(stderr, "Unhandled callback %d\n", reason);
            break;
    }
    return 0;
}


void init_http_server() {
    
    static struct lws_protocols protocols[] = {
        {"http-only", http_callback, 0, 0},
        { NULL, NULL, 0, 0 } /* terminator */ };

    (void)protocols;

}