#include "time.h"
#include "syscalls.h"

void sleep(int ticks) {
    unsigned long start = sys_get_ticks();
    while (sys_get_ticks() < start + ticks) {
        // Busy wait
    }
}
