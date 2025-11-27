#pragma once
#include <stdint.h>

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) GDTDescriptor;

typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed)) TSS;

void gdt_init();
void load_gdt(GDTDescriptor* gdt);
void load_tss(uint16_t selector);
void enter_user_mode(uint64_t rip, uint64_t rsp);
