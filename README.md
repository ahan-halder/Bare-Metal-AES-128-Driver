# Bare-Metal AES-128 Driver

Bare-metal C driver, RTL, and testbench for a memory-mapped AES-128 hardware accelerator.

This repository is organized around one main workflow: write a 128-bit key and 128-bit input block to an AES IP core over MMIO, start the operation, poll for completion, and read back the output. The C side is intentionally small and firmware-like so it mirrors an early bring-up environment rather than an application-level crypto library.

## What’s In The Repo

- `src/aes_driver.c`: bare-metal driver implementation.
- `include/aes_driver.h`: register map, MMIO helpers, and public API.
- `src/testbench.c`: host-side test runner with a mock MMIO backend.
- `include/aes_test_vectors.h`: NIST-style AES-128 vectors.
- `rtl/`: Verilog RTL for the AES core and wrapper.
- `main.ino`: separate ESP32 demo that uses the Arduino AES library, not the MMIO hardware.
- `Makefile`: build entry points for the C testbench and RTL simulation.

## Quick Start

If you just want to build and run the C testbench from the repo root on Windows/MSYS or Git Bash:

```bash
mkdir -p sim
gcc -Wall -Wextra -O2 -DAES_DRIVER_USE_EXTERNAL_MMIO=1 -Iinclude -o sim/testbench testbench.c src/aes_driver.c
./sim/testbench
```

The root-level `testbench.c` is a thin wrapper that includes `src/testbench.c`, so the command above works exactly as shown. If you prefer the direct path, compile `src/testbench.c` instead.

With GNU Make:

```bash
mingw32-make test
```

or on Unix-like shells:

```bash
make test
```

## What The Testbench Shows

The mock testbench validates three AES vectors for both encryption and decryption and prints:

- a hex dump of key, input, output, and expected values,
- a 4x4 byte matrix view of the block,
- a byte-level diff map where `.` means match and `^` means mismatch.

Successful runs end with `6` tests passed and `0` failed.

## Build And Run

### C Testbench

Build:

```bash
gcc -Wall -Wextra -O2 -DAES_DRIVER_USE_EXTERNAL_MMIO=1 -Iinclude -o sim/testbench testbench.c src/aes_driver.c
```

Run:

```bash
./sim/testbench
```

Clean:

```bash
rm -rf sim/testbench sim/obj_dir *.o
```

### Makefile Targets

```bash
mingw32-make test     # Build and run the host testbench
mingw32-make all      # Build testbench and RTL sim scaffold
mingw32-make clean    # Remove build artifacts
mingw32-make help     # Show available targets
```

## Driver Overview

The driver uses a simple MMIO register map at `AES_BASE_ADDR`:

| Offset | Register | Direction | Purpose |
|---|---|---|---|
| `0x00` | `CTRL` | RW | Start bit and mode bit |
| `0x04` | `STATUS` | R | Busy/done flags |
| `0x08-0x14` | `KEY[3:0]` | W | 128-bit key |
| `0x18-0x24` | `DATA_IN[3:0]` | W | 128-bit input block |
| `0x28-0x34` | `DATA_OUT[3:0]` | R | 128-bit output block |

### C API

```c
void aes_init(aes_ctx_t *ctx, uint32_t base_addr);
void aes_set_key(aes_ctx_t *ctx, const uint8_t *key);
int aes_encrypt(aes_ctx_t *ctx, const uint8_t *plaintext, uint8_t *ciphertext);
int aes_decrypt(aes_ctx_t *ctx, const uint8_t *ciphertext, uint8_t *plaintext);
uint32_t aes_get_status(aes_ctx_t *ctx);
int aes_is_busy(aes_ctx_t *ctx);
int aes_is_done(aes_ctx_t *ctx);
```

### Minimal Usage

```c
#include "aes_driver.h"

int main(void) {
    aes_ctx_t ctx;
    uint8_t key[16] = { /* 16 bytes */ };
    uint8_t plaintext[16] = { /* 16 bytes */ };
    uint8_t ciphertext[16];

    aes_init(&ctx, 0x80000000);
    aes_set_key(&ctx, key);
    aes_encrypt(&ctx, plaintext, ciphertext);
    return 0;
}
```

## Mock Hardware Test Mode

The C testbench uses an in-memory MMIO model so it can run on a normal PC without hardware. That mode is enabled by defining `AES_DRIVER_USE_EXTERNAL_MMIO=1`, which makes the header expose MMIO hooks instead of inline pointer dereferences.

That is why the working build command includes:

```bash
-DAES_DRIVER_USE_EXTERNAL_MMIO=1
```

## RTL Integration

The RTL folder contains the AES core and wrapper that match the MMIO layout above. The intended hardware bring-up flow is:

1. Write the key into `KEY[3:0]`.
2. Write the 16-byte input into `DATA_IN[3:0]`.
3. Set `CTRL.start` and the encrypt/decrypt mode bit.
4. Poll `STATUS` until `DONE` is set.
5. Read the result from `DATA_OUT[3:0]`.

If you integrate the IP into a larger SoC, keep `AES_BASE_ADDR` aligned with the bus map used by your system.

## ESP32 Note

The ESP32 sketch in `main.ino` is a separate demo path. It uses the ESP32 software AES library and serial commands; it does not talk to the MMIO AES block used by the C driver and RTL.

Use that sketch if you want a standalone microcontroller demo. Use the C driver if you want the actual register-level hardware flow.

## Repository Layout

```
rtl/       Verilog RTL modules
src/       C driver and host testbench
include/   Driver headers and test vectors
sim/       Build output for the host testbench
main.ino   ESP32 demo sketch
Makefile   Build entry points
```

## Troubleshooting

- If `gcc` cannot find `testbench.c`, use the root-level wrapper or compile `src/testbench.c` directly.
- If the testbench hangs or fails, confirm `AES_DRIVER_USE_EXTERNAL_MMIO=1` is present for the host build.
- If you want the build to start from the repo root every time, use `mingw32-make test`.
- If you are targeting real hardware, replace the mock MMIO path with your platform’s actual address map.

## Related Docs

This README is the primary entry point. The other markdown files are retained as deeper or legacy references, but the setup and usage steps you need are now consolidated here.
 