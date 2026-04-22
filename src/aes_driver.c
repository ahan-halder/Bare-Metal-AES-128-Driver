/*
 * AES-128 Hardware Driver Implementation
 * Bare-metal C driver for AES-128 encryption accelerator
 */

#include "aes_driver.h"

static inline uint32_t aes_reg_addr(const aes_ctx_t *ctx, uint32_t offset) {
    return ctx->base_addr + offset;
}

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * Poll for hardware ready (not busy)
 * Returns 0 on success, -1 on timeout
 */
static int aes_wait_ready(aes_ctx_t *ctx) {
    uint32_t timeout = 1000000;  // Max iterations

    while (timeout--) {
        uint32_t status = aes_read_reg(aes_reg_addr(ctx, 0x04));
        if (!(status & AES_STATUS_BUSY)) {
            return 0;
        }
    }
    return -1;
}

/**
 * Poll for operation complete
 * Returns 0 on success, -1 on timeout
 */
static int aes_wait_done(aes_ctx_t *ctx) {
    uint32_t timeout = 1000000;  // Max iterations

    while (timeout--) {
        uint32_t status = aes_read_reg(aes_reg_addr(ctx, 0x04));
        if (status & AES_STATUS_DONE) {
            return 0;
        }
    }
    return -1;
}

/* ============================================================================
 * AES Driver API Functions
 * ============================================================================ */

/**
 * Initialize AES context
 */
void aes_init(aes_ctx_t *ctx, uint32_t base_addr) {
    ctx->base_addr = base_addr;
    ctx->mode = 0;  // Default: encrypt

    // Clear buffers
    for (int i = 0; i < 16; i++) {
        ctx->key[i] = 0;
        ctx->plaintext[i] = 0;
        ctx->ciphertext[i] = 0;
    }
}

/**
 * Set encryption key (128-bit)
 */
void aes_set_key(aes_ctx_t *ctx, const uint8_t *key) {
    // Copy key to context
    for (int i = 0; i < 16; i++) {
        ctx->key[i] = key[i];
    }

    // Write key to hardware registers
    aes_write_128(aes_reg_addr(ctx, 0x08), key);
}

/**
 * Encrypt a 128-bit block
 * Returns 0 on success, -1 on error
 */
int aes_encrypt(aes_ctx_t *ctx, const uint8_t *plaintext, uint8_t *ciphertext) {
    // Wait for hardware ready
    if (aes_wait_ready(ctx) != 0) {
        return -1;  // Timeout
    }

    ctx->mode = 0;

    // Copy plaintext to context
    for (int i = 0; i < 16; i++) {
        ctx->plaintext[i] = plaintext[i];
    }

    // Write plaintext to hardware
    aes_write_128(aes_reg_addr(ctx, 0x18), plaintext);

    // Set mode to encrypt (bit[1] = 0) and start (bit[0] = 1)
    uint32_t ctrl = AES_CTRL_START;  // Encrypt mode
    aes_write_reg(aes_reg_addr(ctx, 0x00), ctrl);

    // Wait for operation to complete
    if (aes_wait_done(ctx) != 0) {
        return -1;  // Timeout
    }

    // Read ciphertext from hardware
    aes_read_128(aes_reg_addr(ctx, 0x28), ciphertext);

    // Copy to context
    for (int i = 0; i < 16; i++) {
        ctx->ciphertext[i] = ciphertext[i];
    }

    return 0;
}

/**
 * Decrypt a 128-bit block
 * Returns 0 on success, -1 on error
 */
int aes_decrypt(aes_ctx_t *ctx, const uint8_t *ciphertext, uint8_t *plaintext) {
    // Wait for hardware ready
    if (aes_wait_ready(ctx) != 0) {
        return -1;  // Timeout
    }

    ctx->mode = 1;

    // Copy ciphertext to context
    for (int i = 0; i < 16; i++) {
        ctx->plaintext[i] = ciphertext[i];
    }

    // Write ciphertext to hardware
    aes_write_128(aes_reg_addr(ctx, 0x18), ciphertext);

    // Set mode to decrypt (bit[1] = 1) and start (bit[0] = 1)
    uint32_t ctrl = AES_CTRL_START | AES_CTRL_MODE;  // Decrypt mode
    aes_write_reg(aes_reg_addr(ctx, 0x00), ctrl);

    // Wait for operation to complete
    if (aes_wait_done(ctx) != 0) {
        return -1;  // Timeout
    }

    // Read plaintext from hardware
    aes_read_128(aes_reg_addr(ctx, 0x28), plaintext);

    // Copy to context
    for (int i = 0; i < 16; i++) {
        ctx->ciphertext[i] = plaintext[i];
    }

    return 0;
}

/**
 * Get hardware status
 */
uint32_t aes_get_status(aes_ctx_t *ctx) {
    return aes_read_reg(aes_reg_addr(ctx, 0x04));
}

/**
 * Check if hardware is busy
 */
int aes_is_busy(aes_ctx_t *ctx) {
    return (aes_read_reg(aes_reg_addr(ctx, 0x04)) & AES_STATUS_BUSY) != 0;
}

/**
 * Check if operation is done
 */
int aes_is_done(aes_ctx_t *ctx) {
    return (aes_read_reg(aes_reg_addr(ctx, 0x04)) & AES_STATUS_DONE) != 0;
}
