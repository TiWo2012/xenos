# ====== Config ======
CXX     := c++
INCLUDE_DIR := include
CXXFLAGS := -Wall -Wextra -Werror -std=c++23 -g -MMD -MP -I$(INCLUDE_DIR)

LDFLAGS :=
LDLIBS  :=

SRC_DIR := src
BUILD_DIR := build
TARGET := app

# ====== Sources ======
SRC := $(shell find $(SRC_DIR) -name '*.cpp' -print | sed 's/ /\\ /g')
OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
DEP := $(OBJ:.o=.d)

# ====== Default target ======
all: $(BUILD_DIR)/$(TARGET)

# ====== Link ======
$(BUILD_DIR)/$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

# ====== Compile ======
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ====== Dependencies ======
-include $(DEP)

run: $(BUILD_DIR)/$(TARGET)
	./$(BUILD_DIR)/$(TARGET)

# ====== Compile Commands ======
compile_commands.json:
	bear -- $(MAKE)

# ====== Clean ======
clean:
	rm -rf $(BUILD_DIR) compile_commands.json

tags:
	ctags -R

# ====== Phony ======
.PHONY: all tags clean run compile_commands.json
