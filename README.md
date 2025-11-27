# MeowOS

A barebones x86_64 operating system implemented in C and assembly.

## Features

- **Bootloader**: Multiboot-compliant x86_64 bootloader.
- **Memory Management**:
  - Physical Memory Manager (PMM) with bitmap allocation.
  - Virtual Memory Manager (VMM) with recursive 4-level paging.
  - Linked-list heap allocator.
- **Interrupts and I/O**:
  - IDT setup with PIC remapping.
  - Keyboard driver with circular buffer.
  - ATA PIO driver for disk I/O (read/write).
- **Filesystem**: FAT16 support (read/write/delete).
- **Shell**: Command-line interface with commands: help, clear, info, malloc_test, read_disk, write, ls, cat, mkfile, rm, edit.
- **Text Editor**: Simple TUI editor for file editing (not fully featured).

## Architecture

- **Kernel**: Written in C, compiled with GCC for freestanding environment.
- **Assembly**: Boot code and low-level initialization in NASM.
- **Target**: x86_64 architecture, runs in QEMU with a raw disk image.
- **Build System**: Makefile-based compilation and ISO generation with GRUB.

## Build Instructions

1. Ensure dependencies: GCC, NASM, LD, GRUB2, QEMU.
2. Run `make build-x86_64` to build the kernel ISO.
3. The ISO is generated at `dist/x86_64/kernel.iso`.

## Usage

- Boot the OS in QEMU: `qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso -drive file=disk.img,format=raw,index=0,media=disk`
- Interact via the shell prompt `MeowShell>`.
- Format the disk image as FAT16 if needed.

## File Structure

- `src/intf/`: Header files.
- `src/impl/x86_64/`: Assembly code.
- `src/impl/kernel/`: C kernel code.
- `targets/x86_64/`: Linker script and ISO structure.
- `build/`: Object files.
- `dist/`: Binaries and ISO.

## Dependencies

- GCC (freestanding)
- NASM
- GNU LD
- GRUB2
- QEMU

## License

MIT License.