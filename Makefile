# Rodmin OS Makefile
# Complete build system for all components

# Toolchain
CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy
MKISOFS = mkisofs

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
DRIVERS_DIR = drivers
FS_DIR = fs
GUI_DIR = gui
APPS_DIR = apps
CLI_DIR = cli
NET_DIR = net
SECURITY_DIR = security
SDK_DIR = sdk
ASSETS_DIR = assets
BUILD_DIR = build
TOOLS_DIR = tools

# Compiler flags
CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-builtin -nostdlib \
         -Wall -Wextra -O2 -mcmodel=large -mno-red-zone -mno-mmx -mno-sse \
         -mno-sse2 -I$(KERNEL_DIR) -I$(FS_DIR) -I$(GUI_DIR) -I$(NET_DIR)

ASFLAGS = -f elf64

LDFLAGS = -T linker.ld -nostdlib -z max-page-size=0x1000

# Source files
BOOT_SOURCES = $(BOOT_DIR)/stage1.asm $(BOOT_DIR)/stage2.asm
KERNEL_SOURCES = $(wildcard $(KERNEL_DIR)/*.c) $(wildcard $(KERNEL_DIR)/*.asm)
DRIVER_SOURCES = $(wildcard $(DRIVERS_DIR)/*.c) $(wildcard $(DRIVERS_DIR)/*/*.c)
FS_SOURCES = $(wildcard $(FS_DIR)/*.c)
GUI_SOURCES = $(wildcard $(GUI_DIR)/*.c)
NET_SOURCES = $(wildcard $(NET_DIR)/*.c) $(wildcard $(NET_DIR)/*/*.c)
CLI_SOURCES = $(wildcard $(CLI_DIR)/*.c) $(wildcard $(CLI_DIR)/*/*.c)
SECURITY_SOURCES = $(wildcard $(SECURITY_DIR)/*.c)

# Application sources
FILE_EXPLORER_SOURCES = $(wildcard $(APPS_DIR)/file_explorer/*.c)
TERMINAL_SOURCES = $(wildcard $(APPS_DIR)/terminal/*.c)
TEXT_EDITOR_SOURCES = $(wildcard $(APPS_DIR)/text_editor/*.c)
IMAGE_VIEWER_SOURCES = $(wildcard $(APPS_DIR)/image_viewer/*.c)
MUSIC_PLAYER_SOURCES = $(wildcard $(APPS_DIR)/music_player/*.c)
VIDEO_PLAYER_SOURCES = $(wildcard $(APPS_DIR)/video_player/*.c)
WEB_BROWSER_SOURCES = $(wildcard $(APPS_DIR)/web_browser/*.c)
EMAIL_CLIENT_SOURCES = $(wildcard $(APPS_DIR)/email_client/*.c)
CALENDAR_SOURCES = $(wildcard $(APPS_DIR)/calendar/*.c)
SYSTEM_MONITOR_SOURCES = $(wildcard $(APPS_DIR)/system_monitor/*.c)

# Object files
BOOT_OBJECTS = $(BUILD_DIR)/stage1.bin $(BUILD_DIR)/stage2.bin
KERNEL_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(filter %.c,$(KERNEL_SOURCES))) \
                 $(patsubst %.asm,$(BUILD_DIR)/%.o,$(filter %.asm,$(KERNEL_SOURCES)))
DRIVER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(DRIVER_SOURCES))
FS_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(FS_SOURCES))
GUI_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(GUI_SOURCES))
NET_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(NET_SOURCES))
CLI_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CLI_SOURCES))
SECURITY_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SECURITY_SOURCES))

# Application objects
FILE_EXPLORER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(FILE_EXPLORER_SOURCES))
TERMINAL_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(TERMINAL_SOURCES))
TEXT_EDITOR_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(TEXT_EDITOR_SOURCES))
IMAGE_VIEWER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(IMAGE_VIEWER_SOURCES))
MUSIC_PLAYER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(MUSIC_PLAYER_SOURCES))
VIDEO_PLAYER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(VIDEO_PLAYER_SOURCES))
WEB_BROWSER_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(WEB_BROWSER_SOURCES))
EMAIL_CLIENT_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(EMAIL_CLIENT_SOURCES))
CALENDAR_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CALENDAR_SOURCES))
SYSTEM_MONITOR_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SYSTEM_MONITOR_SOURCES))

# All kernel objects
ALL_KERNEL_OBJECTS = $(KERNEL_OBJECTS) $(DRIVER_OBJECTS) $(FS_OBJECTS) \
                     $(GUI_OBJECTS) $(NET_OBJECTS) $(CLI_OBJECTS) $(SECURITY_OBJECTS)

# Targets
.PHONY: all clean iso run debug apps

all: $(BUILD_DIR)/rodmin.iso

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)
	mkdir -p $(BUILD_DIR)/$(DRIVERS_DIR)
	mkdir -p $(BUILD_DIR)/$(FS_DIR)
	mkdir -p $(BUILD_DIR)/$(GUI_DIR)
	mkdir -p $(BUILD_DIR)/$(NET_DIR)
	mkdir -p $(BUILD_DIR)/$(CLI_DIR)
	mkdir -p $(BUILD_DIR)/$(SECURITY_DIR)
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/file_explorer
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/terminal
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/text_editor
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/image_viewer
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/music_player
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/video_player
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/web_browser
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/email_client
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/calendar
	mkdir -p $(BUILD_DIR)/$(APPS_DIR)/system_monitor

# Bootloader
$(BUILD_DIR)/stage1.bin: $(BOOT_DIR)/stage1.asm | $(BUILD_DIR)
	$(AS) -f bin $< -o $@

$(BUILD_DIR)/stage2.bin: $(BOOT_DIR)/stage2.asm | $(BUILD_DIR)
	$(AS) -f bin $< -o $@

# Kernel objects
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Kernel binary
$(BUILD_DIR)/kernel.bin: $(ALL_KERNEL_OBJECTS) linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(ALL_KERNEL_OBJECTS)

# Applications
$(BUILD_DIR)/file_explorer: $(FILE_EXPLORER_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(FILE_EXPLORER_OBJECTS) $(ALL_KERNEL_OBJECTS)

$(BUILD_DIR)/terminal: $(TERMINAL_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(TERMINAL_OBJECTS) $(ALL_KERNEL_OBJECTS)

$(BUILD_DIR)/text_editor: $(TEXT_EDITOR_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(TEXT_EDITOR_OBJECTS) $(ALL_KERNEL_OBJECTS)

$(BUILD_DIR)/image_viewer: $(IMAGE_VIEWER_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(IMAGE_VIEWER_OBJECTS) $(ALL_KERNEL_OBJECTS)

$(BUILD_DIR)/music_player: $(MUSIC_PLAYER_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(MUSIC_PLAYER_OBJECTS) $(ALL_KERNEL_OBJECTS)

$(BUILD_DIR)/video_player: $(VIDEO_PLAYER_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(VIDEO_PLAYER_OBJECTS) $(ALL_KERNEL_OBJECTS)

$(BUILD_DIR)/web_browser: $(WEB_BROWSER_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(WEB_BROWSER_OBJECTS) $(ALL_KERNEL_OBJECTS)

$(BUILD_DIR)/email_client: $(EMAIL_CLIENT_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(EMAIL_CLIENT_OBJECTS) $(ALL_KERNEL_OBJECTS)

$(BUILD_DIR)/calendar: $(CALENDAR_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(CALENDAR_OBJECTS) $(ALL_KERNEL_OBJECTS)

$(BUILD_DIR)/system_monitor: $(SYSTEM_MONITOR_OBJECTS) $(ALL_KERNEL_OBJECTS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(SYSTEM_MONITOR_OBJECTS) $(ALL_KERNEL_OBJECTS)

# All applications
apps: $(BUILD_DIR)/file_explorer $(BUILD_DIR)/terminal $(BUILD_DIR)/text_editor \
      $(BUILD_DIR)/image_viewer $(BUILD_DIR)/music_player $(BUILD_DIR)/video_player \
      $(BUILD_DIR)/web_browser $(BUILD_DIR)/email_client $(BUILD_DIR)/calendar \
      $(BUILD_DIR)/system_monitor

# Create disk image
$(BUILD_DIR)/disk.img: $(BUILD_DIR)/stage1.bin $(BUILD_DIR)/stage2.bin $(BUILD_DIR)/kernel.bin apps | $(BUILD_DIR)
	# Create 100MB disk image
	dd if=/dev/zero of=$@ bs=1M count=100
	
	# Write bootloader
	dd if=$(BUILD_DIR)/stage1.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD_DIR)/stage2.bin of=$@ bs=512 count=8 seek=1 conv=notrunc
	
	# Write kernel
	dd if=$(BUILD_DIR)/kernel.bin of=$@ bs=512 seek=9 conv=notrunc
	
	# Create file system and copy files
	$(TOOLS_DIR)/create_fs.sh $@

# Create ISO image
$(BUILD_DIR)/rodmin.iso: $(BUILD_DIR)/disk.img | $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/iso
	cp $(BUILD_DIR)/disk.img $(BUILD_DIR)/iso/
	cp -r $(ASSETS_DIR)/* $(BUILD_DIR)/iso/ 2>/dev/null || true
	$(MKISOFS) -b disk.img -no-emul-boot -boot-load-size 4 -boot-info-table \
	           -o $@ $(BUILD_DIR)/iso

# Run in QEMU
run: $(BUILD_DIR)/rodmin.iso
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/rodmin.iso -m 512M -enable-kvm \
	                    -netdev user,id=net0 -device rtl8139,netdev=net0 \
	                    -audiodev pa,id=audio0 -device ac97,audiodev=audio0

# Debug with GDB
debug: $(BUILD_DIR)/rodmin.iso
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/rodmin.iso -m 512M -s -S &
	gdb -ex "target remote localhost:1234" -ex "symbol-file $(BUILD_DIR)/kernel.bin"

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Install to USB drive (be careful!)
install: $(BUILD_DIR)/rodmin.iso
	@echo "WARNING: This will overwrite the target device!"
	@echo "Make sure you specify the correct device."
	@read -p "Enter device (e.g., /dev/sdb): " device; \
	sudo dd if=$(BUILD_DIR)/rodmin.iso of=$$device bs=1M status=progress

# Create development environment
dev-setup:
	sudo apt-get update
	sudo apt-get install -y build-essential nasm qemu-system-x86 gdb \
	                        genisoimage mtools xorriso
	
# Documentation
docs:
	mkdir -p $(BUILD_DIR)/docs
	doxygen Doxyfile

# Code analysis
analyze:
	cppcheck --enable=all --std=c99 $(KERNEL_DIR) $(DRIVERS_DIR) $(FS_DIR) \
	         $(GUI_DIR) $(NET_DIR) $(CLI_DIR) $(SECURITY_DIR) $(APPS_DIR)

# Performance profiling
profile: $(BUILD_DIR)/rodmin.iso
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/rodmin.iso -m 512M \
	                    -monitor stdio -enable-kvm

# Memory leak detection
memcheck: $(BUILD_DIR)/rodmin.iso
	valgrind --tool=memcheck --leak-check=full \
	         qemu-system-x86_64 -cdrom $(BUILD_DIR)/rodmin.iso -m 512M

# Unit tests
test:
	$(MAKE) -C tests all
	$(BUILD_DIR)/tests/run_tests

# Continuous integration
ci: clean all test analyze docs
	@echo "All CI checks passed!"

# Help
help:
	@echo "Rodmin OS Build System"
	@echo "====================="
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build complete OS (default)"
	@echo "  clean      - Clean build files"
	@echo "  iso        - Create ISO image"
	@echo "  run        - Run in QEMU"
	@echo "  debug      - Debug with GDB"
	@echo "  apps       - Build all applications"
	@echo "  install    - Install to USB drive"
	@echo "  dev-setup  - Setup development environment"
	@echo "  docs       - Generate documentation"
	@echo "  analyze    - Run static code analysis"
	@echo "  profile    - Performance profiling"
	@echo "  memcheck   - Memory leak detection"
	@echo "  test       - Run unit tests"
	@echo "  ci         - Continuous integration"
	@echo "  help       - Show this help"

# Dependencies
-include $(BUILD_DIR)/*.d