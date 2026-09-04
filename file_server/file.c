#include "include/file.h"
#include "include/data_structures.h"
#include "include/utils.h"
#include "uv.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_resources_mode_1(app_ctx_t *app_context){

            free(app_context->network_file.file);
            app_context->network_file.file = NULL;
            app_context->network_file.file_length = 0;
            app_context->network_file.file_capacity = 0;
            app_context->network_file.header_length = 0;
            app_context->network_file.file_name_capacity = 0;
            app_context->network_file.file_name_bytes_cp = 0;
            app_context->network_file.string_copied = false;
            app_context->network_file.file_name_header_lenght = 0;
            app_context->selected_mode = 0;
}

void on_write_end(uv_write_t *req, int status);

static bool double_file_buf_size(app_ctx_t *ctx){

uint64_t old_capacity = ctx->file_send.file_buf_capacity;
uint64_t new_capacity = ctx->file_send.file_buf_capacity * 2;

if (ctx->file_send.file_buf_len < old_capacity) return false; // don't reallocate it yet

uint8_t *tmp = realloc(ctx->file_send.file_buf, new_capacity);

if (!tmp){
    fprintf(stderr, "Couldn't reallocate!\n");
    perror("realloc");
    return false;
}

ctx->file_send.file_buf = tmp;
ctx->file_send.file_buf_capacity = new_capacity;

return true;

}

