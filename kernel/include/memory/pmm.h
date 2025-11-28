#pragma once
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

#include "core/stats.h"

void pmm_init(uint64_t multiboot_addr);
void* pmm_alloc_page();
void pmm_free_page(void* addr);
void pmm_lock_page(void* addr);
void pmm_get_info(MemInfo* info);
