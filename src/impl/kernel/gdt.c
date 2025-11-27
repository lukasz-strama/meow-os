#include "gdt.h"
#include "print.h"

GDTEntry gdt[7];
TSS tss;
GDTDescriptor gdtr;

static uint8_t stack[4096]; // Temporary kernel stack for interrupts

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void gdt_set_tss_gate(int num, uint64_t base, uint32_t limit) {
    // Low 8 bytes (Standard Descriptor)
    // Limit 15:0, Base 15:0, Base 23:16, Type, S, DPL, P, Limit 19:16, AVL, L, G, Base 31:24
    // Type for 64-bit TSS (Available) is 0x9 (1001).
    // S (Descriptor Type) = 0 (System).
    // DPL = 0.
    // P = 1.
    // So Access byte = 1000 1001 = 0x89.
    gdt_set_gate(num, (uint32_t)base, limit, 0x89, 0x00); 

    // High 8 bytes
    // Base 63:32 at offset 0 of next entry.
    // Rest reserved (0).
    
    uint32_t base_high = (base >> 32);
    
    gdt[num + 1].limit_low = (base_high & 0xFFFF);
    gdt[num + 1].base_low = (base_high >> 16) & 0xFFFF;
    gdt[num + 1].base_middle = 0;
    gdt[num + 1].access = 0;
    gdt[num + 1].granularity = 0;
    gdt[num + 1].base_high = 0;
}

void gdt_init() {
    // 0: Null
    gdt_set_gate(0, 0, 0, 0, 0);

    // 1: Kernel Code (0x08)
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xA0);

    // 2: Kernel Data (0x10)
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xC0);

    // 3: User Data (0x18)
    gdt_set_gate(3, 0, 0xFFFFF, 0xF2, 0xC0);

    // 4: User Code (0x20)
    gdt_set_gate(4, 0, 0xFFFFF, 0xFA, 0xA0);

    // 5: TSS (0x28)
    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(tss) - 1;
    
    // Clear TSS
    uint8_t* tss_ptr = (uint8_t*)&tss;
    for(int i=0; i<sizeof(tss); i++) tss_ptr[i] = 0;
    
    // Set RSP0
    tss.rsp0 = (uint64_t)&stack[4096];
    
    gdt_set_tss_gate(5, tss_base, tss_limit);

    // Load GDT
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)&gdt;
    
    load_gdt(&gdtr);
    
    // Load TSS (0x28)
    load_tss(0x28);
    
    print_str("GDT & TSS Initialized.\n");
}
