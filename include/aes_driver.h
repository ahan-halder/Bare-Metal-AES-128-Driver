/*
 * AES-128 Hardware Driver - Register Definitions and MMIO Macros
 * Bare-metal C driver for AES-128 encryption accelerator
 * No OS, no standard libraries
 */

#ifndef AES_128_DRIVER_H
#define AES_128_DRIVER_H

#include <stdint.h>

/* ============================================================================
 * Register Base Address Configuration
 * ============================================================================ */

// Default base address for AES-128 hardware
#ifndef AES_BASE_ADDR
#define AES_BASE_ADDR  0x80000000
#endif

/* ============================================================================
 * Register Map
 * ============================================================================ */

#define AES_REG_CTRL        (AES_BASE_ADDR + 0x00)   // Control Register (RW)
#define AES_REG_STATUS      (AES_BASE_ADDR + 0x04)   // Status Register (R)
#define AES_REG_KEY_0       (AES_BASE_ADDR + 0x08)   // Key[127:96] (W)
#define AES_REG_KEY_1       (AES_BASE_ADDR + 0x0C)   // Key[95:64] (W)
#define AES_REG_KEY_2       (AES_BASE_ADDR + 0x10)   // Key[63:32] (W)
#define AES_REG_KEY_3       (AES_BASE_ADDR + 0x14)   // Key[31:0] (W)
#define AES_REG_DATA_IN_0   (AES_BASE_ADDR + 0x18)   // Data In[127:96] (W)
#define AES_REG_DATA_IN_1   (AES_BASE_ADDR + 0x1C)   // Data In[95:64] (W)
#define AES_REG_DATA_IN_2   (AES_BASE_ADDR + 0x20)   // Data In[63:32] (W)
#define AES_REG_DATA_IN_3   (AES_BASE_ADDR + 0x24)   // Data In[31:0] (W)
#define AES_REG_DATA_OUT_0  (AES_BASE_ADDR + 0x28)   // Data Out[127:96] (R)
#define AES_REG_DATA_OUT_1  (AES_BASE_ADDR + 0x2C)   // Data Out[95:64] (R)
#define AES_REG_DATA_OUT_2  (AES_BASE_ADDR + 0x30)   // Data Out[63:32] (R)
#define AES_REG_DATA_OUT_3  (AES_BASE_ADDR + 0x34)   // Data Out[31:0] (R)

/* ============================================================================
 * CTRL Register Bit Fields
 * ============================================================================ */

#define AES_CTRL_START      0x01    // [0] Start encryption/decryption
#define AES_CTRL_MODE       0x02    // [1] Mode: 0=Encrypt, 1=Decrypt

/* ============================================================================
 * STATUS Register Bit Fields
 * ============================================================================ */

#define AES_STATUS_BUSY     0x01    // [0] Hardware is busy
#define AES_STATUS_DONE     0x02    // [1] Operation complete

/* ============================================================================
 * Inline MMIO Access Functions
 * ============================================================================ */

#ifndef AES_DRIVER_USE_EXTERNAL_MMIO
/**
 * Read from a 32-bit register
 */
static inline uint32_t aes_read_reg(uint32_t addr) {
    return *(volatile uint32_t *)(uintptr_t)addr;
}

/**
 * Write to a 32-bit register
 */
static inline void aes_write_reg(uint32_t addr, uint32_t value) {
    *(volatile uint32_t *)(uintptr_t)addr = value;
}
#else
uint32_t aes_read_reg(uint32_t addr);
void aes_write_reg(uint32_t addr, uint32_t value);
#endif

/**
 * Read 128-bit value from four 32-bit registers
 */
static inline void aes_read_128(uint32_t base_addr, uint8_t *output) {
    uint32_t val0 = aes_read_reg(base_addr + 0x00);
    uint32_t val1 = aes_read_reg(base_addr + 0x04);
    uint32_t val2 = aes_read_reg(base_addr + 0x08);
    uint32_t val3 = aes_read_reg(base_addr + 0x0C);

    // Store in big-endian format
    output[0]  = (val0 >> 24) & 0xFF;
    output[1]  = (val0 >> 16) & 0xFF;
    output[2]  = (val0 >> 8) & 0xFF;
    output[3]  = val0 & 0xFF;
    output[4]  = (val1 >> 24) & 0xFF;
    output[5]  = (val1 >> 16) & 0xFF;
    output[6]  = (val1 >> 8) & 0xFF;
    output[7]  = val1 & 0xFF;
    output[8]  = (val2 >> 24) & 0xFF;
    output[9]  = (val2 >> 16) & 0xFF;
    output[10] = (val2 >> 8) & 0xFF;
    output[11] = val2 & 0xFF;
    output[12] = (val3 >> 24) & 0xFF;
    output[13] = (val3 >> 16) & 0xFF;
    output[14] = (val3 >> 8) & 0xFF;
    output[15] = val3 & 0xFF;
}

/**
 * Write 128-bit value to four 32-bit registers
 */
static inline void aes_write_128(uint32_t base_addr, const uint8_t *input) {
    uint32_t val0 = ((uint32_t)input[0] << 24) | ((uint32_t)input[1] << 16) |
                    ((uint32_t)input[2] << 8) | input[3];
    uint32_t val1 = ((uint32_t)input[4] << 24) | ((uint32_t)input[5] << 16) |
                    ((uint32_t)input[6] << 8) | input[7];
    uint32_t val2 = ((uint32_t)input[8] << 24) | ((uint32_t)input[9] << 16) |
                    ((uint32_t)input[10] << 8) | input[11];
    uint32_t val3 = ((uint32_t)input[12] << 24) | ((uint32_t)input[13] << 16) |
                    ((uint32_t)input[14] << 8) | input[15];

    aes_write_reg(base_addr + 0x00, val0);
    aes_write_reg(base_addr + 0x04, val1);
    aes_write_reg(base_addr + 0x08, val2);
    aes_write_reg(base_addr + 0x0C, val3);
}

/* ============================================================================
 * AES Driver Data Structures
 * ============================================================================ */

/**
 * AES Context Structure
 */
typedef struct {
    uint8_t key[16];           // 128-bit key
    uint8_t plaintext[16];     // 128-bit plaintext/ciphertext input
    uint8_t ciphertext[16];    // 128-bit output
    uint32_t base_addr;        // Base address of AES hardware
    int mode;                  // 0=Encrypt, 1=Decrypt
} aes_ctx_t;

void aes_init(aes_ctx_t *ctx, uint32_t base_addr);
void aes_set_key(aes_ctx_t *ctx, const uint8_t *key);
int aes_encrypt(aes_ctx_t *ctx, const uint8_t *plaintext, uint8_t *ciphertext);
int aes_decrypt(aes_ctx_t *ctx, const uint8_t *ciphertext, uint8_t *plaintext);
uint32_t aes_get_status(aes_ctx_t *ctx);
int aes_is_busy(aes_ctx_t *ctx);
int aes_is_done(aes_ctx_t *ctx);

#endif // AES_128_DRIVER_H
