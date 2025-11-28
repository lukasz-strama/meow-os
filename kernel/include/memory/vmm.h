#pragma once
#include <stdint.h>

#define PTE_PRESENT 1
#define PTE_WRITABLE 2
#define PTE_USER 4
#define PTE_NX 0x8000000000000000

#define PTE_ADDR(entry) (entry & 0x000FFFFFFFFFF000ULL)
#define PTE_FLAGS(entry) (entry & 0xFFF0000000000FFFULL)

int vmm_map(uint64_t* pml4, uint64_t phys, uint64_t virt, uint64_t flags);
uint64_t read_cr3();
void load_cr3(uint64_t val);
