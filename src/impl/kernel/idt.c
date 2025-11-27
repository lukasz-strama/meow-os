#include "idt.h"
#include "print.h"

struct IdtEntry idt[256];
struct IdtPtr idt_ptr;

extern void idt_load(struct IdtPtr* ptr);
extern void isr_stub();

void idt_set_entry(int index, uint64_t base, uint16_t selector, uint8_t type_attr) {
    idt[index].offset_low = base & 0xFFFF;
    idt[index].selector = selector;
    idt[index].ist = 0;
    idt[index].type_attr = type_attr;
    idt[index].offset_mid = (base >> 16) & 0xFFFF;
    idt[index].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[index].zero = 0;
}

void isr_handler_c() {
    print_str("Interrupt Received!\n");
}

void idt_init() {
    idt_ptr.limit = (sizeof(struct IdtEntry) * 256) - 1;
    idt_ptr.base = (uint64_t)&idt;

    // 0x08 is the code segment offset in GDT (from boot.asm)
    // 0x8E = Present (1) | DPL 0 (00) | 0 | Gate Type Interrupt (1110)
    idt_set_entry(33, (uint64_t)isr_stub, 0x08, 0x8E);

    idt_load(&idt_ptr);
}
