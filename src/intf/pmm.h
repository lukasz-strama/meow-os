#pragma once
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

void pmm_init(uint64_t multiboot_addr);
void* pmm_alloc_page();
void pmm_free_page(void* addr);
