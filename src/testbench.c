/*
 * AES-128 Hardware Testbench
 * Comprehensive test suite for validation
 * Compile with: gcc -o testbench testbench.c aes_driver.c -I../include
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aes_driver.h"
#include "aes_test_vectors.h"

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * Print a 16-byte buffer as hexadecimal
 */
void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
        if ((i + 1) % 4 == 0 && i + 1 < len) printf(" ");
    }
    printf("\n");
}

/**
 * Compare two buffers
 * Returns 0 if equal, -1 if different
 */
int compare_buffers(const uint8_t *a, const uint8_t *b, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return -1;
        }
    }
    return 0;
}

/* ============================================================================
 * Mock Hardware Interface (for Verilator)
 * ============================================================================ */

// Simulated hardware register file
static uint8_t mock_registers[256];

/**
 * Override default MMIO read for simulation
 */
uint32_t aes_read_reg(uint32_t addr) {
    // Remove base address for simulation
    uint32_t offset = addr - AES_BASE_ADDR;

    if (offset >= sizeof(mock_registers)) {
        return 0;
    }

    // Read from simulated register file
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value = (value << 8) | mock_registers[offset + i];
    }
    return value;
}

/**
 * Override default MMIO write for simulation
 */
void aes_write_reg(uint32_t addr, uint32_t value) {
    // Remove base address for simulation
    uint32_t offset = addr - AES_BASE_ADDR;

    if (offset >= sizeof(mock_registers)) {
        return;
    }

    // Write to simulated register file
    for (int i = 3; i >= 0; i--) {
        mock_registers[offset + i] = value & 0xFF;
        value >>= 8;
    }

    // Simulate encryption operation
    if (offset == 0x00) {  // CTRL register
        if (value & AES_CTRL_START) {
            // Trigger simulated HW operation
            // In real Verilator, this would call the RTL simulation
            // For now, mark as done
            mock_registers[0x04] = (AES_STATUS_DONE << 0);
        }
    }
}

/* ============================================================================
 * Test Functions
 * ============================================================================ */

/**
 * Test encryption with a single test vector
 */
int test_encryption(const aes_test_vector_t *test_vec) {
    aes_ctx_t ctx;
    uint8_t output[16];
    int result;

    printf("\n--- Testing: %s ---\n", test_vec->description);

    // Initialize AES context
    aes_init(&ctx, AES_BASE_ADDR);

    // Set key
    aes_set_key(&ctx, test_vec->key);
    print_hex("Key        ", test_vec->key, 16);

    // Encrypt
    print_hex("Plaintext  ", test_vec->plaintext, 16);
    result = aes_encrypt(&ctx, test_vec->plaintext, output);

    if (result != 0) {
        printf("ERROR: Encryption operation failed\n");
        return -1;
    }

    // Print results
    print_hex("Ciphertext ", output, 16);
    print_hex("Expected   ", test_vec->ciphertext, 16);

    // Verify
    if (compare_buffers(output, test_vec->ciphertext, 16) != 0) {
        printf("FAILED: Ciphertext mismatch\n");
        return -1;
    }

    printf("PASSED\n");
    return 0;
}

/**
 * Test decryption (round-trip)
 */
int test_decryption(const aes_test_vector_t *test_vec) {
    aes_ctx_t ctx;
    uint8_t decrypted[16];
    int result;

    printf("\n--- Testing Decryption (Round-Trip): %s ---\n", test_vec->description);

    // Initialize AES context
    aes_init(&ctx, AES_BASE_ADDR);

    // Set key
    aes_set_key(&ctx, test_vec->key);

    // Decrypt
    print_hex("Ciphertext ", test_vec->ciphertext, 16);
    result = aes_decrypt(&ctx, test_vec->ciphertext, decrypted);

    if (result != 0) {
        printf("ERROR: Decryption operation failed\n");
        return -1;
    }

    // Print results
    print_hex("Decrypted  ", decrypted, 16);
    print_hex("Expected   ", test_vec->plaintext, 16);

    // Verify
    if (compare_buffers(decrypted, test_vec->plaintext, 16) != 0) {
        printf("FAILED: Decrypted plaintext mismatch\n");
        return -1;
    }

    printf("PASSED\n");
    return 0;
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================ */

int main(void) {
    int pass_count = 0;
    int fail_count = 0;

    printf("========================================\n");
    printf("AES-128 Hardware Accelerator Testbench\n");
    printf("========================================\n");

    // Run encryption tests
    printf("\n=== ENCRYPTION TESTS ===\n");
    for (int i = 0; i < NUM_TEST_VECTORS; i++) {
        if (test_encryption(&aes_test_vectors[i]) == 0) {
            pass_count++;
        } else {
            fail_count++;
        }
    }

    // Run decryption tests
    printf("\n=== DECRYPTION TESTS ===\n");
    for (int i = 0; i < NUM_TEST_VECTORS; i++) {
        if (test_decryption(&aes_test_vectors[i]) == 0) {
            pass_count++;
        } else {
            fail_count++;
        }
    }

    // Summary
    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total Tests: %d\n", pass_count + fail_count);
    printf("Passed:      %d\n", pass_count);
    printf("Failed:      %d\n", fail_count);
    printf("Result:      %s\n", fail_count == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    printf("========================================\n");

    return (fail_count == 0) ? 0 : 1;
}
