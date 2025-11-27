#include "pic.h"
#include "io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

void pic_remap() {
    uint8_t a1, a2;

    // Save masks
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    // Start initialization sequence (ICW1)
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);

    // ICW2: Vector offsets
    outb(PIC1_DATA, 0x20); // Master PIC vector offset 32 (0x20)
    outb(PIC2_DATA, 0x28); // Slave PIC vector offset 40 (0x28)

    // ICW3: Cascading
    outb(PIC1_DATA, 4);    // Tell Master there is a Slave at IRQ2 (0000 0100)
    outb(PIC2_DATA, 2);    // Tell Slave its cascade identity (0000 0010)

    // ICW4: Environment (8086 mode)
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    // Mask all interrupts except IRQ1 (Keyboard)
    // 0xFD = 1111 1101 (Bit 1 is 0, so IRQ1 is enabled. Bit 0 is 1, so IRQ0/Timer is disabled)
    outb(PIC1_DATA, 0xFD);
    outb(PIC2_DATA, 0xFF);
}
