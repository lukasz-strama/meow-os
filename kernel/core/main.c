#include "drivers/print.h"
#include "core/idt.h"
#include "core/gdt.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "kmonitor/kmonitor.h"
#include "fs/fat.h"
#include "core/syscall.h"
#include "core/process.h"
#include "core/loader.h"

// Helper to write char to (x,y)
void safe_print(int x, int y, char c, uint8_t color) {
    uint16_t* vga = (uint16_t*)0xB8000;
    vga[y * 80 + x] = (uint16_t)c | ((uint16_t)color << 8);
}

void blinker_task() { 
    while(1) { 
        // Write blinking '!' at top right corner (Offset 79)
        safe_print(79, 0, '!', 0x4E); // Yellow on Red '!'
        for(volatile int d=0; d<5000000; d++); // Delay
        safe_print(79, 0, ' ', 0x07); // Clear
        for(volatile int d=0; d<5000000; d++); 
    } 
}

void kernel_main(uint64_t magic, uint64_t multiboot_addr) {
    print_clear();
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    printf("Welcome to my 64-bit OS!\nInitialization complete.\n");

    // gdt_init();
    void fix_gdt();
    fix_gdt();
    printf("GDT initialized.\n");

    printf("Magic: %p\nAddr: %p\n", (void*)magic, (void*)multiboot_addr);

    idt_init();
    printf("IDT Initialized.\n");
    
    syscall_init();

    // --- DEBUG & FIX INITIAL PAGE TABLES ---
    uint64_t cr3 = read_cr3();
    uint64_t* pml4_ptr = (uint64_t*)cr3;
    
    printf("Initial PML4[0]: %x\n", pml4_ptr[0]);
    // FORCE USER BIT (Bit 2) on PML4
    // 0x07 = Present(1) | Write(2) | User(4)
    if ((pml4_ptr[0] & 0x07) != 0x07) {
        printf("Patching PML4[0] flags to 0x07 (User+RW)...\n");
        pml4_ptr[0] |= 0x07; 
    }

    uint64_t* pdp_ptr = (uint64_t*)(pml4_ptr[0] & 0x000FFFFFFFFFF000);
    printf("Initial PDP[0]: %x\n", pdp_ptr[0]);
    // FORCE USER BIT (Bit 2) on PDP
    if ((pdp_ptr[0] & 0x07) != 0x07) {
        printf("Patching PDP[0] flags to 0x07 (User+RW)...\n");
        pdp_ptr[0] |= 0x07;
    }

    // 3. FORCE USER BIT on the first 512 entries of PD (First 1GB)
    uint64_t* pd_ptr = (uint64_t*)(pdp_ptr[0] & 0x000FFFFFFFFFF000);
    printf("Unlocking first 1GB for User Mode...\n");
    for (int i = 0; i < 512; i++) {
        // If the page is present, OR it with 0x07 (User+RW+Present)
        if (pd_ptr[i] & 1) {
            pd_ptr[i] |= 0x07;
        }
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

    printf("Multitasking Test: Look at top-right corner!\n");

    scheduler_init();
    process_create(blinker_task);

    // Start Shell
    printf("Enabling Interrupts & Starting Login...\n");
    asm volatile("sti"); // Set Interrupt Flag
    
    if (program_load("LOGIN.BIN") != 0) {
        printf("Failed to load LOGIN.BIN! Falling back to KMonitor.\n");
        kmonitor_init();
    }
    // -----------------

    while(1) {
        asm volatile("hlt"); // Save power, wait for interrupt
    }
}
