#pragma once
#include "data_structures.h"

void process_file(app_ctx_t *app_context, uv_stream_t *client, const uv_buf_t *buf, ssize_t nread);