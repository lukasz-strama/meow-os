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

# Userland
USER_DIR = userland
USER_BUILD_DIR = $(BUILD_DIR)/userland
USER_LDFLAGS = -n -T $(USER_DIR)/linker.ld

USER_C_SOURCES := $(shell find $(USER_DIR) -name '*.c')
USER_ASM_SOURCES := $(shell find $(USER_DIR) -name '*.asm')
USER_OBJECTS := $(patsubst $(USER_DIR)/%.c, $(USER_BUILD_DIR)/%.o, $(USER_C_SOURCES)) \
                $(patsubst $(USER_DIR)/%.asm, $(USER_BUILD_DIR)/%.o, $(USER_ASM_SOURCES))

# Ensure start.o is linked first
USER_START_OBJ := $(USER_BUILD_DIR)/lib/start.o
USER_HELLO_OBJ := $(USER_BUILD_DIR)/hello.o
USER_SNAKE_OBJ := $(USER_BUILD_DIR)/snake.o
USER_SHELL_OBJ := $(USER_BUILD_DIR)/shell.o
USER_LOGIN_OBJ := $(USER_BUILD_DIR)/login.o
USER_EDITOR_OBJ := $(USER_BUILD_DIR)/editor.o
USER_PS_OBJ := $(USER_BUILD_DIR)/ps.o
USER_FREE_OBJ := $(USER_BUILD_DIR)/free.o

# Library objects are everything except start.o, hello.o, snake.o, shell.o, login.o, editor.o, ps.o, free.o
USER_LIB_OBJS := $(filter-out $(USER_START_OBJ) $(USER_HELLO_OBJ) $(USER_SNAKE_OBJ) $(USER_SHELL_OBJ) $(USER_LOGIN_OBJ) $(USER_EDITOR_OBJ) $(USER_PS_OBJ) $(USER_FREE_OBJ), $(USER_OBJECTS))

KERNEL_BIN = $(DIST_DIR)/kernel.bin
HELLO_BIN = $(DIST_DIR)/hello.bin
SNAKE_BIN = $(DIST_DIR)/snake.bin
SHELL_BIN = $(DIST_DIR)/shell.bin
LOGIN_BIN = $(DIST_DIR)/login.bin
NANO_BIN = $(DIST_DIR)/nano.bin
PS_BIN = $(DIST_DIR)/ps.bin
FREE_BIN = $(DIST_DIR)/free.bin
ISO_IMAGE = $(DIST_DIR)/meowos.iso

.PHONY: all clean run

all: $(ISO_IMAGE) $(HELLO_BIN) $(SNAKE_BIN) $(SHELL_BIN) $(LOGIN_BIN) $(NANO_BIN) $(PS_BIN) $(FREE_BIN)

$(KERNEL_BIN): $(ASM_OBJECTS) $(C_OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "--> Kernel Linked"

$(HELLO_BIN): $(USER_OBJECTS) disk-setup
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) -o $(DIST_DIR)/hello.elf $(USER_START_OBJ) $(USER_LIB_OBJS) $(USER_HELLO_OBJ)
	objcopy -O binary $(DIST_DIR)/hello.elf $@
	@echo "--> Hello App Built"
	@if [ -f disk.img ]; then \
		mcopy -o -i disk.img $@ ::/BIN/HELLO.BIN || echo "Failed to copy to disk.img"; \
	else \
		echo "Warning: disk.img not found, skipping copy"; \
	fi

$(SNAKE_BIN): $(USER_OBJECTS) disk-setup
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) -o $(DIST_DIR)/snake.elf $(USER_START_OBJ) $(USER_LIB_OBJS) $(USER_SNAKE_OBJ)
	objcopy -O binary $(DIST_DIR)/snake.elf $@
	@echo "--> Snake App Built"
	@if [ -f disk.img ]; then \
		mcopy -o -i disk.img $@ ::/BIN/SNAKE.BIN || echo "Failed to copy to disk.img"; \
	else \
		echo "Warning: disk.img not found, skipping copy"; \
	fi

$(SHELL_BIN): $(USER_OBJECTS) disk-setup
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) -o $(DIST_DIR)/shell.elf $(USER_START_OBJ) $(USER_LIB_OBJS) $(USER_SHELL_OBJ)
	objcopy -O binary $(DIST_DIR)/shell.elf $@
	@echo "--> Shell App Built"
	@if [ -f disk.img ]; then \
		mcopy -o -i disk.img $@ ::/BIN/SHELL.BIN || echo "Failed to copy to disk.img"; \
	else \
		echo "Warning: disk.img not found, skipping copy"; \
	fi

