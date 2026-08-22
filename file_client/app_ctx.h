#pragma once

#include <uv.h>

typedef struct message_buffer_s{
char *buffer;
int length;
} message_buffer_t;


typedef struct app_ctx_s{
uv_tcp_t *tcp;
uint8_t *file_buf;
uint64_t file_buf_capacity;
uint64_t file_buf_len;
} app_ctx_t;
