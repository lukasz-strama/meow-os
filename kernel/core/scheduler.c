#include "core/process.h"
#include "drivers/print.h"
#include "memory/heap.h"
#include "core/stats.h"

#define MAX_PROCESSES 3

Process processes[MAX_PROCESSES];
Process* current_process = &processes[0];
int process_count = 0;

void scheduler_init() {
    // Initialize Process 0 (The current running kernel thread)
    // We don't need to set RSP here because it will be saved when the first interrupt fires.
    processes[0].pid = 0;
    processes[0].state = 1; // Running
    
    // Set name "kernel"
    char* name = "kernel";
    for(int i=0; i<31 && name[i]; i++) processes[0].name[i] = name[i];
    processes[0].name[6] = '\0';

    process_count = 1;
    current_process = &processes[0];
    printf("Scheduler Initialized. Main Process PID: 0\n");
}

void process_create(char* name, void (*fn)()) {
    if (process_count >= MAX_PROCESSES) {
        printf("Error: Max processes reached.\n");
        return;
    }

    Process* p = &processes[process_count];
    p->pid = process_count;
    p->state = 0; // Ready
    
    // Copy name
    int i = 0;
    while (i < 31 && name[i]) {
        p->name[i] = name[i];
        i++;
    }
    p->name[i] = '\0';

    // Setup Stack
    p->kstack = (uint64_t)malloc(4096);
    uint64_t* stack = (uint64_t*)(p->kstack + 4096);

    // Simulate Interrupt Frame for Kernel Thread
    // Push SS, RSP, RFLAGS, CS, RIP to match IRETQ expectation.
    
    *(--stack) = 0x10;         // SS
    *(--stack) = (uint64_t)(p->kstack + 4096); // RSP
    *(--stack) = 0x202;        // RFLAGS (Interrupts enabled)
    *(--stack) = 0x08;         // CS (Kernel Code)
    *(--stack) = (uint64_t)fn; // RIP
    
    // ISR Wrapper Pushes (irq0_handler)
    // push 0 (Dummy)
    // push 32 (Vector)
    *(--stack) = 0;  // Dummy Error Code
    *(--stack) = 32; // Vector Number
    
    // General Purpose Registers (15 regs)
    // R15, R14, R13, R12, R11, R10, R9, R8, RBP, RDI, RSI, RDX, RCX, RBX, RAX
    for (int i = 0; i < 15; i++) {
        *(--stack) = 0;
    }
    
    // DS/ES (The current handler pushes DS as RAX, then sets DS/ES to 0x10)
    // "mov ax, ds; push rax"
    *(--stack) = 0x10; // DS (We want kernel data segment)
    
    p->rsp = (uint64_t)stack;
    process_count++;
    printf("Process %d created. Entry: %p, RSP: %p\n", p->pid, fn, p->rsp);
}

void schedule() {
    // Simple Round Robin
    int next_id = (current_process->pid + 1) % process_count;
    current_process = &processes[next_id];
    // printf("Switching to PID %d\n", current_process->pid);
}

int get_process_info(int pid, ProcessInfo* info) {
    if (pid < 0 || pid >= process_count) return -1;
    Process* p = &processes[pid];
    info->pid = p->pid;
    info->state = p->state;
    
    int i = 0;
    while (i < 31 && p->name[i]) {
        info->name[i] = p->name[i];
        i++;
    }
    info->name[i] = '\0';
    
    return 0;
}
