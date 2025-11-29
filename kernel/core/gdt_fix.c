#include <stdint.h>

// Debug prints helper (assume declared in headers)
void printf(const char* fmt, ...);

// 1. Packed Structures
struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

// x86_64 TSS Structure (Must be 104 bytes)
struct TSSEntry {
    uint32_t reserved0;
    uint64_t rsp0;       // Offset 4
    uint64_t rsp1;       // Offset 12
    uint64_t rsp2;       // Offset 20
    uint64_t reserved1;
    uint64_t ist[7];     // Offset 36
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

struct GDTDescriptor {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));

// Global Tables (Aligned for performance/safety)
__attribute__((aligned(16))) static struct GDTEntry gdt[7];
__attribute__((aligned(16))) static struct TSSEntry tss;

// ASM Helpers
extern void load_gdt(struct GDTDescriptor* gdtr);
extern void load_tss(uint16_t selector);

// Helper to encode standard entry
void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index].base_low    = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high   = (base >> 24) & 0xFF;
    gdt[index].limit_low   = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[index].access      = access;
}

void fix_gdt() {
    // printf("[GDT] Re-initializing GDT & TSS...\n");

    // 1. Clear & Setup TSS
    // Fill with zeros first
    uint8_t* tss_ptr = (uint8_t*)&tss;
    for(int i=0; i<sizeof(struct TSSEntry); i++) tss_ptr[i] = 0;

    // Set Critical Fields
    // Move Kernel Interrupt Stack to 6MB mark (Safe, Identity Mapped RAM)
    // 0x600000 + 4KB (Stack grows down)
    tss.rsp0 = 0x600000 + 4096;
    tss.iomap_base = sizeof(struct TSSEntry); // Disable IO Map

    // printf("[GDT] TSS Base: %p, RSP0: %x (Safe Location)\n", &tss, tss.rsp0);

    // 2. Setup GDT Entries
    // Index 0: Null
    set_gdt_entry(0, 0, 0, 0, 0);

    // Index 1: Kernel Code (0x08)
    set_gdt_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);

    // Index 2: Kernel Data (0x10)
    set_gdt_entry(2, 0, 0xFFFFF, 0x92, 0xC0);

    // Index 3: User Data (0x1B)
    set_gdt_entry(3, 0, 0xFFFFF, 0xF2, 0xC0);

    // Index 4: User Code (0x23)
    set_gdt_entry(4, 0, 0xFFFFF, 0xFA, 0xA0);

    // Index 5 & 6: TSS (System Segment)
    uint64_t tss_base = (uint64_t)&tss;
    uint32_t tss_limit = sizeof(struct TSSEntry) - 1;

    // Low 32 bits of Base
    set_gdt_entry(5, (uint32_t)tss_base, tss_limit, 0x89, 0x00);
    
    // High 32 bits of Base (Special format for System Descriptors)
    // The "Limit" field in the next entry holds the middle of the base
    // The "Base" field holds the top
    gdt[6].limit_low = (uint16_t)((tss_base >> 32) & 0xFFFF);
    gdt[6].base_low  = (uint16_t)((tss_base >> 48) & 0xFFFF);
    gdt[6].base_middle = 0;
    gdt[6].access = 0;
    gdt[6].granularity = 0;
    gdt[6].base_high = 0;

    // 3. Load
    struct GDTDescriptor gdtr;
    gdtr.size = sizeof(gdt) - 1;
    gdtr.offset = (uint64_t)&gdt;

    // printf("[GDT] Loading GDTR (Size: %d, Offset: %p)...\n", gdtr.size, gdtr.offset);
    load_gdt(&gdtr);
    
    // printf("[GDT] Loading TR (0x28)...\n");
    load_tss(0x28);
    
    // printf("[GDT] Done.\n");
}




