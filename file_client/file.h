#pragma once

#include "app_ctx.h"
#include <stdbool.h>
#include "stdint.h"

bool get_file(app_ctx_t *ctx, char *file_name);
uint8_t *prepare_buffer(app_ctx_t *ctx, uint64_t *buf_length);
bool select_mode(int *mode);
bool request_file(app_ctx_t *ctx, int *request_len);
void parse_file(app_ctx_t *app_context, uv_stream_t *server, ssize_t nread, const uv_buf_t* buf);