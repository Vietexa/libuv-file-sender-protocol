#pragma once
#include "app_ctx.h"
#include <stdbool.h>

bool get_file(app_ctx_t *ctx, char *file_name);

void write_u64_be(uint8_t *buf, uint64_t value);
uint64_t read_u64_be(const uint8_t *buf);
uint8_t *prepare_buffer(app_ctx_t *ctx, uint64_t *buf_length);


