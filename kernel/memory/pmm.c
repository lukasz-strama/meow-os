#include "memory/pmm.h"
#include "core/multiboot.h"
#include "drivers/print.h"

extern char _kernel_end[];

uint8_t* bitmap;
uint64_t total_memory = 0;
uint64_t bitmap_size = 0;
static uint64_t max_pages = 0;

void* my_memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

void bit_set(uint64_t bit) {
    if (bit >= max_pages) return;
    bitmap[bit / 8] |= (1 << (bit % 8));
}

void bit_unset(uint64_t bit) {
    if (bit >= max_pages) return;
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

int bit_test(uint64_t bit) {
    if (bit >= max_pages) return 0;
    return bitmap[bit / 8] & (1 << (bit % 8));
}

void pmm_set_bit(uint64_t page_index) {
    if (page_index >= max_pages) return;
    uint64_t byte_idx = page_index / 8;
    uint8_t bit_idx = page_index % 8;
    bitmap[byte_idx] |= (1 << bit_idx);
}

void pmm_unset_bit(uint64_t page_index) {
    if (page_index >= max_pages) return;
    uint64_t byte_idx = page_index / 8;
    uint8_t bit_idx = page_index % 8;
    bitmap[byte_idx] &= ~(1 << bit_idx);
}

int pmm_test_bit(uint64_t page_index) {
    if (page_index >= max_pages) return 0;
    uint64_t byte_idx = page_index / 8;
    uint8_t bit_idx = page_index % 8;
    return bitmap[byte_idx] & (1 << bit_idx);
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
    
    // Align bitmap to next page boundary
    // Add a 64KB (0x10000) safety gap to skip over Bootloader Page Tables
    // which are likely located immediately after the kernel image.
    uint64_t end_addr = (uint64_t)_kernel_end;
    uint64_t bitmap_addr = (end_addr + 0x10000 + 4095) & ~((uint64_t)4095);
    bitmap = (uint8_t*)bitmap_addr;

    max_pages = bitmap_size * 8; // Ensure we never go beyond the allocated array

    print_str("--- DEBUG INFO ---\n");
    print_str("Total RAM: ");
    print_hex(total_memory);
    print_str("\n");

    print_str("Bitmap Size: ");
    print_hex(bitmap_size);
    print_str("\n");

    printf("Kernel End: %p\n", _kernel_end);
    printf("Bitmap Ptr: %p (with safety gap)\n", bitmap);

    print_str("PMM: Memset Start...\n");
    my_memset(bitmap, 0xFF, bitmap_size);
    print_str("PMM: Memset Done. Starting loops...\n");

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
                        pmm_unset_bit(addr / PAGE_SIZE);
                    }
                }
            }
        }
        tag = (struct MultibootTag*)((uint8_t*)tag + ((tag->size + 7) & ~7));
    }

    print_str("PMM: RAM Unlocked. Locking Kernel...\n");

    // Step D: Protect Critical Regions
    
    // 1. Protect Kernel and Bitmap
    // We lock from 0 up to the end of the bitmap
    uint64_t bitmap_end_addr = (uint64_t)bitmap + bitmap_size;
    uint64_t pages_to_lock = (bitmap_end_addr + PAGE_SIZE - 1) / PAGE_SIZE; // Ceiling division

    for (uint64_t i = 0; i < pages_to_lock; i++) {
        pmm_set_bit(i);
    }

    // 2. Protect Multiboot Info Structure
    uint32_t mb_size = *(uint32_t*)multiboot_addr;
    uint64_t mb_start_page = multiboot_addr / PAGE_SIZE;
    uint64_t mb_end_page = (multiboot_addr + mb_size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t i = mb_start_page; i < mb_end_page; i++) {
        pmm_set_bit(i);
    }
    
    print_str("PMM: Init Finished.\n");
    print_str("PMM Initialized. Total Memory: ");
    print_hex(total_memory);
    print_str("\n");
}

void* pmm_alloc_page() {
    for (uint64_t i = 0; i < max_pages; i++) {
        if (!pmm_test_bit(i)) {
            pmm_set_bit(i);
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free_page(void* addr) {
    uint64_t index = (uint64_t)addr / PAGE_SIZE;
    pmm_unset_bit(index);
}

void pmm_lock_page(void* addr) {
    uint64_t index = (uint64_t)addr / PAGE_SIZE;
    pmm_set_bit(index);
}
