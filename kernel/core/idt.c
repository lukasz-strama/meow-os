#include "core/idt.h"
#include "drivers/print.h"
#include "drivers/pic.h"
#include "drivers/io.h"

struct IdtEntry idt[256];
struct IdtPtr idt_ptr;

volatile uint64_t timer_ticks = 0;

extern uint64_t isr_table[];
extern void irq0_handler();
extern void isr_keyboard_stub();
extern void idt_load(struct IdtPtr* ptr);

void idt_set_entry(int index, uint64_t base, uint16_t selector, uint8_t type_attr) {
    idt[index].offset_low = base & 0xFFFF;
    idt[index].selector = selector;
    idt[index].ist = 0;
    idt[index].type_attr = type_attr;
    idt[index].offset_mid = (base >> 16) & 0xFFFF;
    idt[index].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[index].zero = 0;
}

void isr_handler_c(uint64_t* stack) {
    // stack[0..14] = GPRs
    // stack[15] = Interrupt Number
    // stack[16] = Error Code
    // stack[17] = RIP
    // stack[18] = CS
    // stack[19] = RFLAGS
    // stack[20] = RSP
    // stack[21] = SS

    uint64_t int_num = stack[15];
    uint64_t error_code = stack[16];
    uint64_t rip = stack[17];
    uint64_t cs = stack[18];
    uint64_t rsp = stack[20];
    
    print_set_color(PRINT_COLOR_RED, PRINT_COLOR_BLACK);
    print_str("\n[CPU] INTERRUPT RECEIVED!\n");
    
    print_str("Vector: "); print_hex(int_num); print_str("\n");
    print_str("Error:  "); print_hex(error_code); print_str("\n");
    print_str("RIP:    "); print_hex(rip); print_str("\n");
    print_str("CS:     "); print_hex(cs); print_str("\n");
    print_str("RSP:    "); print_hex(rsp); print_str("\n");

    // Dump bytes at RIP
    print_str("Code at RIP: ");
    uint8_t* code = (uint8_t*)rip;
    for(int i=0; i<8; i++) {
        print_hex(code[i]);
        print_str(" ");
    }
    print_str("\n");

    print_str("System Halted.\n");
    while(1);
}

void timer_handler() {
    timer_ticks++;
    outb(0x20, 0x20); // EOI
}

void idt_init() {
    pic_remap();

    idt_ptr.limit = (sizeof(struct IdtEntry) * 256) - 1;
    idt_ptr.base = (uint64_t)&idt;

    // Set handlers for 0-31 using the table
    for (int i = 0; i < 32; i++) {
        idt_set_entry(i, isr_table[i], 0x08, 0x8E);
    }

    // Set Timer Handler (IRQ0 -> 32)
    idt_set_entry(32, (uint64_t)irq0_handler, 0x08, 0x8E);

    // Set Keyboard Handler (IRQ1 -> 33)
    idt_set_entry(33, (uint64_t)isr_keyboard_stub, 0x08, 0x8E);

    idt_load(&idt_ptr);
}
