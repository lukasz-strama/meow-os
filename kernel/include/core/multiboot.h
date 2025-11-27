#pragma once
#include <stdint.h>

struct MultibootTag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct MultibootMmapEntry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

struct MultibootMmapTag {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    struct MultibootMmapEntry entries[];
} __attribute__((packed));

#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_MEMORY_AVAILABLE 1
