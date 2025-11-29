# MeowOS Memory Map

> **Note:** This document describes the Physical and Virtual memory layout of the MeowOS kernel (x86_64).
> Addresses are represented in Hexadecimal.

## 1. Physical Memory Layout
The physical RAM is managed by the **PMM (Physical Memory Manager)** using a Bitmap allocator.
Regions marked as **Reserved** are protected from dynamic allocation.

| Start Address | End Address | Region Name | Description |
| :--- | :--- | :--- | :--- |
| `0x00000000` | `0x00001000` | **IVT / BIOS Data** | Legacy Real Mode structures (Preserved). |
| `0x00001000` | `0x0009FFFF` | **Low Memory** | Free/Used by Bootloader. |
| `0x000A0000` | `0x000BFFFF` | **Video RAM** | VGA Text Mode Buffer resides at `0xB8000`. |
| `0x000C0000` | `0x000FFFFF` | **BIOS ROM** | System BIOS / Video BIOS Shadows. |
| `0x00100000` | `_kernel_end`| **Kernel Image** | The Kernel code/data loaded by GRUB. |
| `_kernel_end`| `+ 64KB`     | **Safety Gap** | Padding to prevent PMM from overwriting Bootloader Page Tables (PML4/PDP/PD). |
| `+ 64KB`     | `+ Bitmap Sz`| **PMM Bitmap** | The allocation bitmap for the Physical Memory Manager. |
| **`0x00400000`** | `...` | **User Program Load** | Standard entry point for `exec` (Flat Binary / ELF). |
| **`0x00500000`** | `0x00501000` | **User Mode Stack** | 4KB Stack for Ring 3 processes (Grows down). |
| **`0x00600000`** | `0x00601000` | **Kernel ISR Stack** | **TSS RSP0**: Target stack for interrupts occurring in User Mode. |
| `...`        | `TOTAL_RAM`  | **Dynamic Store** | Free physical frames handed out by PMM (used for Heap, Page Tables, etc.). |

---

## 2. Virtual Memory Layout (Paging)
MeowOS uses **4-Level Paging (Long Mode)**.
Currently, the kernel operates in the lower half (Identity Mapped) for simplicity.

### Global Map
| Virtual Address Range | Type | Permissions | Description |
| :--- | :--- | :--- | :--- |
| `0x00000000` - `0x40000000` | **Identity Map** | `RWX | User/Sup` | The first 1GB of physical RAM is mapped 1:1. Includes Kernel, VGA, and Userland base. |
| `0x40000000` - `0x46400000` | **Kernel Heap** | `RW | Supervisor` | 100MB region reserved for `malloc`/`free`. Backed by non-contiguous physical pages. |
| `...` | ... | ... | Unmapped space (triggers Page Fault). |

### Important Virtual Addresses
* **VGA Buffer:** `0xB8000` (Directly accessible via Identity Map).
* **User Entry Point:** `0x400000` (Fixed load address for binaries).
* **User Stack Top:** `0x501000` (Argument pointers `argv` are pushed here).
* **Kernel Heap Base:** `0x40000000` (Defined in `heap.h`).

---

## 3. Kernel Symbols (Linker Script)
These symbols are defined in `linker.ld` and used by the kernel to determine its own size and location.

* `_kernel_start`: Start of the `.text` section (usually `1M` / `0x100000`).
* `_kernel_end`: End of the `.bss` section. Used by PMM to calculate where safe memory begins.

## 4. GDT & TSS Layout
Global Descriptor Table configuration for Ring 0 and Ring 3 switching.

| Index | Selector | Type | Access Byte | Flags |
| :--- | :--- | :--- | :--- | :--- |
| 0 | `0x00` | Null | `0x00` | `0x00` |
| 1 | `0x08` | **Kernel Code** | `0x9A` (Exec/Read, Ring 0) | `0xA` (Long Mode) |
| 2 | `0x10` | **Kernel Data** | `0x92` (Read/Write, Ring 0) | `0xC` |
| 3 | `0x1B` | **User Data** | `0xF2` (Read/Write, Ring 3) | `0xC` |
| 4 | `0x23` | **User Code** | `0xFA` (Exec/Read, Ring 3) | `0xA` (Long Mode) |
| 5 | `0x28` | **TSS** | `0x89` (System Segment) | - |

> **Note:** User Selectors include the RPL (Requested Privilege Level) of 3.
> Base selectors are `0x18` (Data) and `0x20` (Code).
> TSS RSP0 is hardcoded to `0x601000` (Top of 6MB page).