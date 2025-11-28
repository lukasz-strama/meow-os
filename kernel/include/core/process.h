#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "core/stats.h"

typedef struct {
    uint64_t rsp;       // Saved Stack Pointer
    uint64_t cr3;       // Page Directory Base Register
    uint64_t kstack;    // Kernel Stack Top
    int pid;
    int state;          // 0=READY, 1=RUNNING, 2=BLOCKED, 3=TERMINATED
    char name[32];      // Process Name
} Process;

extern Process* current_process;

void scheduler_init();
void process_create(char* name, void (*fn)());
void schedule();
int get_process_info(int pid, ProcessInfo* info);

#endif
