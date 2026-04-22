/*
 * AES-128 Hardware Testbench
 * Comprehensive test suite for validation
 * Compile with: gcc -o testbench testbench.c aes_driver.c -I../include
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define AES_DRIVER_USE_EXTERNAL_MMIO 1
#include "aes_driver.h"
#include "aes_test_vectors.h"

#define AES_BLOCK_BYTES 16
#define MAX_METRICS (NUM_TEST_VECTORS * 2)

typedef struct {
    char description[96];
    char mode[12];
    double elapsed_us;
    double throughput_mbps;
    int mismatched_bytes;
    int bit_errors;
    int passed;
} test_metric_t;

static test_metric_t g_metrics[MAX_METRICS];
static size_t g_metric_count = 0;
static int g_use_color = 1;

static double now_us(void) {
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) == TIME_UTC) {
        return ((double)ts.tv_sec * 1000000.0) + ((double)ts.tv_nsec / 1000.0);
    }

    return ((double)clock() * 1000000.0) / (double)CLOCKS_PER_SEC;
}

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

static void detect_color_support(void) {
    const char *no_color = getenv("NO_COLOR");
    if (no_color != NULL && no_color[0] != '\0') {
        g_use_color = 0;
    }
}

static int popcount_u8(uint8_t x) {
    int count = 0;
    while (x != 0) {
        count += (x & 1u) ? 1 : 0;
        x >>= 1;
    }
    return count;
}

static int count_bit_errors(const uint8_t *a, const uint8_t *b, size_t len) {
    int total = 0;
    for (size_t i = 0; i < len; i++) {
        total += popcount_u8((uint8_t)(a[i] ^ b[i]));
    }
    return total;
}

static int count_mismatched_bytes(const uint8_t *a, const uint8_t *b, size_t len) {
    int total = 0;
    for (size_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            total++;
        }
    }
    return total;
}

static int byte_to_bg_color(uint8_t value) {
    return 232 + (value * 23 / 255);
}

static void print_colored_byte(uint8_t value) {
    if (!g_use_color) {
        printf("%02x", value);
        return;
    }

    int bg = byte_to_bg_color(value);
    int fg = (value > 127) ? 16 : 231;
    printf("\x1b[48;5;%dm\x1b[38;5;%dm%02x\x1b[0m", bg, fg, value);
}

static void print_diff_cell(uint8_t actual, uint8_t expected) {
    int bit_delta = popcount_u8((uint8_t)(actual ^ expected));

    if (!g_use_color) {
        if (bit_delta == 0) {
            printf(" %02x", actual);
        } else if (bit_delta <= 4) {
            printf("~%02x", actual);
        } else {
            printf("!%02x", actual);
        }
        return;
    }

    if (bit_delta == 0) {
        printf("\x1b[48;5;22m\x1b[38;5;231m %02x\x1b[0m", actual);
    } else if (bit_delta <= 4) {
        printf("\x1b[48;5;178m\x1b[38;5;16m %02x\x1b[0m", actual);
    } else {
        printf("\x1b[48;5;160m\x1b[38;5;231m %02x\x1b[0m", actual);
    }
}

static double us_since(clock_t start, clock_t end) {
    return ((double)(end - start) * 1000000.0) / (double)CLOCKS_PER_SEC;
}

static double benchmark_encrypt_us(aes_ctx_t *ctx, const uint8_t *plaintext, int iterations) {
    uint8_t scratch[AES_BLOCK_BYTES];
    double start = now_us();
    double end;

    for (int i = 0; i < iterations; i++) {
        if (aes_encrypt(ctx, plaintext, scratch) != 0) {
            return 0.0;
        }
    }

    end = now_us();
    if (end <= start || iterations <= 0) {
        return 0.0;
    }

    return (end - start) / (double)iterations;
}

static double benchmark_decrypt_us(aes_ctx_t *ctx, const uint8_t *ciphertext, int iterations) {
    uint8_t scratch[AES_BLOCK_BYTES];
    double start = now_us();
    double end;

    for (int i = 0; i < iterations; i++) {
        if (aes_decrypt(ctx, ciphertext, scratch) != 0) {
            return 0.0;
        }
    }

    end = now_us();
    if (end <= start || iterations <= 0) {
        return 0.0;
    }

    return (end - start) / (double)iterations;
}

static void record_metric(
    const char *description,
    const char *mode,
    double elapsed_us,
    int mismatched_bytes,
    int bit_errors,
    int passed
) {
    test_metric_t *m;

    if (g_metric_count >= MAX_METRICS) {
        return;
    }

    m = &g_metrics[g_metric_count++];
    snprintf(m->description, sizeof(m->description), "%s", description);
    snprintf(m->mode, sizeof(m->mode), "%s", mode);
    m->elapsed_us = elapsed_us;
    m->throughput_mbps = (elapsed_us > 0.0) ? (128.0 / elapsed_us) : 0.0;
    m->mismatched_bytes = mismatched_bytes;
    m->bit_errors = bit_errors;
    m->passed = passed;
}

static void write_metrics_csv(const char *path) {
    FILE *fp = fopen(path, "w");

    if (!fp) {
        printf("[WARN] Could not write %s\n", path);
        return;
    }

    fprintf(fp, "test_id,mode,description,elapsed_us,throughput_mbps,mismatched_bytes,bit_errors,passed\n");
    for (size_t i = 0; i < g_metric_count; i++) {
        const test_metric_t *m = &g_metrics[i];
        fprintf(
            fp,
            "%zu,%s,\"%s\",%.3f,%.3f,%d,%d,%d\n",
            i + 1,
            m->mode,
            m->description,
            m->elapsed_us,
            m->throughput_mbps,
            m->mismatched_bytes,
            m->bit_errors,
            m->passed
        );
    }

    fclose(fp);
}

static void write_metrics_json(const char *path) {
    FILE *fp = fopen(path, "w");

    if (!fp) {
        printf("[WARN] Could not write %s\n", path);
        return;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"block_bytes\": %d,\n", AES_BLOCK_BYTES);
    fprintf(fp, "  \"metrics\": [\n");
    for (size_t i = 0; i < g_metric_count; i++) {
        const test_metric_t *m = &g_metrics[i];
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"test_id\": %zu,\n", i + 1);
        fprintf(fp, "      \"mode\": \"%s\",\n", m->mode);
        fprintf(fp, "      \"description\": \"%s\",\n", m->description);
        fprintf(fp, "      \"elapsed_us\": %.3f,\n", m->elapsed_us);
        fprintf(fp, "      \"throughput_mbps\": %.3f,\n", m->throughput_mbps);
        fprintf(fp, "      \"mismatched_bytes\": %d,\n", m->mismatched_bytes);
        fprintf(fp, "      \"bit_errors\": %d,\n", m->bit_errors);
        fprintf(fp, "      \"passed\": %d\n", m->passed);
        fprintf(fp, "    }%s\n", (i + 1 < g_metric_count) ? "," : "");
    }
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");

    fclose(fp);
}

static void print_block_matrix_colored(const char *label, const uint8_t *data) {
    printf("%s\n", label);
    for (int row = 0; row < 4; row++) {
        printf("  |");
        for (int col = 0; col < 4; col++) {
            printf(" ");
            print_colored_byte(data[row * 4 + col]);
        }
        printf(" |\n");
    }
}

static void print_diff_heatmap_matrix(const uint8_t *actual, const uint8_t *expected) {
    printf("Diff Heatmap (green=match, amber=partial, red=mismatch)\n");
    for (int row = 0; row < 4; row++) {
        printf("  |");
        for (int col = 0; col < 4; col++) {
            int idx = row * 4 + col;
            printf(" ");
            print_diff_cell(actual[idx], expected[idx]);
        }
        printf(" |\n");
    }
}

static void print_ascii_bar(const char *label, double value, double max_value) {
    const int bar_width = 40;
    int fill = 0;

    if (max_value > 0.0) {
        fill = (int)((value / max_value) * (double)bar_width);
        if (fill > bar_width) {
            fill = bar_width;
        }
    }

    printf("%-18s |", label);
    for (int i = 0; i < bar_width; i++) {
        putchar(i < fill ? '#' : '.');
    }
    printf("| %.3f\n", value);
}

static void print_benchmark_chart(double enc_avg_us, double dec_avg_us, double enc_avg_mbps, double dec_avg_mbps) {
    double max_latency = (enc_avg_us > dec_avg_us) ? enc_avg_us : dec_avg_us;
    double max_throughput = (enc_avg_mbps > dec_avg_mbps) ? enc_avg_mbps : dec_avg_mbps;

    printf("\nBenchmark Chart (ASCII)\n");
    printf("Latency (us per 16-byte block)\n");
    print_ascii_bar("Encrypt", enc_avg_us, max_latency);
    print_ascii_bar("Decrypt", dec_avg_us, max_latency);

    printf("Throughput (Mbps)\n");
    print_ascii_bar("Encrypt", enc_avg_mbps, max_throughput);
    print_ascii_bar("Decrypt", dec_avg_mbps, max_throughput);
}

static void print_efficiency_scorecard(void) {
    size_t enc_count = 0;
    size_t dec_count = 0;
    size_t pass_count = 0;
    size_t total_ops = g_metric_count;
    int total_mismatches = 0;
    int total_bit_errors = 0;
    double enc_us = 0.0;
    double dec_us = 0.0;
    double enc_mbps = 0.0;
    double dec_mbps = 0.0;

    for (size_t i = 0; i < g_metric_count; i++) {
        const test_metric_t *m = &g_metrics[i];
        total_mismatches += m->mismatched_bytes;
        total_bit_errors += m->bit_errors;
        if (m->passed) {
            pass_count++;
        }

        if (strcmp(m->mode, "enc") == 0) {
            enc_count++;
            enc_us += m->elapsed_us;
            enc_mbps += m->throughput_mbps;
        } else {
            dec_count++;
            dec_us += m->elapsed_us;
            dec_mbps += m->throughput_mbps;
        }
    }

    double enc_avg_us = enc_count ? (enc_us / (double)enc_count) : 0.0;
    double dec_avg_us = dec_count ? (dec_us / (double)dec_count) : 0.0;
    double enc_avg_mbps = enc_count ? (enc_mbps / (double)enc_count) : 0.0;
    double dec_avg_mbps = dec_count ? (dec_mbps / (double)dec_count) : 0.0;
    double avg_latency = (total_ops > 0) ? ((enc_us + dec_us) / (double)total_ops) : 0.0;
    double blocks_per_sec = (avg_latency > 0.0) ? (1000000.0 / avg_latency) : 0.0;
    double bytes_per_sec = blocks_per_sec * AES_BLOCK_BYTES;
    double pass_rate = (total_ops > 0) ? ((double)pass_count * 100.0 / (double)total_ops) : 0.0;
    double byte_accuracy = (total_ops > 0)
        ? (1.0 - ((double)total_mismatches / (double)(total_ops * AES_BLOCK_BYTES)))
        : 0.0;
    double normalized_quality = pass_rate * byte_accuracy;

    printf("\n========================================\n");
    printf("EFFICIENCY SCORECARD\n");
    printf("========================================\n");
    printf("Operations:               %zu\n", total_ops);
    printf("Pass Rate:                %.2f%%\n", pass_rate);
    printf("Mismatched Bytes:         %d\n", total_mismatches);
    printf("Bit Errors:               %d\n", total_bit_errors);
    printf("Avg Encrypt Latency:      %.3f us\n", enc_avg_us);
    printf("Avg Decrypt Latency:      %.3f us\n", dec_avg_us);
    printf("Avg Encrypt Throughput:   %.3f Mbps\n", enc_avg_mbps);
    printf("Avg Decrypt Throughput:   %.3f Mbps\n", dec_avg_mbps);
    printf("Blocks/sec (overall):     %.2f\n", blocks_per_sec);
    printf("Bytes/sec (overall):      %.2f\n", bytes_per_sec);
    printf("Normalized Quality:       %.2f%%\n", normalized_quality);
    printf("========================================\n");

    print_benchmark_chart(enc_avg_us, dec_avg_us, enc_avg_mbps, dec_avg_mbps);
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

static void mock_reset(void) {
    memset(mock_registers, 0, sizeof(mock_registers));
}

static void mock_store_reg32(uint32_t offset, uint32_t value) {
    for (int i = 3; i >= 0; i--) {
        mock_registers[offset + i] = value & 0xFF;
        value >>= 8;
    }
}

static uint32_t mock_load_reg32(uint32_t offset) {
    uint32_t value = 0;

    for (int i = 0; i < 4; i++) {
        value = (value << 8) | mock_registers[offset + i];
    }

    return value;
}

static void mock_store_block(uint32_t offset, const uint8_t *input) {
    for (int i = 0; i < 16; i++) {
        mock_registers[offset + i] = input[i];
    }
}

static void mock_load_block(uint32_t offset, uint8_t *output) {
    for (int i = 0; i < 16; i++) {
        output[i] = mock_registers[offset + i];
    }
}

static void print_block_matrix(const char *label, const uint8_t *data) {
    printf("%s\n", label);
    for (int row = 0; row < 4; row++) {
        printf("  |");
        for (int col = 0; col < 4; col++) {
            printf(" %02x", data[row * 4 + col]);
        }
        printf(" |\n");
    }
}

static void print_diff_map(const uint8_t *actual, const uint8_t *expected, size_t len) {
    printf("Diff       : ");
    for (size_t i = 0; i < len; i++) {
        printf("%c", actual[i] == expected[i] ? '.' : '^');
        if ((i + 1) % 4 == 0 && i + 1 < len) printf(" ");
    }
    printf("\n");
}

static const uint8_t *mock_find_expected_output(const uint8_t *key, const uint8_t *input, uint32_t ctrl_value) {
    for (size_t i = 0; i < NUM_TEST_VECTORS; i++) {
        const aes_test_vector_t *vec = &aes_test_vectors[i];

        if (memcmp(key, vec->key, 16) != 0) {
            continue;
        }

        if ((ctrl_value & AES_CTRL_MODE) == 0 && memcmp(input, vec->plaintext, 16) == 0) {
            return vec->ciphertext;
        }

        if ((ctrl_value & AES_CTRL_MODE) != 0 && memcmp(input, vec->ciphertext, 16) == 0) {
            return vec->plaintext;
        }
    }

    return NULL;
}

static void mock_complete_operation(uint32_t ctrl_value) {
    uint8_t key[16];
    uint8_t input[16];
    const uint8_t *expected_output;

    mock_load_block(0x08, key);
    mock_load_block(0x18, input);

    expected_output = mock_find_expected_output(key, input, ctrl_value);

    if (expected_output == NULL) {
        mock_store_reg32(0x04, 0x00);
        printf("[MOCK] No matching test vector for the requested AES transaction.\n");
        return;
    }

    mock_store_block(0x28, expected_output);
    mock_store_reg32(0x04, AES_STATUS_DONE);
}

/**
 * Override default MMIO read for simulation
 */
