#include "print.h"
#include "idt.h"
#include "pmm.h"

void kernel_main(uint64_t magic, uint64_t multiboot_addr) {
    print_clear();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    printf("Welcome to my 64-bit OS!\nInitialization complete.\n");

    printf("Magic: %p\nAddr: %p\n", (void*)magic, (void*)multiboot_addr);

    printf("--- PRINTF TEST ---\n");
    printf("Char: %c, String: %s\n", 'A', "Hello World");
    printf("Dec: %d, Neg: %d\n", 123, -456);
    printf("Hex: %x, Ptr: %p\n", 0xABCD, (void*)0x123456789);

    pmm_init(multiboot_addr);

    printf("PMM Initialized.\n");

    // Test 1: Allocate first available page
    void* p1 = pmm_alloc_page();
    printf("Allocated P1: %p\n", p1);

    // Test 2: Allocate second page
    void* p2 = pmm_alloc_page();
    printf("Allocated P2: %p\n", p2);

    // Test 3: Free P1
    pmm_free_page(p1);
    printf("Freed P1.\n");

    // Test 4: Allocate again (Should get P1 address back)
    void* p3 = pmm_alloc_page();
    printf("Allocated P3: %p\n", p3);

    while(1);
}
