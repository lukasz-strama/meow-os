#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "print.h"

BlockHeader* start_tag = NULL;

void heap_init() {
    uint64_t* pml4 = (uint64_t*)read_cr3();
    
    printf("HEAP: Initializing at %p with size %d bytes\n", (void*)HEAP_START, HEAP_INITIAL_SIZE);

    for (uint64_t i = 0; i < HEAP_INITIAL_SIZE; i += PAGE_SIZE) {
        void* phys = pmm_alloc_page();
        if (!phys) {
            printf("HEAP: OOM during init!\n");
            return;
        }
        vmm_map(pml4, (uint64_t)phys, HEAP_START + i, PTE_PRESENT | PTE_WRITABLE);
    }

    start_tag = (BlockHeader*)HEAP_START;
    start_tag->size = HEAP_INITIAL_SIZE - sizeof(BlockHeader);
    start_tag->is_free = true;
    start_tag->next = NULL;

    printf("HEAP: Initialized. First block size: %d\n", start_tag->size);
}

void* malloc(size_t size) {
    BlockHeader* current = start_tag;
    
    while (current) {
        if (current->is_free && current->size >= size) {
            // Found a fit
            if (current->size > size + sizeof(BlockHeader) + 16) {
                // Split
                BlockHeader* new_block = (BlockHeader*)((uint64_t)current + sizeof(BlockHeader) + size);
                new_block->size = current->size - size - sizeof(BlockHeader);
                new_block->is_free = true;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = false;
            return (void*)(current + 1);
        }
        current = current->next;
    }
    
    printf("HEAP: OOM in malloc for size %d\n", size);
    return NULL;
}

void free(void* ptr) {
    if (!ptr) return;

    BlockHeader* header = (BlockHeader*)ptr - 1;
    header->is_free = true;

    // Coalesce
    BlockHeader* current = start_tag;
    while (current) {
        if (current->is_free && current->next && current->next->is_free) {
            current->size += sizeof(BlockHeader) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}
