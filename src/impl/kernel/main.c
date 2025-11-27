#include "print.h"
#include "idt.h"
#include "pmm.h"

void kernel_main(uint64_t magic, uint64_t multiboot_addr) {
    print_clear();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_str("Welcome to my 64-bit OS!\nInitialization complete.\n");

    print_str("Magic: ");
    print_hex(magic);
    print_str("\nAddr: ");
    print_hex(multiboot_addr);
    print_str("\n");

    pmm_init(multiboot_addr);

    print_str("PMM Initialized.\n");

    // Test 1: Allocate first available page
    void* p1 = pmm_alloc_page();
    print_str("Alloc P1: "); print_hex((uint64_t)p1); print_str("\n");

    // Test 2: Allocate second page
    void* p2 = pmm_alloc_page();
    print_str("Alloc P2: "); print_hex((uint64_t)p2); print_str("\n");

    // Test 3: Free P1
    pmm_free_page(p1);
    print_str("Freed P1.\n");

    // Test 4: Allocate again (Should get P1 address back)
    void* p3 = pmm_alloc_page();
    print_str("Alloc P3: "); print_hex((uint64_t)p3); print_str("\n");

    while(1);
}
