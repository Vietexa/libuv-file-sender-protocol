#include "file.h"
#include "app_ctx.h"
#include "utils.h"
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

    uint8_t mode = 1;

    uint8_t total_file_bytes[8]; // amount of bytes allocated for the file

    write_u64_be(total_file_bytes, ctx->file_buf_len);

    uint8_t name_len_bytes[4]; // amount of bytes for the name len field

    write_u32_be(name_len_bytes, file_name_len);

    // 1 byte for the mode + 8 bytes for file total byte amount field + 4 bytes for the name len field + x bytes for the file name + x bytes for the file
    *buf_length = sizeof(mode) + sizeof(total_file_bytes) + sizeof(name_len_bytes) + file_name_len + ctx->file_buf_len;
    
    uint8_t *send_data = malloc(*buf_length);

    memcpy(send_data, &mode, sizeof(mode)); // copy the 1 byte mode field
    memcpy(send_data + sizeof(mode), total_file_bytes, sizeof(total_file_bytes)); // copy 8 byte field data
    memcpy(send_data + sizeof(mode) + sizeof(total_file_bytes), name_len_bytes, sizeof(name_len_bytes)); // copy 4 byte field data
    memcpy(send_data + sizeof(mode) + sizeof(total_file_bytes) + sizeof(name_len_bytes), file_name, file_name_len); // copy the file name
    memcpy(send_data + sizeof(mode) + sizeof(total_file_bytes) + sizeof(name_len_bytes) + file_name_len, ctx->file_buf, ctx->file_buf_len); // copy the file

    return send_data;
}

bool select_mode(int *mode){
    printf("Enter what you'd like to do:\n1.Send file\n2.Request file\n");
    char mode_buf[4];
    fgets(mode_buf, sizeof(mode_buf), stdin);
    mode_buf[strcspn(mode_buf, "\n")] = '\0';

    printf("Mode: %s\n", mode_buf);

    int selected_mode = atoi(mode_buf);

    if (selected_mode == 1 || selected_mode == 2){
        *mode = selected_mode;
        return true;
    }
    else {
        fprintf(stderr, "Error: Invalid Mode!\n");
        return false;
    }

}

bool request_file(app_ctx_t *ctx, int *request_len){
    char file_name[255];
    printf("Enter the name of the file you want to request:\n");
    fgets(file_name,sizeof(file_name), stdin);

    file_name[strcspn(file_name, "\n")] = '\0';

    if (file_name[0] == '\0'){
        fprintf(stderr, "No name or path entered!\n");
        return false;
    }

    uint8_t mode = 2;

    uint8_t name_len_bytes[4];
    write_u32_be(name_len_bytes, strlen(file_name) + 1);

    *request_len = sizeof(mode) + sizeof(name_len_bytes) + strlen(file_name) + 1;

    ctx->receive_file.receive_buf = malloc(*request_len);

    memcpy(ctx->receive_file.receive_buf, &mode, sizeof(mode));
    memcpy(ctx->receive_file.receive_buf + sizeof(mode), name_len_bytes, sizeof(name_len_bytes));
    memcpy(ctx->receive_file.receive_buf + sizeof(mode) + sizeof(name_len_bytes), file_name, strlen(file_name) + 1);

    return true;

}
void parse_file(app_ctx_t *app_context, uv_stream_t *server, ssize_t nread, const uv_buf_t* buf){

    int bytes_offset = 0;

    if (app_context->network_file.header_length < 8){
            size_t needed_bytes = 8 - app_context->network_file.header_length;
            size_t bytes_to_copy = (size_t)nread - bytes_offset < needed_bytes ? nread - bytes_offset : needed_bytes;
            memcpy(app_context->network_file.header + app_context->network_file.header_length,
             buf->base + bytes_offset, bytes_to_copy);
            app_context->network_file.header_length += bytes_to_copy;
            bytes_offset += bytes_to_copy;
        }

        if (app_context->network_file.file_name_header_lenght < 4 &&
            app_context->network_file.header_length == 8){

            size_t needed_bytes = 4 - app_context->network_file.file_name_header_lenght;
            size_t bytes_to_copy = (size_t)nread - bytes_offset < needed_bytes ? nread - bytes_offset : needed_bytes;
            memcpy(app_context->network_file.file_name_header +
             app_context->network_file.file_name_header_lenght, buf->base + bytes_offset, bytes_to_copy);
            app_context->network_file.file_name_header_lenght += bytes_to_copy;
            bytes_offset += bytes_to_copy;

        }


        if (!app_context->network_file.file_capacity && app_context->network_file.header_length == 8 && app_context->network_file.file_name_header_lenght == 4) {
            app_context->network_file.file_capacity = read_u64_be(app_context->network_file.header);
            app_context->network_file.file = malloc(app_context->network_file.file_capacity);

            app_context->network_file.file_name_capacity = read_u32_be(app_context->network_file.file_name_header);
            // and our file_name is an array of char of 255 bytes so no need to allocate on the heap

         }

        if (!app_context->network_file.string_copied && app_context->network_file.header_length == 8 && app_context->network_file.file_name_header_lenght == 4){
            size_t needed_bytes = app_context->network_file.file_name_capacity - app_context->network_file.file_name_bytes_cp;
            size_t bytes_to_copy = (size_t)nread - bytes_offset < needed_bytes ? nread - bytes_offset : needed_bytes;

            memcpy(app_context->network_file.file_name + app_context->network_file.file_name_bytes_cp,
             buf->base + bytes_offset, bytes_to_copy);

            app_context->network_file.file_name_bytes_cp += bytes_to_copy;

            if (app_context->network_file.file_name_capacity == app_context->network_file.file_name_bytes_cp){
                app_context->network_file.string_copied = true;
            }
            bytes_offset += bytes_to_copy;
        }

        

        if (app_context->network_file.file_length < app_context->network_file.file_capacity &&
            app_context->network_file.header_length == 8 && app_context->network_file.file_name_header_lenght == 4 && app_context->network_file.string_copied){

            size_t remaining = app_context->network_file.file_capacity - app_context->network_file.file_length;
            size_t copy_amount = (size_t)nread - bytes_offset < remaining ? nread - bytes_offset : remaining;


            memcpy(app_context->network_file.file + app_context->network_file.file_length, buf->base + bytes_offset, copy_amount);
            app_context->network_file.file_length += copy_amount;
        }

        if ( app_context->network_file.file_capacity > 0 && app_context->network_file.file_length == app_context->network_file.file_capacity &&  app_context->network_file.header_length == 8 && 
            app_context->network_file.file_name_header_lenght == 4 && app_context->network_file.string_copied ){
            
            printf("File name: %s\n", app_context->network_file.file_name);

            FILE *file = fopen(app_context->network_file.file_name, "wb");

            if (!file){
                perror("fopen");
                return;
            }

            size_t written = fwrite(app_context->network_file.file, 1, app_context->network_file.file_length, file);

            if (written != app_context->network_file.file_length) {
                perror("fwrite");
                fclose(file);
                free(buf->base);
                return;
            }

            fclose(file);

            free(app_context->network_file.file);
            app_context->network_file.file = NULL;
            app_context->network_file.file_length = 0;
            app_context->network_file.file_capacity = 0;
            app_context->network_file.header_length = 0;
            app_context->network_file.file_name_capacity = 0;
            app_context->network_file.file_name_bytes_cp = 0;
            app_context->network_file.string_copied = false;
            app_context->network_file.file_name_header_lenght = 0;

        }

    }
