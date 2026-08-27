#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

typedef struct peer_t{
    uv_tcp_t *client;
    char name[100];
} peer_t;

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
} network_file_t;

typedef struct file_req_s { // used for parsing the mode 2 sent by the client
    uint8_t file_name_header[4];
    size_t f_n_header_length;
    size_t f_n_header_len_conv;
    size_t file_name_length;
    uint8_t *file_name;
} file_req_t;

typedef struct file_send_s{ // used for sending the file the client asked for
uint64_t file_buf_capacity;
uint64_t file_buf_len;
uint8_t *file_buf;
} file_send_t;

typedef struct clients_t{
peer_t **peers;
int client_count;
int client_capacity;
} clients_t;

typedef struct app_ctx_t{
    uv_loop_t *loop;
    struct sockaddr_in addr;
    clients_t clients;
    network_file_t network_file;
    file_req_t file_req;
    file_send_t file_send;
    uint8_t selected_mode;
    bool process_fatal_error;
} app_ctx_t;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;