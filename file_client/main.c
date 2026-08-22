#include "utils.h"
#include "uv.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <uv/unix.h>
#include "stdbool.h"
#include "app_ctx.h"


void on_close(uv_handle_t *handle) {
    app_ctx_t *ctx = handle->data;
    free(ctx->file_buf);
    free(handle);
}


void echo_read(uv_stream_t *server, ssize_t nread, const uv_buf_t* buf) {

}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {

    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void on_write_end(uv_write_t *req, int status) {
    app_ctx_t *ctx = req->handle->data;
    uint8_t *old_send_data = req->data;

    if (status < 0) {
        fprintf(stderr, "write error: %s\n", uv_strerror(status));
        free(req->data);
        free(req);
        return;
    }

    free(old_send_data);

    uint64_t buf_len = 0;

    uint8_t *send_data = prepare_buffer(ctx, &buf_len);
    uv_buf_t send_buffer = uv_buf_init((char *)send_data, buf_len);

    uv_write_t *write_req = malloc(sizeof(*write_req));
    write_req->data = send_data;

    uv_write(write_req, req->handle, &send_buffer, 1, on_write_end);

    free(req);

    

}

void on_connect(uv_connect_t *req, int status) {

    app_ctx_t *ctx = req->data; 
    
    if (status < 0) {
        req->handle->data = ctx;
        fprintf(stderr, "Error connecting!\n");
        uv_close((uv_handle_t *)req->handle, on_close);
        
        free(req);
        return;
    }

    ctx->tcp = (uv_tcp_t *)req->handle; 


    ctx->tcp->data = ctx;

    uint64_t buf_len = 0;
    uint8_t *send_data = prepare_buffer(ctx, &buf_len);

    uv_buf_t send_buffer = uv_buf_init((char *)send_data, buf_len);

    uv_write_t *write_req = malloc(sizeof(*write_req));
    write_req->data = send_data;


    uv_write(write_req, req->handle, &send_buffer, 1, on_write_end);
    
    uv_read_start((uv_stream_t *)ctx->tcp, alloc_buffer, echo_read);

    free(req);
}

int main(void){

app_ctx_t ctx = {0};

ctx.file_buf = malloc(4096);
ctx.file_buf_capacity = 4096;

uv_loop_t *loop = uv_default_loop();
uv_tcp_t *socket = malloc(sizeof(uv_tcp_t));
uv_tcp_init(loop, socket);

ctx.tcp = NULL; 


uv_connect_t *connect = malloc(sizeof(uv_connect_t));
connect->data = &ctx; 

struct sockaddr_in dest;
uv_ip4_addr("127.0.0.1", 7000, &dest); 


uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connect);


uv_run(loop, UV_RUN_DEFAULT);


}