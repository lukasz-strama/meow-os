## Phase 1: The Core Foundation (Completed)

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

---

## Phase 2: Userland & Isolation (Completed)

### User Mode (Ring 3)
- [x] **Context Switching:** Assembly logic to switch CPU rings (`iretq`).
- [x] **Syscall Interface:** Implementation of `syscall`/`sysret` mechanism.
- [x] **Protection:** Enabling USER bits in Page Tables to prevent kernel memory access.

### Architecture Refactoring
- [x] **Directory Structure:** Separate `kernel/`, `libc/`, and `apps/`.
- [x] **Build System:** Recursive Makefile for modular compilation.

### MeowLib (The C Library)
- [x] **System Call Wrappers:** `sys_print`, `sys_exit`, `sys_get_ticks`, `sys_kbhit`, `sys_clear`, `sys_open`, `sys_read`, `sys_write`, `sys_close`.
- [x] **String Library:** `strcpy`, `strlen`, `memcpy`, `rand`, `srand`.
- [x] **Standard IO:** `printf` implementation for user space.

---

## Phase 3: Multitasking (Completed)

### Process Management
- [x] **PCB:** Process Control Block structure defined.
- [x] **Scheduler:** Round-Robin task switching implementation.
- [x] **Context Switching:** Full register saving/restoring in Assembly (`irq0_handler`).
- [x] **Kernel Threads:** Ability to run background tasks (e.g., Blinker) alongside the shell.
- [x] **Preemption:** Timer-based interrupt handling for multitasking.

---

## Phase 4: VFS & Unix Environment (Completed)

### Virtual File System
- [x] **VFS Layer:** Abstract node structure for Files, Directories, and Devices.
- [x] **DevFS:** Virtual filesystem exposing drivers as files (`/dev/keyboard`, `/dev/console`).
- [x] **File Descriptors:** Global FD table and standard streams (`stdin`, `stdout`).

### Shell & Environment
- [x] **User Shell (MeowSH):** Standalone userland shell.
- [x] **Session Manager:** Login screen (`login.bin`) and session loop.
- [x] **I/O Redirection:** Support for `>` operator in shell.
- [x] **CoreUtils:** `cat`, `echo`, `touch` migrated to standalone binaries.
- [x] **Power Management:** `shutdown`, `reboot`, `logout` commands.

### Program Loading
- [x] **Flat Binary Loader:** Executing raw `.bin` files via `exec` command.
- [x] **ELF64 Loader:** Parser for Executable and Linkable Format (Implemented in Kernel).

---

## Phase 5: Future Expansions (Todo)

- [ ] **Native ELF Execution:** Transition build system to use pure ELF files without `objcopy`.
- [ ] **Environment Variables:** Basic support for PATH and variables in Shell.
- [ ] **Process Isolation:** Implementing separate CR3 (Page Tables) for each process.
- [ ] **Graphics:** VESA / GOP Video Mode (Linear Framebuffer).
- [ ] **Mouse Driver:** PS/2 Mouse support.