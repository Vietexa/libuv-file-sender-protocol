#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <uv.h>

typedef enum {
    INPUT_MODE,
    INPUT_SEND_FILE_PATH,
    INPUT_REQUEST_FILE_NAME
} input_state_t;

typedef enum {
    MODE_SEND_FILE = 1,
    MODE_REQUEST_FILE,
    MODE_RECEIVE_ERROR
} control_mode_t;

typedef struct user_input_s {
    input_state_t input_state;
    char input_buf[1024];
    size_t input_len;
    
} user_input_t;

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
} network_file_t;

typedef struct error_rcv_s {
uint8_t error_header[4];
size_t error_header_len;
uint32_t error_capacity;
size_t error_len;
uint8_t *error;
} error_rcv_t;



typedef struct app_ctx_s{
uint8_t mode;
uv_tcp_t *tcp;
uint8_t *file_buf;
uint64_t file_buf_capacity;
uint64_t file_buf_len;
rcv_file_t receive_file;
network_file_t network_file;
user_input_t user_input;
error_rcv_t error_rcv;
uv_tty_t stdin_tty;
} app_ctx_t;
