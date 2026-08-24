#include "utils.h"
#include "app_ctx.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool double_file_buf_size(app_ctx_t *ctx){

uint64_t old_capacity = ctx->file_buf_capacity;
uint64_t new_capacity = ctx->file_buf_capacity * 2;

if (ctx->file_buf_len < old_capacity) return false; // don't reallocate it yet

uint8_t *tmp = realloc(ctx->file_buf, new_capacity);

if (!tmp){
    fprintf(stderr, "Couldn't reallocate!\n");
    perror("realloc");
    return false;
}

ctx->file_buf = tmp;
ctx->file_buf_capacity = new_capacity;

return true;

}

bool get_file(app_ctx_t *ctx, char *file_name){

ctx->file_buf_len = 0;

FILE *file_to_send = fopen(file_name, "rb");

if (!file_to_send){
    perror("fopen");
    return false;
}

while (1){

    if (ctx->file_buf_len >= ctx->file_buf_capacity){
        bool status = double_file_buf_size(ctx);

        if (status == false){
            fclose(file_to_send);
            return false;
       } 
    } 
    size_t available = ctx->file_buf_capacity - ctx->file_buf_len;
    size_t bytes_read = fread(ctx->file_buf + ctx->file_buf_len,sizeof(uint8_t),available , file_to_send);
    ctx->file_buf_len += bytes_read;

    if (bytes_read < available){

        if (feof(file_to_send)) {
           fclose(file_to_send);
           return true;
        }

        if (ferror(file_to_send)) {
            perror("file");
            fclose(file_to_send);
            return false;
            }
    }
}

}

void write_u64_be(uint8_t *buf, uint64_t value)
{
    buf[0] = (uint8_t)(value >> 56);
    buf[1] = (uint8_t)(value >> 48);
    buf[2] = (uint8_t)(value >> 40);
    buf[3] = (uint8_t)(value >> 32);
    buf[4] = (uint8_t)(value >> 24);
    buf[5] = (uint8_t)(value >> 16);
    buf[6] = (uint8_t)(value >> 8);
    buf[7] = (uint8_t)value;
}

uint64_t read_u64_be(const uint8_t *buf)
{
    return ((uint64_t)buf[0] << 56) |
           ((uint64_t)buf[1] << 48) |
           ((uint64_t)buf[2] << 40) |
           ((uint64_t)buf[3] << 32) |
           ((uint64_t)buf[4] << 24) |
           ((uint64_t)buf[5] << 16) |
           ((uint64_t)buf[6] << 8)  |
           ((uint64_t)buf[7]);
}

void write_u32_be(uint8_t *buf, uint32_t value)
{
    buf[0] = (uint8_t)(value >> 24); // 11
    buf[1] = (uint8_t)(value >> 16); // 22
    buf[2] = (uint8_t)(value >> 8);  // 33
    buf[3] = (uint8_t)value;        // 44
}

uint32_t read_u32_be(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8)  |
           ((uint32_t)buf[3]);
}

uint8_t *prepare_buffer(app_ctx_t *ctx, uint64_t *buf_length){
    char file_path[1024];
    
    printf("Enter the path of the file you want to send:\n");
    fgets(file_path,sizeof(file_path), stdin);
    file_path[strcspn(file_path, "\n")] = '\0';

    if (file_path[0] == '\0'){
        fprintf(stderr, "No name or path entered!\n");
        return NULL;
    }

    char *file_name = strrchr(file_path, '/');

    if (file_name) {
        file_name++;  // move past the /
    } 
    else {
    // There was no /, so the input itself is the filename
    file_name = file_path;
    }

    size_t file_name_len = strlen(file_name) + 1;

    printf("File Path: %s, File Name: %s\n", file_path, file_name);

     if (!get_file(ctx, file_path)){
        return NULL;
     }

    uint8_t total_file_bytes[8]; // amount of bytes allocated for the file

    write_u64_be(total_file_bytes, ctx->file_buf_len);

    uint8_t name_len_bytes[4]; // amount of bytes for the name len field

    write_u32_be(name_len_bytes, file_name_len);

    // 8 bytes for file total byte amount field + 4 bytes for the name len field + x bytes for the file name + x bytes for the file
    *buf_length = sizeof(total_file_bytes) + sizeof(name_len_bytes) + file_name_len + ctx->file_buf_len;
    
    uint8_t *send_data = malloc(*buf_length);

    memcpy(send_data, total_file_bytes, sizeof(total_file_bytes)); // copy 8 byte field data
    memcpy(send_data + sizeof(total_file_bytes), name_len_bytes, sizeof(name_len_bytes)); // copy 4 byte field data
    memcpy(send_data + sizeof(total_file_bytes) + sizeof(name_len_bytes), file_name, file_name_len); // copy the file name
    memcpy(send_data + sizeof(total_file_bytes) + sizeof(name_len_bytes) + file_name_len, ctx->file_buf, ctx->file_buf_len); // copy the file

    return send_data;
}
