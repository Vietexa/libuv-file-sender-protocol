#pragma once

#include <stdbool.h>
#include <uv.h>

typedef struct message_buffer_s{
char *buffer;
int length;
} message_buffer_t;

typedef struct rcv_file_s {
    uint8_t *receive_buf;
    uint64_t receive_buf_cap;
    uint64_t receive_buf_len;
} rcv_file_t;

typedef struct network_file_s {
    uint64_t file_capacity;
    uint64_t file_length;
    uint8_t *file;
    uint8_t header[8];
    size_t header_length;
    uint32_t file_name_capacity;
    uint8_t file_name_header[4];
    size_t file_name_header_lenght;
    char file_name[255];
    uint32_t file_name_bytes_cp;
    bool string_copied;
    uint8_t mode;
} network_file_t;


typedef struct app_ctx_s{
uv_tcp_t *tcp;
uint8_t *file_buf;
uint64_t file_buf_capacity;
uint64_t file_buf_len;
rcv_file_t receive_file;
network_file_t network_file;
} app_ctx_t;
