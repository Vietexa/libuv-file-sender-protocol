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
} app_ctx_t;

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
} write_req_t;