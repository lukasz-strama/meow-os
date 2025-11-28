#include "lib/syscalls.h"

void main() {
    sys_print("----------------------------------\n");
    sys_print("Hello form Userland! (Loaded from Disk)\n");
    sys_print("Exec works! I am a standalone binary.\n");
    sys_print("----------------------------------\n");

    // Spin forever so we don't crash returning to nowhere
    while(1);
}
