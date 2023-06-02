GNU := riscv64-unknown-elf
COPS = -g -O0 -Wall -nostdlib -march=rv64imac -mabi=lp64 -I include -mcmodel=medany -fno-PIE -fomit-frame-pointer -Wno-builtin-declaration-mismatch -ggdb -g
BUILD_ROOT= build
BUILD_DIR = $(BUILD_ROOT)/os
SRC_DIR = kernel
LIB_DIR = lib
USR_DIR = usr

SRC_EXP_DIR = $(SRC_DIR)/trap
SRC_MEM_DIR = $(SRC_DIR)/memory
SRC_PRO_DIR = $(SRC_DIR)/process
SRC_DRIVER_DIR = $(SRC_DIR)/driver

BUILD_LIB_DIR = $(BUILD_ROOT)/lib
BUILD_EXP_DIR = $(BUILD_ROOT)/trap
BUILD_MEM_DIR = $(BUILD_ROOT)/memory
BUILD_PRO_DIR = $(BUILD_ROOT)/process
BUILD_USR_DIR = $(BUILD_ROOT)/usr
BUILD_DRIVER_DIR = $(BUILD_ROOT)/driver

all : clean kernel-qemu

clean :
	rm -rf $(BUILD_ROOT)  *.bin  *.map *.elf
	rm -rf kernel-qemu

$(BUILD_DRIVER_DIR)/%_c.o: $(SRC_DRIVER_DIR)/%.c
	mkdir -p $(BUILD_DRIVER_DIR); echo " CC   $@" ; $(GNU)-gcc $(COPS) -c $< -o $@

$(BUILD_PRO_DIR)/%_c.o: $(SRC_PRO_DIR)/%.c
	mkdir -p $(BUILD_PRO_DIR); echo " CC   $@" ; $(GNU)-gcc $(COPS) -c $< -o $@

$(BUILD_EXP_DIR)/%_c.o: $(SRC_EXP_DIR)/%.c
	mkdir -p $(BUILD_EXP_DIR); echo " CC   $@" ; $(GNU)-gcc $(COPS) -c $< -o $@

$(BUILD_MEM_DIR)/%_c.o: $(SRC_MEM_DIR)/%.c
	mkdir -p $(BUILD_MEM_DIR); echo " CC   $@" ; $(GNU)-gcc $(COPS) -c $< -o $@

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR); echo " CC   $@" ; $(GNU)-gcc   $(COPS) -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	mkdir -p $(BUILD_DIR); echo " AS   $@"; $(GNU)-gcc $(COPS) -c $< -o $@

$(BUILD_LIB_DIR)/%_c.o: $(LIB_DIR)/%.c
	mkdir -p $(BUILD_LIB_DIR); echo " CC   $@" ; $(GNU)-gcc $(COPS) -c $< -o $@

$(BUILD_USR_DIR)/%_c.o: $(USR_DIR)/%.c
	$(CMD_PREFIX)mkdir -p $(BUILD_USR_DIR); echo " CC   $@" ; $(GNU)-gcc $(COPS) -c $< -o $@

$(BUILD_USR_DIR)/%_s.o: $(USR_DIR)/%.S
	mkdir -p $(BUILD_USR_DIR); echo " AS   $@"; $(GNU)-gcc $(COPS) $(AOPS) -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
LIB_FILES = $(wildcard $(LIB_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
SRC_EXP_FILES = $(wildcard $(SRC_EXP_DIR)/*.c)
SRC_MEM_FILES = $(wildcard $(SRC_MEM_DIR)/*.c)
SRC_PRO_FILES = $(wildcard $(SRC_PRO_DIR)/*.c)
SRC_DRIVER_FILES = $(wildcard $(SRC_DRIVER_DIR)/*.c)
USR_C_FILES = $(wildcard $(USR_DIR)/*.c)
USR_A_FILES = $(wildcard $(USR_DIR)/*.S)

OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
OBJ_FILES += $(LIB_FILES:$(LIB_DIR)/%.c=$(BUILD_LIB_DIR)/%_c.o)
OBJ_FILES += $(SRC_EXP_FILES:$(SRC_EXP_DIR)/%.c=$(BUILD_EXP_DIR)/%_c.o)
OBJ_FILES += $(SRC_MEM_FILES:$(SRC_MEM_DIR)/%.c=$(BUILD_MEM_DIR)/%_c.o)
OBJ_FILES += $(SRC_PRO_FILES:$(SRC_PRO_DIR)/%.c=$(BUILD_PRO_DIR)/%_c.o)
OBJ_FILES += $(SRC_DRIVER_FILES:$(SRC_DRIVER_DIR)/%.c=$(BUILD_DRIVER_DIR)/%_c.o)
OBJ_FILES += $(USR_C_FILES:$(USR_DIR)/%.c=$(BUILD_USR_DIR)/%_c.o)
OBJ_FILES += $(USR_A_FILES:$(USR_DIR)/%.S=$(BUILD_USR_DIR)/%_s.o)

kernel-qemu: $(SRC_DIR)/linker.ld $(OBJ_FILES) $(LIB_DIR)/*.a
	$(GNU)-ld -z muldefs -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel-qemu.elf  $(OBJ_FILES) $(LIB_DIR)/*.a -Map os.map; echo " LD $(BUILD_DIR)/kernel-qemu.elf"
	$(GNU)-objcopy $(BUILD_DIR)/kernel-qemu.elf -O binary kernel-qemu; echo " OBJCOPY kernel-qemu"
	cp $(BUILD_DIR)/kernel-qemu.elf kernel-qemu.elf
	

######################run qemu#########################################
QEMU_FLAGS  = -nographic  -machine virt  -m 128M -smp 2
QEMU_BIOS = -bios default	  -kernel kernel-qemu
QEMU_DIVICE = -global virtio-mmio.force-legacy=false
QEMU_DEVICES = -drive file=sdcard.img,if=none,format=raw,id=x0  -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 
QEMU_DUMP_DEVICE_TREE = -machine dumpdtb=qemu-virt.dtb
#./bootloader/fw_jump.bin
run:
	qemu-system-riscv64 $(QEMU_FLAGS) $(QEMU_DEVICES) $(QEMU_BIOS)
debug:
	qemu-system-riscv64 $(QEMU_FLAGS) $(QEMU_DEVICES) $(QEMU_BIOS) -gdb tcp::1234 -S 
	riscv64-unknown-elf-gdb ./kernel-qemu.bin.elf
	target remote localhost:1234