bool get_file(app_ctx_t *ctx, char *file_name){

ctx->file_send.file_buf_len = 0;

FILE *file_to_send = fopen(file_name, "rb");

if (!file_to_send){
    perror("fopen");
    return false;
}
printf("get_file: Opened the file\n");
while (1){

    if (ctx->file_send.file_buf_len >= ctx->file_send.file_buf_capacity){
        bool status = double_file_buf_size(ctx);

        if (status == false){
            fclose(file_to_send);
            return false;
       } 
    } 
    size_t available = ctx->file_send.file_buf_capacity - ctx->file_send.file_buf_len;
    size_t bytes_read = fread(ctx->file_send.file_buf + ctx->file_send.file_buf_len,sizeof(uint8_t),available , file_to_send);
    ctx->file_send.file_buf_len += bytes_read;

    printf("get_file: fread returned %zu\n", bytes_read);

    if (bytes_read < available){

        if (feof(file_to_send)) {
           fclose(file_to_send);
           printf("get_file: Successfully reached EOF\n");
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

static void send_err_mode_3(int err_num, app_ctx_t *app_context, uv_stream_t *client){
    printf("send_err_mode_3 triggered: preparing to send err to client\n");
    char error[400];
    snprintf(error, sizeof(error), "fopen failed: %s", strerror(err_num));

    uint8_t mode = 3;

    uint8_t error_header[4];

    int error_len = strlen(error) + 1;
    write_u32_be(error_header, error_len);

    size_t buf_length = sizeof(mode) + sizeof(error_header) + error_len;
    
    uint8_t *send_data = malloc(buf_length);

    memcpy(send_data, &mode, sizeof(mode)); // copy the 1 byte mode field
    memcpy(send_data + sizeof(mode), error_header, sizeof(error_header)); // copy 4 bytes from the error header
    memcpy(send_data + sizeof(mode) + sizeof(error_header), error, error_len); // copy the bytes of the error
    
    uv_buf_t send_buffer = uv_buf_init((char *)send_data, buf_length);

    uv_write_t *write_req = malloc(sizeof(*write_req));
    write_req->data = send_data;

    printf("From send_err_mode_3: About to uv_write %zu bytes\n", buf_length);
    uv_write(write_req, client, &send_buffer, 1, on_write_end);

}

void process_file(app_ctx_t *app_context, uv_stream_t *client, const uv_buf_t *buf, ssize_t nread){
    size_t bytes_offset = 0;

    if (!app_context->selected_mode){
        app_context->selected_mode = buf->base[bytes_offset];
        printf("Mode: %d\n", app_context->selected_mode);

        if (app_context->selected_mode != 1 && app_context->selected_mode != 2){
            fprintf(stderr, "Fatal Error: Invalid mode!\n");
            app_context->process_fatal_error = true;
            return;
        }
        bytes_offset += 1;
    }

    if (app_context->selected_mode == 1){ // Client mode: Send file

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
            bytes_offset += copy_amount;
        }

        if ( app_context->network_file.file_capacity > 0 && app_context->network_file.file_length == app_context->network_file.file_capacity &&  app_context->network_file.header_length == 8 && 
            app_context->network_file.file_name_header_lenght == 4 && app_context->network_file.string_copied ){
            
            printf("File name: %s\n", app_context->network_file.file_name);

            FILE *file = fopen(app_context->network_file.file_name, "wb");

            if (!file){
                perror("fopen");
                free_resources_mode_1(app_context);
                return;
            }

            size_t written = fwrite(app_context->network_file.file, 1, app_context->network_file.file_length, file);

            if (written != app_context->network_file.file_length) {
                perror("fwrite");
                fclose(file);
                free_resources_mode_1(app_context);
                return;
            }

            fclose(file);
            free_resources_mode_1(app_context);

        }
    }

    else if (app_context->selected_mode == 2){ // Client mode: Request file

        
         if (app_context->file_req.f_n_header_length < 4){
            size_t needed_bytes = 4 - app_context->file_req.f_n_header_length;
            size_t bytes_to_copy = (size_t)nread - bytes_offset < needed_bytes ? nread - bytes_offset : needed_bytes;

            memcpy(app_context->file_req.file_name_header + app_context->file_req.f_n_header_length,
             buf->base + bytes_offset, bytes_to_copy);

            app_context->file_req.f_n_header_length += bytes_to_copy;
            bytes_offset += bytes_to_copy;
        }

        if (app_context->file_req.f_n_header_length == 4){
            app_context->file_req.f_n_header_len_conv = read_u32_be(app_context->file_req.file_name_header);
            size_t needed_bytes = app_context->file_req.f_n_header_len_conv - app_context->file_req.file_name_length;
            size_t bytes_to_copy = (size_t)nread - bytes_offset < needed_bytes ? nread - bytes_offset : needed_bytes;

            if (app_context->file_req.file_name == NULL){
            app_context->file_req.file_name = malloc(app_context->file_req.f_n_header_len_conv);
            }

            memcpy(app_context->file_req.file_name + app_context->file_req.file_name_length, 
                buf->base + bytes_offset,bytes_to_copy);

            app_context->file_req.file_name_length += bytes_to_copy;

            bytes_offset += bytes_to_copy;
        }

        printf("header bytes: %zu/4\n", app_context->file_req.f_n_header_length);

        printf("expected filename length: %zu\n", app_context->file_req.f_n_header_len_conv);

        printf("received filename length: %zu\n", app_context->file_req.file_name_length);

        printf("nread: %zd, bytes_offset: %zu\n", nread, bytes_offset);

        if (app_context->file_req.f_n_header_length == 4 &&
             app_context->file_req.f_n_header_len_conv == app_context->file_req.file_name_length){

            char file_name[255];
            memcpy(file_name, app_context->file_req.file_name,
                 app_context->file_req.f_n_header_len_conv);
            size_t file_name_len = strlen(file_name) + 1;

            printf("Preparing to look for file %s\n", file_name);

            if(!get_file(app_context, file_name)){
                fprintf(stderr, "There was an error getting the file!\n");
                send_err_mode_3(errno, app_context, client);
                free(app_context->file_req.file_name);
                app_context->file_req.file_name = NULL;
                app_context->file_req.file_name_length = 0;
                app_context->file_req.f_n_header_len_conv = 0;
                app_context->file_req.f_n_header_length = 0;
                app_context->file_send.file_buf_len = 0;
                app_context->selected_mode = 0;
                return;
            }

            printf("Finished parsing the file\n");
            printf("file_buf_len = %zu\n", app_context->file_send.file_buf_len);
            printf("file_buf_capacity = %zu\n", app_context->file_send.file_buf_capacity);

            
            uint8_t mode = 2;

            uint8_t file_header[8];
            write_u64_be(file_header, app_context->file_send.file_buf_len);

            uint8_t file_name_header[4];
            write_u32_be(file_name_header, strlen(file_name) + 1);

            size_t buf_length = sizeof(mode) + sizeof(file_header) + sizeof(file_name_header) + file_name_len + app_context->file_send.file_buf_len;

            uint8_t *send_data = malloc(buf_length);

            memcpy(send_data, &mode, sizeof(mode)); // copy the 1 byte mode field
            memcpy(send_data + sizeof(mode), file_header, sizeof(file_header)); // copy 8 byte field data
            memcpy(send_data + sizeof(mode) + sizeof(file_header), file_name_header, sizeof(file_name_header)); // copy 4 byte field data
            memcpy(send_data + sizeof(mode) + sizeof(file_header) + sizeof(file_name_header), file_name, file_name_len); // copy the file name
            memcpy(send_data + sizeof(mode) + sizeof(file_header) + sizeof(file_name_header) + file_name_len, app_context->file_send.file_buf, app_context->file_send.file_buf_len); // copy the file

            uv_buf_t send_buffer = uv_buf_init((char *)send_data, buf_length);

            uv_write_t *write_req = malloc(sizeof(*write_req));
            write_req->data = send_data;

            printf("About to uv_write %zu bytes\n", buf_length);
            uv_write(write_req, client, &send_buffer, 1, on_write_end);
            
        }
    }

}