GNU := riscv64-unknown-elf
COPS = -g -O0 -Wall -nostdlib -march=rv64imac -mabi=lp64 -Iinclude -mcmodel=medany -fno-PIE -fomit-frame-pointer -Wno-builtin-declaration-mismatch -ggdb -g
BUILD_ROOT= build
BUILD_DIR = $(BUILD_ROOT)/os
SRC_DIR = kernel
LIB_DIR = lib
SRC_EXP_DIR = $(SRC_DIR)/trap
BUILD_LIB_DIR = $(BUILD_ROOT)/lib
BUILD_EXP_DIR = $(BUILD_ROOT)/trap
all : clean benos.bin 

clean :
	rm -rf $(BUILD_ROOT)  *.bin  *.map *.elf

$(BUILD_EXP_DIR)/%_c.o: $(SRC_EXP_DIR)/%.c
	mkdir -p $(BUILD_EXP_DIR); echo " CC   $@" ; $(GNU)-gcc $(COPS) -c $< -o $@

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR); echo " CC   $@" ; $(GNU)-gcc   $(COPS) -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	mkdir -p $(BUILD_DIR); echo " AS   $@"; $(GNU)-gcc $(COPS) -c $< -o $@

$(BUILD_LIB_DIR)/%_c.o: $(LIB_DIR)/%.c
	mkdir -p $(BUILD_LIB_DIR); echo " CC   $@" ; $(GNU)-gcc $(COPS) -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
LIB_FILES = $(wildcard $(LIB_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
SRC_EXP_FILES = $(wildcard $(SRC_EXP_DIR)/*.c)

OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)
OBJ_FILES += $(LIB_FILES:$(LIB_DIR)/%.c=$(BUILD_LIB_DIR)/%_c.o)
OBJ_FILES += $(SRC_EXP_FILES:$(SRC_EXP_DIR)/%.c=$(BUILD_EXP_DIR)/%_c.o)

benos.bin: $(SRC_DIR)/linker.ld $(OBJ_FILES) $(LIB_DIR)/*.a
	$(GNU)-ld -z muldefs -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/os.elf  $(OBJ_FILES) $(LIB_DIR)/*.a -Map os.map; echo " LD $(BUILD_DIR)/os.elf"
	$(GNU)-objcopy $(BUILD_DIR)/os.elf -O binary os.bin; echo " OBJCOPY os.bin"
	cp $(BUILD_DIR)/os.elf os.elf
	

######################run qemu#########################################
QEMU_FLAGS  += -nographic  -machine virt -m 128M	
QEMU_BIOS = -bios default  -kernel os.elf
QEMU_DEVICES = -device loader,file=os.bin,addr=0x80200000  
#./bootloader/fw_jump.bin
run:
	qemu-system-riscv64 $(QEMU_FLAGS) $(QEMU_BIOS) 
debug:
	qemu-system-riscv64 $(QEMU_FLAGS) $(QEMU_BIOS) -gdb tcp::1234 -S 
	riscv64-unknown-elf-gdb ./os.elf
	target remote localhost:1234