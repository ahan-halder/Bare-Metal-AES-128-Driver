# ESP32 Usage Guide

This guide explains how to use the ESP32-WROOM board with the Arduino IDE to run the AES-128 demo in [main.ino](main.ino).

## What This Project Does On ESP32

The ESP32 does not connect to the AES MMIO hardware described in the C driver files. Instead, the Arduino sketch uses the ESP32's built-in AES software library to:

1. Encrypt a 16-byte block with a 16-byte key.
2. Decrypt a 16-byte block with a 16-byte key.
3. Verify the result using a known NIST AES-128 test vector.
4. Accept commands from the Serial Monitor so you can test your own inputs.
5. Blink the onboard LED so you can see when AES work is running.
6. Run a simple benchmark so you can measure speed and resource use.

## Requirements

- ESP32-WROOM board
- Micro-USB cable
- Arduino IDE
- ESP32 board support installed in Arduino IDE

## Open The Sketch

Open [main.ino](main.ino) in Arduino IDE and upload it to the ESP32.

If you are starting from a fresh Arduino install, make sure the ESP32 board package is installed through Boards Manager.

## Serial Monitor Settings

Use these Serial Monitor settings:

- Baud rate: `115200`
- Line ending: `Newline`

## Available Commands

The sketch accepts three command types:

- `E <key_hex> <plaintext_hex>`: encrypt one 16-byte block
- `D <key_hex> <ciphertext_hex>`: decrypt one 16-byte block
- `T`: run the built-in NIST test vector
- `B [count]`: run the AES benchmark and print timing/statistics

Hex input must be 32 hexadecimal characters for the key and 32 hexadecimal characters for the data block.

## Example Commands

### Run The Built-In Test

```text
T
```

### Encrypt One Block

```text
E 2b7e151628aed2a6abf7158809cf4f3c 6bc1bee22e409f96e93d7e117393172a
```

### Decrypt One Block

```text
D 2b7e151628aed2a6abf7158809cf4f3c 3ad77bb40d7a3660a89ecaf32466ef97
```

### Run A Benchmark

```text
B 1000
```

## Expected Output

For the built-in test, the Serial Monitor should show:

- the key
- the plaintext
- the computed ciphertext
- the expected ciphertext
- `Encrypt test: PASS`
- the decrypted block
- `Decrypt test: PASS`

During any AES command, the onboard LED turns on while the operation is running and then flashes after completion. If your board LED behaves inverted, change the LED on/off levels in [main.ino](main.ino).

## LED Behavior

- LED on steady: AES operation is in progress
- Short flash after success: command completed successfully
- Double flash after a successful `T` test run
- Triple flash after a successful benchmark
- Four flashes after a failed test or benchmark

If your board uses a different LED pin or the LED is active-high, update `STATUS_LED_PIN`, `LED_ON_LEVEL`, and `LED_OFF_LEVEL` in [main.ino](main.ino).

## Workflow

1. Connect the ESP32 to the PC using the micro-USB cable.
2. Select the correct board in Arduino IDE.
3. Select the correct COM port.
4. Upload [main.ino](main.ino).
5. Open Serial Monitor at `115200` baud.
6. Send `T` first to confirm the sketch is working.
7. Send your own `E` or `D` command.
8. Use `B 1000` to run the benchmark and compare performance over repeated blocks.

## Input Format Notes

- Spaces inside the hex strings are ignored.
- Uppercase and lowercase hex are both accepted.
- The sketch only processes one 16-byte block at a time.
- The benchmark command uses a fixed AES input so results are easier to compare between runs.

## Troubleshooting

### Nothing Prints In Serial Monitor

- Check that the baud rate is `115200`.
- Press the ESP32 reset button after opening the Serial Monitor.
- Verify the correct COM port is selected.

### Upload Fails

- Confirm the ESP32 board package is installed in Arduino IDE.
- Try a different USB cable if the board is not detected.
- Close any other program using the COM port.

### Command Rejected

- Make sure the key is exactly 32 hex characters.
- Make sure the block is exactly 32 hex characters.
- Use `T` if you just want to verify the sketch first.

## Analysis You Can Do On ESP32

The sketch can measure some useful things directly on the ESP32:

- Encryption and decryption time in microseconds
- Average time per 16-byte block
- Blocks per second and throughput in KB/s
- Free heap before and after the run
- CPU frequency reported by the board

Use `B 1000` or a larger count if you want more stable timing results.

## What The ESP32 Cannot Measure Well By Itself

The sketch cannot accurately measure these on its own:

- Board temperature or heat rise
- Real power draw from USB
- CPU utilization as a percentage over time

For those, use external tools such as:

- A USB power meter for current and voltage
- An IR thermometer or thermal camera for temperature
- An oscilloscope or logic analyzer if you later move to hardware AES signals

If you want a quick thermal estimate, run the benchmark for several minutes and compare the board temperature before and after with an external thermometer.

## Important Limitation

The bare-metal driver in this repository is designed for an external AES hardware block mapped at a fixed register address. The ESP32 sketch is a software demo and command tool, not a direct interface to that MMIO hardware.

If you want, the next step can be a version that reads commands from Serial and then forwards them to a real external AES accelerator over GPIO, SPI, or UART.