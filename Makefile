KERNEL_DIR = kernel
BUILD_DIR = build
DIST_DIR = dist
TARGET_DIR = targets/x86_64

CC = gcc
ASM = nasm
LD = ld

# Include the kernel/include directory for headers
CFLAGS = -ffreestanding -mno-sse -mno-sse2 -mno-mmx -mno-80387 -mno-red-zone -g -I$(KERNEL_DIR)/include
LDFLAGS = -n -T $(TARGET_DIR)/linker.ld

# Recursively find all source files
C_SOURCES := $(shell find $(KERNEL_DIR) -name '*.c')
ASM_SOURCES := $(shell find $(KERNEL_DIR) -name '*.asm')

# Map sources to object files in build dir
C_OBJECTS := $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/kernel/%.o, $(C_SOURCES))
ASM_OBJECTS := $(patsubst $(KERNEL_DIR)/%.asm, $(BUILD_DIR)/kernel/%.o, $(ASM_SOURCES))

KERNEL_BIN = $(DIST_DIR)/kernel.bin
ISO_IMAGE = $(DIST_DIR)/meowos.iso

.PHONY: all clean run

all: $(ISO_IMAGE)

$(KERNEL_BIN): $(ASM_OBJECTS) $(C_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "--> Kernel Linked"

$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf64 -g -o $@ $<

$(ISO_IMAGE): $(KERNEL_BIN)
	@mkdir -p $(DIST_DIR)/iso/boot/grub
	cp $(KERNEL_BIN) $(DIST_DIR)/iso/boot/kernel.bin
	cp $(TARGET_DIR)/iso/boot/grub/grub.cfg $(DIST_DIR)/iso/boot/grub/grub.cfg
	grub2-mkrescue -o $(ISO_IMAGE) $(DIST_DIR)/iso
	@echo "--> ISO Created"

run: $(ISO_IMAGE)
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -drive file=disk.img,format=raw,index=0,media=disk -boot d

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
