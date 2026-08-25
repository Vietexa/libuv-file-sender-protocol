#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <uv.h>

#include "include/alloc.h"
#include "include/data_structures.h"
#include "include/file.h"

#define DEFAULT_PORT 7000
#define DEFAULT_BACKLOG 128

int get_current_peer(app_ctx_t *app_context, uv_stream_t *client){
    
    for (int i = 0; i < app_context->clients.client_capacity; i++){
        uv_tcp_t *peer = app_context->clients.peers[i]->client;

        if (!peer) continue;
        if (peer == (uv_tcp_t*)client) return i;

    }
    return -1;
}

void free_write_req(uv_write_t *req) {
    write_req_t *wr = (write_req_t*) req;
    free(wr->buf.base);
    free(wr);
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base = (char*) malloc(suggested_size);
    buf->len = suggested_size;
}

void on_close(uv_handle_t* handle) {
   app_ctx_t *app_ctx = handle->data;
    
    for (int i = 0; i < app_ctx->clients.client_capacity; i++) {

        if (app_ctx->clients.peers[i]->client == (uv_tcp_t*)handle) {

            app_ctx->clients.peers[i]->client = NULL;
            app_ctx->clients.client_count--;

            break;
        }
    }


    free(handle);
}

void echo_write(uv_write_t *req, int status) {
    if (status) {
        fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }
    free_write_req(req);
}

void on_write_end(uv_write_t *req, int status) {
    printf("on_write_end called, status=%d\n", status);
    app_ctx_t *ctx = req->handle->data;
    if (status < 0) {
        fprintf(stderr, "write error: %s\n", uv_strerror(status));
    }
        free(req->data);
        free(req);
        free(ctx->file_req.file_name);
        ctx->file_req.file_name = NULL;
        
        ctx->file_req.file_name_length = 0;
        ctx->file_req.f_n_header_len_conv = 0;

        ctx->file_send.file_buf_len = 0;

}

void echo_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    app_ctx_t *app_context = client->data;

    if (nread < 0) {

        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));

        free(buf->base);
        uv_close((uv_handle_t*)client, on_close);
        return;
        
    }
    
    process_file(app_context, client, buf, nread);
        
    free(buf->base);
}

void on_new_connection(uv_stream_t *server, int status) {
    app_ctx_t *app = server->data;

    if (status < 0) {
        fprintf(stderr, "New connection error %s\n", uv_strerror(status));
        // error!
        return;
    }

    uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(app->loop, client);
    client->data = server->data;
    if (uv_accept(server, (uv_stream_t*) client) == 0) {

        if (app->clients.client_count == app->clients.client_capacity){
            if(realloc_clients(app) != 0){
                uv_close((uv_handle_t *)client, on_close);
                return;
            }
        }

        for (int i = 0; i < app->clients.client_capacity; i++){
            
            if (!app->clients.peers[i]->client){
                app->clients.peers[i]->client = client;
                app->clients.client_count++;
                break;
            }
        }

        printf("Client count: %d\n", app->clients.client_count);
        printf("Client capacity: %d\n", app->clients.client_capacity);

        uv_read_start((uv_stream_t*) client, alloc_buffer, echo_read);
    }
    else {
        uv_close((uv_handle_t*) client, on_close);
    }
}

int main(void) {
    app_ctx_t app_ctx = {0};
    app_ctx.file_send.file_buf_capacity = 5000;
    app_ctx.file_send.file_buf = malloc(app_ctx.file_send.file_buf_capacity);
    app_ctx.clients.client_capacity = 1;
    app_ctx.clients.peers = calloc(app_ctx.clients.client_capacity,
                                   sizeof(*app_ctx.clients.peers));

for (int i = 0; i < app_ctx.clients.client_capacity; i++) {
        app_ctx.clients.peers[i] = calloc(1, sizeof(*app_ctx.clients.peers[i]));
    }

    app_ctx.loop = uv_default_loop();

    uv_tcp_t server;
    
    app_ctx.clients.client_count = 0;

    uv_tcp_init(app_ctx.loop, &server);
    server.data = &app_ctx;

    uv_ip4_addr("0.0.0.0", DEFAULT_PORT, &app_ctx.addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&app_ctx.addr, 0);
    int r = uv_listen((uv_stream_t*) &server, DEFAULT_BACKLOG, on_new_connection);
    
    if (r) {
        fprintf(stderr, "Listen error %s\n", uv_strerror(r));
        return 1;
    }

    int run_ret_value = uv_run(app_ctx.loop, UV_RUN_DEFAULT);

    uv_loop_close(app_ctx.loop);

    return run_ret_value;
}