# ====== Config ======
CXX        := clang++
CC         := clang
AS         := nasm
LD         := ld.lld

INCLUDE_DIR := include

CXXFLAGS := -Wall -Wextra -std=c++23 -g -MMD -MP \
            -I$(INCLUDE_DIR) \
            -ffreestanding -fno-exceptions -fno-rtti -m64

CFLAGS   := -Wall -Wextra -g -MMD -MP \
            -ffreestanding -m64

ASFLAGS  := -f elf64

LDFLAGS  := -T linker.ld -nostdlib
LDLIBS   :=

SRC_DIR   := src
BUILD_DIR := build
ISO_DIR   := iso

TARGET    := kernel.elf
ISO       := os.iso

# ====== Sources ======
CPP_SRC := $(shell find $(SRC_DIR) -name '*.cpp' -print | sed 's/ /\\ /g')
ASM_SRC := $(shell find $(SRC_DIR) -name '*.asm' -print | sed 's/ /\\ /g')

CPP_OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SRC))
ASM_OBJ := $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(ASM_SRC))

OBJ := $(CPP_OBJ) $(ASM_OBJ)
DEP := $(CPP_OBJ:.o=.d)

# ====== Default target ======
all: iso

# ====== Link kernel ======
$(BUILD_DIR)/$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJ) -o $@

# ====== Compile C++ ======
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ====== Assemble ======
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# ====== Dependencies ======
-include $(DEP)

# ====== ISO ======
iso: $(BUILD_DIR)/$(TARGET)
	rm -rf $(ISO_DIR)
	mkdir -p $(ISO_DIR)/boot/grub

	cp $(BUILD_DIR)/$(TARGET) $(ISO_DIR)/boot/kernel.elf
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg

	grub-mkrescue -o $(ISO) $(ISO_DIR)

# ====== Run ======
run: iso
	qemu-system-x86_64 -cdrom $(ISO) -serial stdio

# ====== Compile Commands ======
compile_commands.json:
	bear -- $(MAKE)

# ====== Clean ======
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO) compile_commands.json

tags:
	ctags -R

# ====== Phony ======
.PHONY: all tags clean run iso compile_commands.json
