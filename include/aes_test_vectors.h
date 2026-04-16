/*
 * Standard AES-128 Test Vectors from NIST
 * Used for validation and testing of hardware implementation
 */

#ifndef AES_TEST_VECTORS_H
#define AES_TEST_VECTORS_H

#include <stdint.h>

/* ============================================================================
 * Test Vector Structure
 * ============================================================================ */

typedef struct {
    const uint8_t *key;
    const uint8_t *plaintext;
    const uint8_t *ciphertext;
    const char *description;
} aes_test_vector_t;

/* ============================================================================
 * NIST Test Vector 1: ECB Mode
 * ============================================================================ */

static const uint8_t nist_key_1[] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static const uint8_t nist_plaintext_1[] = {
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
};

static const uint8_t nist_ciphertext_1[] = {
    0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
    0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
};

/* ============================================================================
 * Test Vector 2
 * ============================================================================ */

static const uint8_t nist_key_2[] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static const uint8_t nist_plaintext_2[] = {
    0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
    0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
};

static const uint8_t nist_ciphertext_2[] = {
    0xe2, 0xd1, 0xff, 0xf5, 0x82, 0xc3, 0xd0, 0xad,
    0x05, 0xdbcc, 0x72, 0x39, 0x36, 0xd4, 0x70, 0x38
};

/* ============================================================================
 * Test Vector 3
 * ============================================================================ */

static const uint8_t nist_key_3[] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static const uint8_t nist_plaintext_3[] = {
    0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
    0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef
};

static const uint8_t nist_ciphertext_3[] = {
    0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
    0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x3c, 0x01
};

/* ============================================================================
 * Test Vector Array
 * ============================================================================ */

static const aes_test_vector_t aes_test_vectors[] = {
    {
        .key = nist_key_1,
        .plaintext = nist_plaintext_1,
        .ciphertext = nist_ciphertext_1,
        .description = "NIST Test Vector 1"
    },
    {
        .key = nist_key_2,
        .plaintext = nist_plaintext_2,
        .ciphertext = nist_ciphertext_2,
        .description = "NIST Test Vector 2"
    },
    {
        .key = nist_key_3,
        .plaintext = nist_plaintext_3,
        .ciphertext = nist_ciphertext_3,
        .description = "NIST Test Vector 3"
    }
};

#define NUM_TEST_VECTORS (sizeof(aes_test_vectors) / sizeof(aes_test_vectors[0]))

#endif // AES_TEST_VECTORS_H
