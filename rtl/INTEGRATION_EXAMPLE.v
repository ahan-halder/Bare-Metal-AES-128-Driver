// Hardware Integration Example
// Shows how to instantiate AES-128 wrapper in a larger SoC

module soc_with_aes (
    input  wire         clk,
    input  wire         rst,

    // CPU Bus Interface (Avalon/AXI-like example)
    input  wire [31:0]  bus_addr,
    input  wire [31:0]  bus_write_data,
    output wire [31:0]  bus_read_data,
    input  wire         bus_write_en,
    input  wire         bus_read_en,

    // Interrupt output
    output wire         aes_irq
);

    // Address decoder - separate address spaces for different peripherals
    wire [31:0] aes_read_data;
    wire        aes_write_en;
    wire        aes_read_en;

    // AES occupies address range 0x80000000 - 0x8000003F
    assign aes_write_en = bus_write_en && (bus_addr[31:6] == 26'h2000000);
    assign aes_read_en  = bus_read_en  && (bus_addr[31:6] == 26'h2000000);

    // Instantiate AES-128 wrapper
    aes_128_wrapper aes_inst (
        .clk(clk),
        .rst(rst),
        .addr(bus_addr[5:0]),           // Only lower 6 bits for register offset
        .write_data(bus_write_data),
        .read_data(aes_read_data),
        .write_en(aes_write_en),
        .read_en(aes_read_en),
        .irq(aes_irq)
    );

    // Multiplex read data from different peripherals
    assign bus_read_data = aes_read_en ? aes_read_data : 32'h00000000;
    // Note: In a real system, multiplex with other peripherals

endmodule

// ============================================================================
// Firmware Usage Example
// ============================================================================

/*
// In your bootloader or firmware:

#define AES_BASE_ADDR 0x80000000

void encrypt_data(void) {
    aes_ctx_t ctx;

    uint8_t key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };

    uint8_t plaintext[16] = {
        0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
        0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
    };

    uint8_t ciphertext[16];

    // Initialize driver
    aes_init(&ctx, AES_BASE_ADDR);

    // Load key
    aes_set_key(&ctx, key);

    // Perform encryption
    int result = aes_encrypt(&ctx, plaintext, ciphertext);

    if (result == 0) {
        // Encryption successful
        // ciphertext now contains: 30 c8 1c 46 a3 5c e4 11
        //                          e5 fb c1 19 1a 0a 52 ef
    }
}

// Interrupt handler example (if using IRQ):
void aes_irq_handler(void) {
    // Operation complete
    uint8_t result[16];
    aes_read_128(AES_REG_DATA_OUT_0, result);
    // ... process result ...
}
*/

// ============================================================================
// Timing and Throughput Analysis
// ============================================================================

/*
PERFORMANCE METRICS:

Clock Frequency: 200 MHz (typical for ASIC)

AES-128 Latency:
- Key expansion: 0 cycles (combinational)
- Rounds 0-10: 11 cycles
- Total: 11 cycles
- Time: 11 / 200 MHz = 55 nanoseconds

Throughput:
- 128 bits per 11 cycles
- = 128 bits / 55 ns
- ≈ 1.45 Gbps

Example: Encrypting 1 MB
- Size: 1,048,576 bytes = 131,072 blocks of 128 bits
- Time: 131,072 blocks × 55 ns = 7.21 ms

Power Consumption (estimated for 28nm technology):
- Idle: ~1 mW (only clocks)
- Active: ~50 mW (full AES operation)
- Peak: ~100 mW (with surrounding logic)
*/

// ============================================================================
// Area Breakdown (estimated for 28nm technology)
// ============================================================================

/*
HARDWARE RESOURCE USAGE:

LUT Usage:
- S-box lookup: 3.2K LUTs
- Key expansion: 1.8K LUTs
- Transformations: 4.2K LUTs
- MMIO wrapper: 2.1K LUTs
- Control logic: 1.2K LUTs
Total: ~12-15K LUTs

Flip-Flop Usage:
- State register (128 bits): 128 FFs
- Round counter (4 bits): 4 FFs
- Status registers (8 bits): 8 FFs
- Control signals: 16 FFs
- Temporary storage: 1024 FFs
Total: ~1.2-1.5K FFs

Block RAM:
- S-box (combinational): 0 BRAM
- Round keys (not stored, generated): 0 BRAM
- Temporary buffers: 0 BRAM
Total: 0 BRAM

Total Area (28nm): ~1.5-2.0 mm²
*/

// ============================================================================
// Multi-block Chaining Example (CBC Mode)
// ============================================================================

/*
To implement CBC mode with the hardware accelerator:

void aes_cbc_encrypt(aes_ctx_t *ctx,
                     const uint8_t *plaintext,
                     size_t length,
                     uint8_t *ciphertext,
                     const uint8_t init_vector[16]) {

    uint8_t previous_block[16];

    // Copy IV for first block
    for (int i = 0; i < 16; i++) {
        previous_block[i] = init_vector[i];
    }

    // Process each 128-bit block
    for (size_t block = 0; block < length; block += 16) {
        uint8_t temp[16];

        // XOR plaintext with previous ciphertext (or IV)
        for (int i = 0; i < 16; i++) {
            temp[i] = plaintext[block + i] ^ previous_block[i];
        }

        // Encrypt combined block
        aes_encrypt(ctx, temp, ciphertext + block);

        // Save this ciphertext for next iteration
        for (int i = 0; i < 16; i++) {
            previous_block[i] = ciphertext[block + i];
        }
    }
}

void aes_cbc_decrypt(aes_ctx_t *ctx,
                     const uint8_t *ciphertext,
                     size_t length,
                     uint8_t *plaintext,
                     const uint8_t init_vector[16]) {

    uint8_t previous_block[16];

    // Copy IV
    for (int i = 0; i < 16; i++) {
        previous_block[i] = init_vector[i];
    }

    // Process each block
    for (size_t block = 0; block < length; block += 16) {
        uint8_t decrypted[16];

        // Decrypt the ciphertext block
        aes_decrypt(ctx, ciphertext + block, decrypted);

        // XOR with previous ciphertext block
        for (int i = 0; i < 16; i++) {
            plaintext[block + i] = decrypted[i] ^ previous_block[i];
        }

        // Update previous ciphertext block
        for (int i = 0; i < 16; i++) {
            previous_block[i] = ciphertext[block + i];
        }
    }
}
*/

// ============================================================================
// DMA Controller Integration (Advanced)
// ============================================================================

/*
For high-performance data processing, consider adding DMA support:

module aes_with_dma (
    input  wire         clk,
    input  wire         rst,

    // Standard MMIO interface for control
    input  wire [31:0]  ctrl_addr,
    input  wire [31:0]  ctrl_write_data,
    output wire [31:0]  ctrl_read_data,
    input  wire         ctrl_write_en,
    input  wire         ctrl_read_en,

    // DMA Master interface (optional)
    output wire [31:0]  dma_read_addr,
    input  wire [31:0]  dma_read_data,
    output wire         dma_read_en,

    output wire [31:0]  dma_write_addr,
    output wire [31:0]  dma_write_data,
    output wire         dma_write_en,

    output wire         aes_irq
);
    // ... DMA logic ...
endmodule

With DMA, bulk encryption can be performed without CPU intervention,
allowing parallel processing and higher throughput.
*/
