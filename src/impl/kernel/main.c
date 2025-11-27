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

    void* p1 = pmm_alloc_page();
    print_str("Allocated P1: "); print_hex((uint64_t)p1); print_str("\n");

    void* p2 = pmm_alloc_page();
    print_str("Allocated P2: "); print_hex((uint64_t)p2); print_str("\n");

    pmm_free_page(p1);
    print_str("Freed P1\n");

    void* p3 = pmm_alloc_page();
    print_str("Allocated P3 (should be P1): "); print_hex((uint64_t)p3); print_str("\n");

    while(1);
}
