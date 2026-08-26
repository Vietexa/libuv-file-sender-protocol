#include "user_input.h"
#include "app_ctx.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void on_write_end(uv_write_t *req, int status);


static void prompt_mode(void){
    printf("Enter what you'd like to do:\n1.Send file\n2.Request file\n");
}


static void handle_send_file_input(app_ctx_t *ctx, const char *file_path){

    if(file_path[0] == '\0'){
        fprintf(stderr, "No name or path entered!\n");
        printf("Enter the path of the file you want to send:\n");
        return;
    }

    uint64_t len = 0;
    uint8_t *data = prepare_buffer(ctx, file_path, &len);

    uv_write_t *req = malloc(sizeof(*req));
    
    if (!req) {
        free(data);
        return;
    }

    uv_buf_t buffer = uv_buf_init((char *)data, len);

    req->data = data;

    int rc = uv_write(req,(uv_stream_t *)ctx->tcp,&buffer,1,on_write_end);

    if (rc < 0) {
        fprintf(stderr, "uv_write: %s\n", uv_strerror(rc));
        free(data);
        free(req);
    }

    ctx->user_input.input_state = INPUT_MODE;
    prompt_mode();

}

static void handle_request_file_input(app_ctx_t *ctx, const char *file_name){
    
    int len = 0;

    if (!request_file(ctx, file_name, &len)) {
        printf("Enter the name of the file you want to request:\n");
        return;
    }

    uv_buf_t buffer = uv_buf_init(
        (char *)ctx->receive_file.receive_buf,
        len
    );

    uv_write_t *req = malloc(sizeof(*req));
    if (!req) {
        free(ctx->receive_file.receive_buf);
        return;
    }

    req->data = ctx->receive_file.receive_buf;

    int rc = uv_write(req,(uv_stream_t *)ctx->tcp, &buffer, 1, on_write_end);

    if (rc < 0) {
        fprintf(stderr, "uv_write: %s\n", uv_strerror(rc));
        free(req->data);
        free(req);
    }

    ctx->user_input.input_state = INPUT_MODE;
    prompt_mode();
}

static void handle_mode_input(app_ctx_t *ctx, const char *line)
{
    if (strcmp(line, "1") == 0) {
        ctx->user_input.input_state = INPUT_SEND_FILE_PATH;

        printf("Enter the path of the file you want to send:\n");
        return;
    }

    if (strcmp(line, "2") == 0) {
        ctx->user_input.input_state = INPUT_REQUEST_FILE_NAME;

        printf("Enter the name of the file you want to request:\n");
        return;
    }

    fprintf(stderr, "Error: Invalid Mode!\n");
    prompt_mode();
    
}


void handle_input_line(app_ctx_t *ctx, const char *line)
{
    switch (ctx->user_input.input_state) {

    case INPUT_MODE:
        handle_mode_input(ctx, line);
        break;

    case INPUT_SEND_FILE_PATH:
        handle_send_file_input(ctx, line);
        break;

    case INPUT_REQUEST_FILE_NAME:
        handle_request_file_input(ctx, line);
        break;
    }
}
