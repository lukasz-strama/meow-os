#include "pmm.h"
#include "multiboot.h"
#include "print.h"

extern char _kernel_end[];

uint8_t* bitmap;
uint64_t total_memory = 0;
uint64_t bitmap_size = 0;

void* my_memset(void* ptr, int value, size_t num) {
    unsigned char* p = ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

void bit_set(uint64_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

void bit_unset(uint64_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int bit_test(uint64_t bit) {
    return bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_init(uint64_t multiboot_addr) {
    struct MultibootTag* tag;
    
    // Step A: Calculate Size
    // We need to find the highest available physical address to determine total_memory
    tag = (struct MultibootTag*)(multiboot_addr + 8);
    while (tag->type != 0) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            struct MultibootMmapTag* mmap = (struct MultibootMmapTag*)tag;
            uint32_t entries_len = mmap->size - sizeof(struct MultibootMmapTag);
            uint32_t num_entries = entries_len / mmap->entry_size;

            for (uint32_t i = 0; i < num_entries; i++) {
                struct MultibootMmapEntry* entry = (struct MultibootMmapEntry*)((uint64_t)mmap->entries + (i * mmap->entry_size));
                
                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                    uint64_t top = entry->addr + entry->len;
                    if (top > total_memory) {
                        total_memory = top;
                    }
                }
            }
        }
        tag = (struct MultibootTag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }

    // Step B: Place Bitmap
    bitmap_size = (total_memory / PAGE_SIZE) / 8;
    bitmap = (uint8_t*)_kernel_end; // Place bitmap after kernel

    print_str("Kernel End: ");
    print_hex((uint64_t)_kernel_end);
    print_str("\n");

    print_str("Bitmap Addr: ");
    print_hex((uint64_t)bitmap);
    print_str("\n");

    print_str("Bitmap Size: ");
    print_hex(bitmap_size);
    print_str("\n");

    // Wait here to verify addresses
    while(1);

    // Initialize bitmap to all used (1) - Safe default
    my_memset(bitmap, 0xFF, bitmap_size);

    // Step C: Free Available Memory
    tag = (struct MultibootTag*)(multiboot_addr + 8);
    while (tag->type != 0) {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
            struct MultibootMmapTag* mmap = (struct MultibootMmapTag*)tag;
            uint32_t entries_len = mmap->size - sizeof(struct MultibootMmapTag);
            uint32_t num_entries = entries_len / mmap->entry_size;

            for (uint32_t i = 0; i < num_entries; i++) {
                struct MultibootMmapEntry* entry = (struct MultibootMmapEntry*)((uint64_t)mmap->entries + (i * mmap->entry_size));
                
                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
                    for (uint64_t addr = entry->addr; addr < entry->addr + entry->len; addr += PAGE_SIZE) {
                        // Ensure we don't go out of bounds of our bitmap
                        if (addr < total_memory) {
                            bit_unset(addr / PAGE_SIZE);
                        }
                    }
                }
            }
        }
        tag = (struct MultibootTag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }

    // Step D: Protect Critical Regions
    
    // 1. Protect Kernel and Bitmap
    // We lock from 0 up to the end of the bitmap
    uint64_t bitmap_end_addr = (uint64_t)bitmap + bitmap_size;
    uint64_t pages_to_lock = (bitmap_end_addr + PAGE_SIZE - 1) / PAGE_SIZE; // Ceiling division

    for (uint64_t i = 0; i < pages_to_lock; i++) {
        bit_set(i);
    }

    // 2. Protect Multiboot Info Structure
    uint32_t mb_size = *(uint32_t*)multiboot_addr;
    uint64_t mb_start_page = multiboot_addr / PAGE_SIZE;
    uint64_t mb_end_page = (multiboot_addr + mb_size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t i = mb_start_page; i < mb_end_page; i++) {
        bit_set(i);
    }
    
    print_str("PMM Initialized. Total Memory: ");
    print_hex(total_memory);
    print_str("\n");
}

void* pmm_alloc_page() {
    for (uint64_t i = 0; i < total_memory / PAGE_SIZE; i++) {
        if (!bit_test(i)) {
            bit_set(i);
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free_page(void* addr) {
    uint64_t index = (uint64_t)addr / PAGE_SIZE;
    bit_unset(index);
}
