#pragma once

#include <stdbool.h>
#include <stdint.h>

void write_u64_be(uint8_t *buf, uint64_t value);
uint64_t read_u64_be(const uint8_t *buf);
void write_u32_be(uint8_t *buf, uint32_t value);
uint32_t read_u32_be(const uint8_t *buf);
bool parse_json_file(const char *file_name, char *array, int size);




