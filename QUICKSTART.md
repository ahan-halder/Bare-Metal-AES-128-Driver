# Quick Start Guide

## One-Time Setup

```bash
cd Bare-Metal-AES-128-Driver
mkdir -p sim
gcc -Wall -Wextra -O2 -DAES_DRIVER_USE_EXTERNAL_MMIO=1 -Iinclude -o sim/testbench src/testbench.c src/aes_driver.c
```

If you have GNU Make installed, you can also run `mingw32-make test` on Windows or `make test` on a Unix-like shell.

## Running Tests

```bash
# Run C testbench with mock hardware
./sim/testbench

# View RTL files
ls rtl/*.v

# Clean build artifacts
rm -rf sim/testbench sim/obj_dir *.o
```

The testbench now prints a byte-grid view of the input and output blocks plus a per-byte diff map, so you can visually confirm whether the result matches the reference vector.

## Project Layout

```
rtl/              - Verilog RTL modules (synthesis-ready)
src/              - C source code (bare-metal driver + tests)
include/          - C headers (driver API + test vectors)
sim/              - Simulation artifacts and compiled testbench
Makefile          - Build automation
README.md         - Project overview
DEVELOPMENT.md    - Complete technical documentation
QUICKSTART.md     - This file
```

## Using the Driver in Your Code

### Example: Encrypt 128 bytes

```c
#include "aes_driver.h"

int main(void) {
    aes_ctx_t ctx;
    uint8_t key[16] = { 
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    uint8_t plaintext[16] = { /* your data */ };
    uint8_t ciphertext[16];

    // Initialize
    aes_init(&ctx, 0x80000000);  // Hardware base address
    
    // Set key (once per different key)
    aes_set_key(&ctx, key);
    
    // Encrypt
    aes_encrypt(&ctx, plaintext, ciphertext);
    
    // ciphertext now contains encrypted data
    return 0;
}
```

## Hardware Integration Checklist

- [ ] AES hardware mapped to address `0x80000000` (or adjust `AES_BASE_ADDR`)
- [ ] Clock and reset connected
- [ ] Memory bus (32-bit) connected to CPU
- [ ] Interrupt output wired (optional)
- [ ] All RTL modules synthesized without errors
- [ ] Post-synthesis simulation passes test vectors

## What The Console Output Shows

The C testbench gives you three layers of feedback:

1. A standard hex dump of the key, plaintext, ciphertext, and expected bytes.
2. A 4x4 byte matrix view of the block so you can inspect the AES state layout.
3. A diff map where `.` means the byte matches the expected output and `^` means it differs.

## Module Dependencies

```
aes_128_wrapper.v
├── aes_128_core.v
│   ├── key_expansion.v
│   ├── sub_bytes.v
│   ├── shift_rows.v
│   ├── mix_columns.v
│   ├── InvShiftRows.v
│   ├── InvSubstitutionMatrix.v
│   └── InvMixColumns.v
├── sbox_lut.v
└── (existing transformation modules)
```

## Key Points

1. **No OS Required**: Pure bare-metal - works in bootloader, firmware, or bare hardware
2. **MMIO-Based**: All hardware access through memory-mapped registers
3. **Blocking API**: `aes_encrypt()` and `aes_decrypt()` block until complete
4. **Test Vectors**: NIST standard vectors included for validation
5. **Verilog RTL**: Synthesis-ready, no behavioral code

## Support

For issues or questions about specific components:
- RTL design: Check `DEVELOPMENT.md` → RTL Implementation section
- Driver API: Check `include/aes_driver.h` comments
- Test results: Run `make test` and check output
- Waveforms: Use Verilator with `--trace` flag

## Next Steps

1. Review `DEVELOPMENT.md` for complete architecture details
2. Run `make test` to verify basic functionality
3. Integrate RTL into your synthesis flow
4. Modify `AES_BASE_ADDR` if needed for your platform
5. Link `aes_driver.c` and `aes_driver.h` into your firmware build
