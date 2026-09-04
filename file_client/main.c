#include "uv.h"

#include "app_ctx.h"
#include "file.h"
#include "user_input.h"
#include "utils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"


#include "cJSON.h"



void stdin_read(uv_stream_t *stream, ssize_t nread, const struct uv_buf_t *buf){

    app_ctx_t *ctx = stream->data;

    if (nread < 0){

        if (nread != UV_EOF){
            fprintf(stderr,"stdin read error: %s\n", uv_strerror(nread));
        }
        free(buf->base);
        uv_read_stop(stream);
        return;
    }
        for (ssize_t i = 0; i < nread; i++) {
        char c = buf->base[i];

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            ctx->user_input.input_buf[ctx->user_input.input_len] = '\0';

            handle_input_line(ctx, ctx->user_input.input_buf);

            ctx->user_input.input_len = 0;
            continue;
        }

        if (ctx->user_input.input_len + 1 < sizeof(ctx->user_input.input_buf)) {
            ctx->user_input.input_buf[ctx->user_input.input_len] = c;
            ctx->user_input.input_len++;
        }
    }


    free(buf->base);

}

void stdin_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void on_close(uv_handle_t *handle) {
    app_ctx_t *ctx = handle->data;
    free(ctx->file_buf);
    free(handle);
}


void echo_read(uv_stream_t *server, ssize_t nread, const uv_buf_t *buf) {

    app_ctx_t *ctx = server->data;

     if (nread < 0) {

        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));

        free(buf->base);
        uv_close((uv_handle_t*)server, on_close);
        return;
        
    }

    if (!parse_file(ctx, server, nread, buf)){
        fprintf(stderr, "echo_read, Critical error: parse_file failed!\n");
        free(buf->base);
        uv_close((uv_handle_t *)server, on_close);
        return;
    }
    
    free(buf->base);
}

void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {

    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void on_write_end(uv_write_t *req, int status) {
    uint8_t *old_send_data = req->data;

    if (status < 0) {
        fprintf(stderr, "write error: %s\n", uv_strerror(status));
        free(req->data);
        free(req);
        return;
    }

    free(old_send_data);

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
    
    uv_read_start((uv_stream_t *)ctx->tcp, alloc_buffer, echo_read);
    uv_tty_init(uv_default_loop(), &ctx->stdin_tty, 0, 1);

    ctx->stdin_tty.data = ctx;
    ctx->user_input.input_state = INPUT_MODE;
    ctx->user_input.input_len = 0;

    printf("Enter what you would like to do:\n1.Send file\n2.Request file\n");

    uv_read_start((uv_stream_t *)&ctx->stdin_tty, stdin_alloc, stdin_read);

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

char json_file[300];
char address_str[124];
int port_number = 0;

if(!parse_json_file("config.json", json_file, sizeof(json_file))) return -1;

cJSON *json = cJSON_Parse(json_file);
const cJSON *address_json = NULL;
const cJSON *port_json = NULL;

if (!json){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        return 1;
    }

address_json = cJSON_GetObjectItemCaseSensitive(json, "address");

if(cJSON_IsString(address_json) && address_json->valuestring){
    snprintf(address_str, sizeof(address_str), "%s", address_json->valuestring);
}

port_json = cJSON_GetObjectItemCaseSensitive(json, "port");

if(cJSON_IsNumber(port_json)){
port_number = port_json->valueint;
}

cJSON_Delete(json);

printf("Address: %s Port: %d\n",address_str, port_number);

uv_connect_t *connect = malloc(sizeof(uv_connect_t));
connect->data = &ctx; 

struct sockaddr_in dest;
uv_ip4_addr(address_str, port_number, &dest); 


uv_tcp_connect(connect, socket, (const struct sockaddr*)&dest, on_connect);


uv_run(loop, UV_RUN_DEFAULT);


}