$(LOGIN_BIN): $(USER_OBJECTS) disk-setup
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) -o $(DIST_DIR)/login.elf $(USER_START_OBJ) $(USER_LIB_OBJS) $(USER_LOGIN_OBJ)
	objcopy -O binary $(DIST_DIR)/login.elf $@
	@echo "--> Login App Built"
	@if [ -f disk.img ]; then \
		mcopy -o -i disk.img $@ ::/BIN/LOGIN.BIN || echo "Failed to copy to disk.img"; \
	else \
		echo "Warning: disk.img not found, skipping copy"; \
	fi

$(NANO_BIN): $(USER_OBJECTS) disk-setup
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) -o $(DIST_DIR)/nano.elf $(USER_START_OBJ) $(USER_LIB_OBJS) $(USER_EDITOR_OBJ)
	objcopy -O binary $(DIST_DIR)/nano.elf $@
	@echo "--> Nano App Built"
	@if [ -f disk.img ]; then \
		mcopy -o -i disk.img $@ ::/BIN/NANO.BIN || echo "Failed to copy to disk.img"; \
	else \
		echo "Warning: disk.img not found, skipping copy"; \
	fi

$(PS_BIN): $(USER_OBJECTS) disk-setup
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) -o $(DIST_DIR)/ps.elf $(USER_START_OBJ) $(USER_LIB_OBJS) $(USER_PS_OBJ)
	objcopy -O binary $(DIST_DIR)/ps.elf $@
	@echo "--> PS App Built"
	@if [ -f disk.img ]; then \
		mcopy -o -i disk.img $@ ::/BIN/PS.BIN || echo "Failed to copy to disk.img"; \
	else \
		echo "Warning: disk.img not found, skipping copy"; \
	fi

$(FREE_BIN): $(USER_OBJECTS) disk-setup
	@mkdir -p $(dir $@)
	$(LD) $(USER_LDFLAGS) -o $(DIST_DIR)/free.elf $(USER_START_OBJ) $(USER_LIB_OBJS) $(USER_FREE_OBJ)
	objcopy -O binary $(DIST_DIR)/free.elf $@
	@echo "--> Free App Built"
	@if [ -f disk.img ]; then \
		mcopy -o -i disk.img $@ ::/BIN/FREE.BIN || echo "Failed to copy to disk.img"; \
	else \
		echo "Warning: disk.img not found, skipping copy"; \
	fi

$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf64 -g -o $@ $<

$(USER_BUILD_DIR)/%.o: $(USER_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -ffreestanding -mno-red-zone -fno-builtin -nostdlib -mno-sse -mno-sse2 -mno-mmx -mgeneral-regs-only -I$(USER_DIR) -c -o $@ $<

$(USER_BUILD_DIR)/%.o: $(USER_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(ASM) -f elf64 -g -o $@ $<

$(ISO_IMAGE): $(KERNEL_BIN)
	@mkdir -p $(DIST_DIR)/iso/boot/grub
	cp $(KERNEL_BIN) $(DIST_DIR)/iso/boot/kernel.bin
	cp $(TARGET_DIR)/iso/boot/grub/grub.cfg $(DIST_DIR)/iso/boot/grub/grub.cfg
	grub2-mkrescue -o $(ISO_IMAGE) $(DIST_DIR)/iso
	@echo "--> ISO Created"

disk-setup:
	@mkdir -p $(DIST_DIR)
	@if [ ! -f disk.img ]; then \
		echo "Creating disk.img..."; \
		dd if=/dev/zero of=disk.img bs=1M count=32; \
		mkfs.fat -F 16 disk.img; \
	fi
	-mmd -i disk.img ::/BIN
	-mmd -i disk.img ::/ETC
	-mmd -i disk.img ::/HOME
	@echo "lukasz:123" > $(DIST_DIR)/passwd
	mcopy -o -i disk.img $(DIST_DIR)/passwd ::/ETC/PASSWD

run: $(ISO_IMAGE)
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -drive file=disk.img,format=raw,index=0,media=disk -boot d

debug: $(ISO_IMAGE)
	qemu-system-x86_64 -cdrom $(ISO_IMAGE) -drive file=disk.img,format=raw,index=0,media=disk -boot d -no-reboot -no-shutdown -serial stdio

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)
