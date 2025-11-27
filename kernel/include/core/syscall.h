#pragma once
#include <stdint.h>

void syscall_init();
void wrmsr(uint32_t msr, uint64_t val);
