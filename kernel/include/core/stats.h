#ifndef STATS_H
#define STATS_H

#include <stdint.h>

typedef struct {
    int pid;
    int state; // 0=Ready, 1=Running, 2=Blocked/Dead
    char name[32];
} ProcessInfo;

typedef struct {
    uint64_t total_ram;
    uint64_t free_ram;
    uint64_t used_ram;
} MemInfo;

#endif