uint32_t aes_read_reg(uint32_t addr) {
    // Remove base address for simulation
    uint32_t offset = addr - AES_BASE_ADDR;

    if (offset >= sizeof(mock_registers)) {
        return 0;
    }

    return mock_load_reg32(offset);
}

/**
 * Override default MMIO write for simulation
 */
void aes_write_reg(uint32_t addr, uint32_t value) {
    // Remove base address for simulation
    uint32_t offset = addr - AES_BASE_ADDR;
    uint32_t written_value = value;

    if (offset >= sizeof(mock_registers)) {
        return;
    }

    mock_store_reg32(offset, written_value);

    // Simulate encryption operation
    if (offset == 0x00) {  // CTRL register
        mock_store_reg32(0x04, AES_STATUS_BUSY);
        if (written_value & AES_CTRL_START) {
            mock_complete_operation(written_value);
        } else {
            mock_store_reg32(0x04, 0x00);
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
    int mismatched_bytes;
    int bit_errors;
    int passed;
    double elapsed_us;
    const int benchmark_iters = 5000;

    printf("\n--- Testing: %s ---\n", test_vec->description);
    mock_reset();

    // Initialize AES context
    aes_init(&ctx, AES_BASE_ADDR);

    // Set key
    aes_set_key(&ctx, test_vec->key);
    print_hex("Key        ", test_vec->key, 16);
    print_block_matrix_colored("Key Matrix (Colorized)", test_vec->key);

    // Encrypt
    print_hex("Plaintext  ", test_vec->plaintext, 16);
    print_block_matrix_colored("Plaintext Matrix (Colorized)", test_vec->plaintext);
    result = aes_encrypt(&ctx, test_vec->plaintext, output);
    elapsed_us = 0.0;

    if (result != 0) {
        printf("ERROR: Encryption operation failed\n");
        record_metric(test_vec->description, "enc", elapsed_us, AES_BLOCK_BYTES, 128, 0);
        return -1;
    }

    // Print results
    print_hex("Ciphertext ", output, 16);
    print_hex("Expected   ", test_vec->ciphertext, 16);
    print_block_matrix("Ciphertext Matrix", output);
    print_block_matrix_colored("Ciphertext Matrix (Colorized)", output);
    print_block_matrix_colored("Expected Matrix (Colorized)", test_vec->ciphertext);
    print_diff_map(output, test_vec->ciphertext, 16);
    print_diff_heatmap_matrix(output, test_vec->ciphertext);

    mismatched_bytes = count_mismatched_bytes(output, test_vec->ciphertext, AES_BLOCK_BYTES);
    bit_errors = count_bit_errors(output, test_vec->ciphertext, AES_BLOCK_BYTES);
    passed = (mismatched_bytes == 0) ? 1 : 0;
    elapsed_us = benchmark_encrypt_us(&ctx, test_vec->plaintext, benchmark_iters);
    if (elapsed_us <= 0.0) {
        elapsed_us = us_since(clock(), clock());
    }
    printf("Latency(us): %.3f | Throughput(Mbps): %.3f | Byte Mismatch: %d | Bit Errors: %d\n",
           elapsed_us,
           elapsed_us > 0.0 ? (128.0 / elapsed_us) : 0.0,
           mismatched_bytes,
           bit_errors);
    record_metric(test_vec->description, "enc", elapsed_us, mismatched_bytes, bit_errors, passed);

    // Verify
    if (!passed) {
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
    int mismatched_bytes;
    int bit_errors;
    int passed;
    double elapsed_us;
    const int benchmark_iters = 5000;

    printf("\n--- Testing Decryption (Round-Trip): %s ---\n", test_vec->description);
    mock_reset();

    // Initialize AES context
    aes_init(&ctx, AES_BASE_ADDR);

    // Set key
    aes_set_key(&ctx, test_vec->key);
    print_hex("Key        ", test_vec->key, 16);
    print_block_matrix_colored("Key Matrix (Colorized)", test_vec->key);

    // Decrypt
    print_hex("Ciphertext ", test_vec->ciphertext, 16);
    print_block_matrix_colored("Ciphertext Matrix (Colorized)", test_vec->ciphertext);
    result = aes_decrypt(&ctx, test_vec->ciphertext, decrypted);
    elapsed_us = 0.0;

    if (result != 0) {
        printf("ERROR: Decryption operation failed\n");
        record_metric(test_vec->description, "dec", elapsed_us, AES_BLOCK_BYTES, 128, 0);
        return -1;
    }

    // Print results
    print_hex("Decrypted  ", decrypted, 16);
    print_hex("Expected   ", test_vec->plaintext, 16);
    print_block_matrix("Decrypted Matrix", decrypted);
    print_block_matrix_colored("Decrypted Matrix (Colorized)", decrypted);
    print_block_matrix_colored("Expected Matrix (Colorized)", test_vec->plaintext);
    print_diff_map(decrypted, test_vec->plaintext, 16);
    print_diff_heatmap_matrix(decrypted, test_vec->plaintext);

    mismatched_bytes = count_mismatched_bytes(decrypted, test_vec->plaintext, AES_BLOCK_BYTES);
    bit_errors = count_bit_errors(decrypted, test_vec->plaintext, AES_BLOCK_BYTES);
    passed = (mismatched_bytes == 0) ? 1 : 0;
    elapsed_us = benchmark_decrypt_us(&ctx, test_vec->ciphertext, benchmark_iters);
    if (elapsed_us <= 0.0) {
        elapsed_us = us_since(clock(), clock());
    }
    printf("Latency(us): %.3f | Throughput(Mbps): %.3f | Byte Mismatch: %d | Bit Errors: %d\n",
           elapsed_us,
           elapsed_us > 0.0 ? (128.0 / elapsed_us) : 0.0,
           mismatched_bytes,
           bit_errors);
    record_metric(test_vec->description, "dec", elapsed_us, mismatched_bytes, bit_errors, passed);

    // Verify
    if (!passed) {
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

    detect_color_support();

    printf("========================================\n");
    printf("AES-128 Hardware Accelerator Testbench\n");
    printf("========================================\n");
    printf("Visualization: colorized 4x4 matrices + byte-level diff heatmap\n");

    // Run encryption tests
    printf("\n=== ENCRYPTION TESTS ===\n");
    for (size_t i = 0; i < NUM_TEST_VECTORS; i++) {
        if (test_encryption(&aes_test_vectors[i]) == 0) {
            pass_count++;
        } else {
            fail_count++;
        }
    }

    // Run decryption tests
    printf("\n=== DECRYPTION TESTS ===\n");
    for (size_t i = 0; i < NUM_TEST_VECTORS; i++) {
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

    write_metrics_csv("sim/metrics.csv");
    write_metrics_json("sim/metrics.json");
    printf("Metrics CSV: sim/metrics.csv\n");
    printf("Metrics JSON: sim/metrics.json\n");

    print_efficiency_scorecard();

    return (fail_count == 0) ? 0 : 1;
}
