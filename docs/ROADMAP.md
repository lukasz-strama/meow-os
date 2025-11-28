## Phase 1: The Core Foundation

### Architecture & Boot
- [x] **Multiboot2 Compliance:** Booting via GRUB.
- [x] **Long Mode (64-bit):** Transition from Protected Mode to Long Mode.
- [x] **GDT (Global Descriptor Table):** Setup for Kernel and User segments.
- [x] **IDT (Interrupt Descriptor Table):** Handling CPU Exceptions and IRQs.

### Memory Management
- [x] **PMM (Physical Memory Manager):** Bitmap allocator for physical frames.
- [x] **VMM (Virtual Memory Manager):** 4-Level Paging implementation.
- [x] **Heap Allocator:** Dynamic memory (`malloc`, `free`) linked list implementation.
- [x] **Stack Protection:** Separate stacks for Kernel and User operations.

### Drivers & I/O
- [x] **VGA Driver:** Text mode output (80x25) with color support.
- [x] **PS/2 Keyboard:** Interrupt-driven input with circular buffer and Shift key support.
- [x] **PIC Remapping:** Programmable Interrupt Controller setup.
- [x] **ATA PIO:** Hard Disk Driver (Read/Write sectors).

### Filesystem & Storage
- [x] **FAT16 Driver:**
    - [x] Read file content (`cat`).
    - [x] List directory (`ls`).
    - [x] Create files (`mkfile`).
    - [x] Delete files (`rm`).
- [x] **Persistence:** Integration with a raw disk image (`disk.img`).

### Kernel Monitor (KMonitor)
- [x] **Interactive Shell:** Basic command line interface running in Ring 0.
- [x] **Text Editor:** TUI-based editor with save functionality.

## Phase 2: Userland & Isolation

### User Mode (Ring 3)
- [x] **Context Switching:** Assembly logic to switch CPU rings (`iretq`).
- [x] **Syscall Interface:** Implementation of `syscall`/`sysret` mechanism.
- [x] **Protection:** Enabling USER bits in Page Tables to prevent kernel memory access.

### Architecture Refactoring
- [x] **Directory Structure:** Separate `kernel/`, `libc/`, and `apps/`.
- [x] **Build System:** Recursive Makefile for modular compilation.

### MeowLib (The C Library)
- [x] **System Call Wrappers:** `open`, `close`, `read`, `write`, `exit`.
- [x] **String Library:** `strcpy`, `strlen`, `memcpy`.
- [x] **Standard IO:** `printf` implementation for user space.

## Phase 3: Program Execution

- [ ] **ELF64 Loader:** Parser for Executable and Linkable Format.
- [ ] **User Shell (MeowSH):** Porting the KMonitor logic to a standalone user application.
- [ ] **Memory Protection:** Ensuring processes cannot crash the kernel.

## Phase 4: Multitasking & Advanced Features

- [ ] **Process Management:** PCB (Process Control Block) structures.
- [ ] **Scheduler:** Round-Robin task switching (Preemptive Multitasking).
- [ ] **Virtual File System (VFS):** Abstract layer for file operations (support for InitRD).
- [ ] **Graphics:** VESA / GOP Video Mode (Linear Framebuffer).