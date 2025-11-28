#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef struct {
    uint64_t rsp;       // Saved Stack Pointer
    int pid;
    int state;          // 0=Ready, 1=Running
    uint8_t kstack[4096]; // Kernel Stack
} Process;

extern Process* current_process;

void scheduler_init();
void process_create(void (*fn)());
void schedule();

#endif
