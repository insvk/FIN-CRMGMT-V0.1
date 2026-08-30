#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stddef.h>
#include <stdint.h>

#define SHA256_HASH_SIZE 32

#ifdef __cplusplus
extern "C" {
#endif

// SHA256 and HMAC-SHA256 calculation
void crypto_sha256(const uint8_t *data, size_t len, uint8_t output[SHA256_HASH_SIZE]);
void crypto_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t output[SHA256_HASH_SIZE]);

// Hex encoding/decoding
void crypto_bytes_to_hex(const uint8_t *bytes, size_t len, char *hex_output);
int crypto_hex_to_bytes(const char *hex, uint8_t *bytes, size_t max_len);

// Base64 & Base64URL encoding/decoding
int crypto_base64_encode(const uint8_t *data, size_t len, char *output, size_t max_len);
int crypto_base64_decode(const char *input, uint8_t *output, size_t max_len);
int crypto_base64url_encode(const uint8_t *data, size_t len, char *output, size_t max_len);
int crypto_base64url_decode(const char *input, uint8_t *output, size_t max_len);

// Password hashing & verification (PBKDF2-HMAC-SHA256)
void crypto_hash_password(const char *password, const char *salt, char *output_hash, size_t max_len);
int crypto_verify_password(const char *password, const char *stored_hash);

// Random token generation
void crypto_generate_random_hex(char *output, size_t byte_count);

#ifdef __cplusplus
}
#endif

#endif // CRYPTO_UTILS_H
