#ifndef TRACKING_ENGINE_H
#define TRACKING_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Generates cryptographically verifiable Tracking ID in format: CR-XXXXXXXX-XX-XXXX
void generate_special_tracking_id(char *output_buffer, size_t buffer_len);

// Verifies integrity and checksum of a Tracking ID without requiring database lookup
int verify_tracking_id_checksum(const char *tracking_id);

#ifdef __cplusplus
}
#endif

#endif // TRACKING_ENGINE_H
