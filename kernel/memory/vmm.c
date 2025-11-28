#include "memory/vmm.h"
#include "memory/pmm.h"
#include "drivers/print.h"

uint64_t read_cr3() {
    uint64_t value;
    asm volatile("mov %%cr3, %0" : "=r" (value));
    return value & 0x000FFFFFFFFFF000;
}

void load_cr3(uint64_t val) {
    asm volatile("mov %0, %%cr3" :: "r"(val));
}

void vmm_flush_tlb(uint64_t addr) {
    asm volatile("invlpg (%0)" :: "r" (addr) : "memory");
}

static void vmm_memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
}

int vmm_map(uint64_t* pml4, uint64_t phys, uint64_t virt, uint64_t flags) {
    // Disable interrupts to prevent interference
    // asm volatile("cli");

    uint64_t idx4 = (virt >> 39) & 0x1FF;
    uint64_t idx3 = (virt >> 30) & 0x1FF;
    uint64_t idx2 = (virt >> 21) & 0x1FF;
    uint64_t idx1 = (virt >> 12) & 0x1FF;

    // --- LEVEL 4 ---
    if (!(pml4[idx4] & PTE_PRESENT)) {
        // Step 1: Alloc
        void* new_ptr = pmm_alloc_page();
        if (!new_ptr) { 
            printf("VMM: OOM allocating PDP\n"); 
            return 1; 
        }
        uint64_t new_pdp = (uint64_t)new_ptr;

        // Step 2: Clear
        uint64_t* ptr = (uint64_t*)new_pdp;
        for(int i = 0; i < 512; i++) {
            ptr[i] = 0;
        }

        // Step 3: Write to PML4
        pml4[idx4] = new_pdp | PTE_PRESENT | PTE_WRITABLE | PTE_USER; // Allow user access to tables? Usually needed for traversal
    }
    // CRITICAL: Mask flags before casting to pointer!
    uint64_t* pdp = (uint64_t*)PTE_ADDR(pml4[idx4]);

    // --- LEVEL 3 ---
    if (!(pdp[idx3] & PTE_PRESENT)) {
        void* new_ptr = pmm_alloc_page();
        if (!new_ptr) { 
            printf("VMM: OOM allocating PD\n"); 
            return 1; 
        }
        uint64_t new_pd = (uint64_t)new_ptr;

        uint64_t* ptr = (uint64_t*)new_pd;
        for(int i = 0; i < 512; i++) {
            ptr[i] = 0;
        }

        pdp[idx3] = new_pd | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    uint64_t* pd = (uint64_t*)PTE_ADDR(pdp[idx3]);

    // --- LEVEL 2 ---
    if (!(pd[idx2] & PTE_PRESENT)) {
        void* new_ptr = pmm_alloc_page();
        if (!new_ptr) { 
            printf("VMM: OOM allocating PT\n"); 
            return 1; 
        }
        uint64_t new_pt = (uint64_t)new_ptr;

        uint64_t* ptr = (uint64_t*)new_pt;
        for(int i = 0; i < 512; i++) {
            ptr[i] = 0;
        }

        pd[idx2] = new_pt | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    }
    uint64_t* pt = (uint64_t*)PTE_ADDR(pd[idx2]);

    // --- LEVEL 1 ---
    pt[idx1] = phys | flags;

    // Invalidate TLB for this address
    vmm_flush_tlb(virt);
    return 0;
}
