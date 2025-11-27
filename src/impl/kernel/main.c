#include "print.h"
#include "idt.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
#include "shell.h"
#include "fat.h"

void kernel_main(uint64_t magic, uint64_t multiboot_addr) {
    print_clear();
    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    printf("Welcome to my 64-bit OS!\nInitialization complete.\n");

    printf("Magic: %p\nAddr: %p\n", (void*)magic, (void*)multiboot_addr);

    idt_init();
    printf("IDT Initialized.\n");

    // --- DEBUG & FIX INITIAL PAGE TABLES ---
    uint64_t cr3 = read_cr3();
    uint64_t* pml4_ptr = (uint64_t*)cr3;
    
    printf("Initial PML4[0]: %x\n", pml4_ptr[0]);
    if (!(pml4_ptr[0] & 2)) {
        printf("PML4[0] was Read-Only! Fixing...\n");
        pml4_ptr[0] |= 2; // Add Write Bit
    }

    uint64_t* pdp_ptr = (uint64_t*)(pml4_ptr[0] & 0x000FFFFFFFFFF000);
    printf("Initial PDP[0]: %x\n", pdp_ptr[0]);
    if (!(pdp_ptr[0] & 2)) {
        printf("PDP[0] was Read-Only! Fixing...\n");
        pdp_ptr[0] |= 2; // Add Write Bit
    }
    
    // Apply changes
    load_cr3(cr3);
    printf("Page Tables Patched.\n");
    // ---------------------------------------

    pmm_init(multiboot_addr);

    printf("PMM Initialized.\n");

    printf("--- PROTECTING BOOT TABLES ---\n");
    uint64_t boot_cr3 = read_cr3();
    
    // Protect PML4
    pmm_lock_page((void*)boot_cr3);
    printf("Locked Boot PML4: %p\n", (void*)boot_cr3);

    // Protect PDP
    uint64_t* boot_pml4 = (uint64_t*)boot_cr3;
    uint64_t boot_pdp_addr = boot_pml4[0] & 0x000FFFFFFFFFF000;
    pmm_lock_page((void*)boot_pdp_addr);
    printf("Locked Boot PDP: %p\n", (void*)boot_pdp_addr);

    // Protect PD
    uint64_t* boot_pdp = (uint64_t*)boot_pdp_addr;
    uint64_t boot_pd_addr = boot_pdp[0] & 0x000FFFFFFFFFF000;
    pmm_lock_page((void*)boot_pd_addr);
    printf("Locked Boot PD: %p\n", (void*)boot_pd_addr);

    printf("--- BOOT TABLES PROTECTED ---\n");

    // VMM Test
    uint64_t phys_addr = 0xb8000; // VGA Buffer
    uint64_t virt_addr = 0xdeadbeef000; // Some random high address
    uint64_t* pml4 = (uint64_t*)read_cr3();

    printf("Mapping 0x%p to VGA physical 0x%x...\n", (void*)virt_addr, phys_addr);
    vmm_map(pml4, phys_addr, virt_addr, PTE_PRESENT | PTE_WRITABLE);

    // Now try to write using the Virtual Address
    char* vga = (char*)virt_addr;
    vga[0] = 'X'; // Should appear at the top-left of the screen
    vga[1] = 0x4F; // Red background, White text

    printf("Wrote 'X' to virtual address 0x%p. Check screen top-left.\n", (void*)virt_addr);

    // --- HEAP TEST ---
    heap_init();
    
    // Initialize FAT
    fat_init();

    // Start Shell
    printf("Enabling Interrupts...\n");
    asm volatile("sti"); // Set Interrupt Flag
    shell_init();
    // -----------------

    while(1);
}
