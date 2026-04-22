# AES-128 Hardware Accelerator Project Makefile
# Supports RTL simulation and bare-metal firmware testing

.PHONY: all clean rtl_sim test help

# ============================================================================
# Configuration
# ============================================================================

VERILATOR = verilator
VERILATOR_FLAGS = -Wall --trace --cc

GCC = gcc
GCC_FLAGS = -Wall -Wextra -O2 -fno-builtin
GCC_INCLUDE = -I./include

# Directories
RTL_DIR = rtl
SRC_DIR = src
INC_DIR = include
SIM_DIR = sim
TEST_DIR = test_vectors

# ============================================================================
# Targets
# ============================================================================

all: testbench rtl_sim
	@echo "Build complete!"

# Build C testbench with mock hardware
testbench: $(SRC_DIR)/testbench.c $(SRC_DIR)/aes_driver.c
	@mkdir -p $(SIM_DIR)
	$(GCC) $(GCC_FLAGS) -DAES_DRIVER_USE_EXTERNAL_MMIO=1 $(GCC_INCLUDE) -o $(SIM_DIR)/testbench \
		$(SRC_DIR)/testbench.c $(SRC_DIR)/aes_driver.c
	@echo "Testbench built successfully"

# Run testbench
test: testbench
	@echo "Running testbench..."
	@./$(SIM_DIR)/testbench

# Verilator RTL simulation
rtl_sim: $(RTL_DIR)/*.v
	@echo "Building Verilator RTL simulation..."
	@mkdir -p $(SIM_DIR)
	@$(VERILATOR) $(VERILATOR_FLAGS) -o $(SIM_DIR)/rtl_sim \
		$(RTL_DIR)/aes_128_wrapper.v \
		$(RTL_DIR)/aes_128_core.v \
		$(RTL_DIR)/key_expansion.v \
		$(RTL_DIR)/sbox_lut.v \
		2>&1 | head -20

# View RTL dependencies
view_rtl:
	@ls -lah $(RTL_DIR)/*.v

# Clean build artifacts
clean:
	@rm -rf $(SIM_DIR)/testbench
	@rm -rf $(SIM_DIR)/obj_dir
	@rm -rf *.o
	@echo "Cleaned build artifacts"

help:
	@echo "AES-128 Hardware Accelerator Project"
	@echo "===================================="
	@echo "Available targets:"
	@echo "  make all          - Build everything"
	@echo "  make testbench    - Build C testbench"
	@echo "  make test         - Run C testbench"
	@echo "  make rtl_sim      - Build Verilator RTL simulation"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make help         - Show this help"
