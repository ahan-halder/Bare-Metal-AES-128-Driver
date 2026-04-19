#include <Arduino.h>
#include <ctype.h>
#include "mbedtls/aes.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

static const uint8_t STATUS_LED_PIN = LED_BUILTIN;
static const uint8_t LED_ON_LEVEL = LOW;
static const uint8_t LED_OFF_LEVEL = HIGH;

static void printHex(const char *label, const uint8_t *data, size_t len) {
  Serial.print(label);
  Serial.print(": ");
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
    if (i + 1 != len) {
      Serial.print(' ');
    }
  }
  Serial.println();
}

static bool bufferEqual(const uint8_t *a, const uint8_t *b, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

static void setStatusLed(bool on) {
  digitalWrite(STATUS_LED_PIN, on ? LED_ON_LEVEL : LED_OFF_LEVEL);
}

static void flashStatusLed(uint8_t times, uint16_t onMs, uint16_t offMs) {
  for (uint8_t i = 0; i < times; i++) {
    setStatusLed(true);
    delay(onMs);
    setStatusLed(false);
    delay(offMs);
  }
}

static int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

static bool parseHexBytes(const String &line, uint8_t *output, size_t expectedLen) {
  size_t outIndex = 0;
  int highNibble = -1;

  for (size_t i = 0; i < line.length(); i++) {
    int value = hexValue(line[i]);
    if (value < 0) {
      continue;
    }

    if (highNibble < 0) {
      highNibble = value;
    } else {
      if (outIndex >= expectedLen) {
        return false;
      }
      output[outIndex++] = (uint8_t)((highNibble << 4) | value);
      highNibble = -1;
    }
  }

  return outIndex == expectedLen && highNibble < 0;
}

static bool aesEncryptBlock(const uint8_t key[16], const uint8_t input[16], uint8_t output[16]) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);

  int ret = mbedtls_aes_setkey_enc(&ctx, key, 128);
  if (ret == 0) {
    ret = mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, input, output);
  }

  mbedtls_aes_free(&ctx);
  return ret == 0;
}

static bool aesDecryptBlock(const uint8_t key[16], const uint8_t input[16], uint8_t output[16]) {
  mbedtls_aes_context ctx;
  mbedtls_aes_init(&ctx);

  int ret = mbedtls_aes_setkey_dec(&ctx, key, 128);
  if (ret == 0) {
    ret = mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, input, output);
  }

  mbedtls_aes_free(&ctx);
  return ret == 0;
}

static void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  E <key_hex> <plaintext_hex>   Encrypt 16-byte block");
  Serial.println("  D <key_hex> <ciphertext_hex>  Decrypt 16-byte block");
  Serial.println("  T                             Run built-in NIST test vector");
  Serial.println("  B [count]                     Run AES benchmark and print stats");
  Serial.println();
  Serial.println("Format example:");
  Serial.println("  E 2b7e151628aed2a6abf7158809cf4f3c 6bc1bee22e409f96e93d7e117393172a");
}

static bool runNistTest() {
  static const uint8_t key[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
  };

  static const uint8_t plaintext[16] = {
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
  };

  static const uint8_t expectedCipher[16] = {
    0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
    0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97
  };

  uint8_t ciphertext[16];
  uint8_t decrypted[16];

  Serial.println();
  Serial.println("Running built-in NIST test...");
  printHex("Key", key, 16);
  printHex("Plaintext", plaintext, 16);

  if (!aesEncryptBlock(key, plaintext, ciphertext)) {
    Serial.println("Encryption failed");
    return false;
  }

  printHex("Ciphertext", ciphertext, 16);
  printHex("Expected", expectedCipher, 16);
  Serial.println(bufferEqual(ciphertext, expectedCipher, 16) ? "Encrypt test: PASS" : "Encrypt test: FAIL");

  if (!aesDecryptBlock(key, ciphertext, decrypted)) {
    Serial.println("Decryption failed");
    return false;
  }

  printHex("Decrypted", decrypted, 16);
  Serial.println(bufferEqual(decrypted, plaintext, 16) ? "Decrypt test: PASS" : "Decrypt test: FAIL");
  return bufferEqual(ciphertext, expectedCipher, 16) && bufferEqual(decrypted, plaintext, 16);
}

