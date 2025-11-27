#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HEAP_START 0x40000000
#define HEAP_INITIAL_SIZE (100 * 4096) // 100 Pages

typedef struct BlockHeader {
    struct BlockHeader* next;
    size_t size;
    bool is_free;
} BlockHeader;

void heap_init();
void* malloc(size_t size);
void free(void* ptr);
