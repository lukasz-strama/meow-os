#include <stdint.h>

// Internal structures to avoid header dependency hell
struct GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct TSSEntry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

struct GDTDescriptor {
    uint16_t size;
    uint64_t offset;
} __attribute__((packed));

// Global Storage
static struct GDTEntry gdt[7]; // 0:Null, 1:KC, 2:KD, 3:UD, 4:UC, 5:TSS_Lo, 6:TSS_Hi
static struct TSSEntry tss;
static uint8_t stack_for_interrupts[4096];

// Assembly helper
extern void load_gdt(struct GDTDescriptor* gdtr);
extern void load_tss(uint16_t selector);

// Helper to encode entry
void set_gdt_entry(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[index].base_low    = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high   = (base >> 24) & 0xFF;
    gdt[index].limit_low   = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[index].access      = access;
}

void fix_gdt() {
    // 0. Setup TSS
    // Important: Fill with zeros first!
    uint8_t* tss_ptr = (uint8_t*)&tss;
    for(int i=0; i<sizeof(struct TSSEntry); i++) tss_ptr[i] = 0;
    
    tss.rsp0 = (uint64_t)stack_for_interrupts + 4096;
    tss.iomap_base = sizeof(struct TSSEntry); // Disable IO Map

    uint64_t tss_base = (uint64_t)&tss;
    uint64_t tss_limit = sizeof(struct TSSEntry) - 1;

    // 1. Setup GDT Entries
    // Index 0: Null
    set_gdt_entry(0, 0, 0, 0, 0);

    // Index 1: Kernel Code (Offset 0x08)
    // Access: 0x9A (Present, Ring0, Code, Read), Gran: 0xA0 (Long Mode)
    set_gdt_entry(1, 0, 0, 0x9A, 0xA0); // Limit is ignored in Long Mode

    // Index 2: Kernel Data (Offset 0x10)
    // Access: 0x92 (Present, Ring0, Data, Write)
    set_gdt_entry(2, 0, 0, 0x92, 0x00);

    // Index 3: User Data (Offset 0x18) -- SELECTOR 0x1B
    // Access: 0xF2 (Present, Ring3, Data, Write)
    set_gdt_entry(3, 0, 0, 0xF2, 0x00);

    // Index 4: User Code (Offset 0x20) -- SELECTOR 0x23
    // Access: 0xFA (Present, Ring3, Code, Read), Gran: 0xA0 (Long Mode)
    // CRITICAL: 0xA0 means Long Mode (Bit 5 of gran) is SET.
    set_gdt_entry(4, 0, 0, 0xFA, 0xA0);

    // Index 5 & 6: TSS (System Segment, 16 bytes) -- SELECTOR 0x28
    // Access: 0x89 (Present, Ring0, Available TSS)
    set_gdt_entry(5, (uint32_t)tss_base, (uint32_t)tss_limit, 0x89, 0x00);
    
    // TSS High part (bits 32-63 of base) goes into what looks like the next entry
    gdt[6].limit_low = (uint16_t)(tss_base >> 32);
    gdt[6].base_low  = (uint16_t)(tss_base >> 48);
    gdt[6].base_middle = 0;
    gdt[6].access = 0;
    gdt[6].granularity = 0;
    gdt[6].base_high = 0;

    // 2. Load
    struct GDTDescriptor gdtr;
    gdtr.size = sizeof(gdt) - 1;
    gdtr.offset = (uint64_t)&gdt;

    load_gdt(&gdtr);
    load_tss(0x28); // Load TSS (Offset 5 * 8)
}