static bool runBenchmark(uint32_t iterations) {
  static const uint8_t key[16] = {
    0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
  };

  static const uint8_t plaintext[16] = {
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
  };

  uint8_t ciphertext[16];
  uint8_t decrypted[16];
  volatile uint32_t checksum = 0;

  Serial.println();
  Serial.println("Running AES benchmark...");
  Serial.print("Iterations: ");
  Serial.println(iterations);
  Serial.print("CPU MHz: ");
  Serial.println(ESP.getCpuFreqMHz());
  Serial.print("Free heap before: ");
  Serial.println(ESP.getFreeHeap());

  uint32_t startUs = micros();
  for (uint32_t i = 0; i < iterations; i++) {
    if (!aesEncryptBlock(key, plaintext, ciphertext)) {
      Serial.println("Encryption benchmark failed");
      return false;
    }
    checksum ^= ciphertext[i % 16];
  }
  uint32_t encryptUs = micros() - startUs;

  startUs = micros();
  for (uint32_t i = 0; i < iterations; i++) {
    if (!aesDecryptBlock(key, ciphertext, decrypted)) {
      Serial.println("Decryption benchmark failed");
      return false;
    }
    checksum ^= decrypted[i % 16];
  }
  uint32_t decryptUs = micros() - startUs;

  Serial.print("Checksum: 0x");
  Serial.println((uint32_t)checksum, HEX);
  Serial.print("Free heap after: ");
  Serial.println(ESP.getFreeHeap());

  float encryptAvgUs = (float)encryptUs / (float)iterations;
  float decryptAvgUs = (float)decryptUs / (float)iterations;
  float encryptBlocksPerSec = 1000000.0f / encryptAvgUs;
  float decryptBlocksPerSec = 1000000.0f / decryptAvgUs;

  Serial.print("Encrypt total us: ");
  Serial.println(encryptUs);
  Serial.print("Encrypt avg us/block: ");
  Serial.println(encryptAvgUs, 3);
  Serial.print("Encrypt blocks/sec: ");
  Serial.println(encryptBlocksPerSec, 2);

  Serial.print("Decrypt total us: ");
  Serial.println(decryptUs);
  Serial.print("Decrypt avg us/block: ");
  Serial.println(decryptAvgUs, 3);
  Serial.print("Decrypt blocks/sec: ");
  Serial.println(decryptBlocksPerSec, 2);

  Serial.print("Encrypt throughput KB/s: ");
  Serial.println((encryptBlocksPerSec * 16.0f) / 1024.0f, 2);
  Serial.print("Decrypt throughput KB/s: ");
  Serial.println((decryptBlocksPerSec * 16.0f) / 1024.0f, 2);

  return true;
}

static void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) {
    return;
  }

  char command = toupper(line.charAt(0));
  line = line.substring(1);
  line.trim();

  if (command == 'T') {
    setStatusLed(true);
    bool ok = runNistTest();
    setStatusLed(false);
    flashStatusLed(ok ? 2 : 4, ok ? 120 : 160, 120);
    return;
  }

  if (command == 'B') {
    uint32_t iterations = 1000;
    if (line.length() > 0) {
      iterations = (uint32_t)line.toInt();
      if (iterations == 0) {
        iterations = 1000;
      }
    }

    setStatusLed(true);
    bool ok = runBenchmark(iterations);
    setStatusLed(false);
    flashStatusLed(ok ? 3 : 4, ok ? 80 : 160, 80);
    return;
  }

  int splitIndex = line.indexOf(' ');
  if (splitIndex < 0) {
    Serial.println("Error: expected two hex strings after the command.");
    printHelp();
    return;
  }

  String keyText = line.substring(0, splitIndex);
  String blockText = line.substring(splitIndex + 1);
  keyText.trim();
  blockText.trim();

  uint8_t key[16];
  uint8_t input[16];
  uint8_t output[16];

  if (!parseHexBytes(keyText, key, 16)) {
    Serial.println("Error: key must be 32 hex characters.");
    return;
  }

  if (!parseHexBytes(blockText, input, 16)) {
    Serial.println("Error: block must be 32 hex characters.");
    return;
  }

  printHex("Key", key, 16);
  printHex((command == 'E') ? "Plaintext" : "Ciphertext", input, 16);

  bool ok = false;
  setStatusLed(true);
  if (command == 'E') {
    ok = aesEncryptBlock(key, input, output);
  } else if (command == 'D') {
    ok = aesDecryptBlock(key, input, output);
  } else {
    setStatusLed(false);
    Serial.println("Error: unknown command. Use E, D, or T.");
    printHelp();
    return;
  }

  setStatusLed(false);

  if (!ok) {
    flashStatusLed(2, 150, 150);
    Serial.println("AES operation failed.");
    return;
  }

  printHex((command == 'E') ? "Ciphertext" : "Plaintext", output, 16);
  flashStatusLed(1, 220, 80);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(STATUS_LED_PIN, OUTPUT);
  setStatusLed(false);

  Serial.println();
  Serial.println("ESP32 AES-128 Serial Tool");
  Serial.println("-------------------------");
  printHelp();
}

void loop() {
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    handleCommand(line);
    Serial.println();
    Serial.println("Ready for next command.");
  }
}