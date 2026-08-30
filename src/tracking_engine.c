#include "tracking_engine.h"
#include "utils/crypto_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TRACKING_SECRET_KEY "CRMGMT-SECRET-SALT-2026"

void generate_special_tracking_id(char *output_buffer, size_t buffer_len) {
    if (!output_buffer || buffer_len < 24) return;
    
    uint32_t ts = (uint32_t)time(NULL);
    uint16_t salt = (uint16_t)(rand() % 65535);
    
    char raw_payload[96];
    snprintf(raw_payload, sizeof(raw_payload), "%08X%04X-%s", ts, salt, TRACKING_SECRET_KEY);
    
    uint8_t hash[32];
    crypto_sha256((const uint8_t*)raw_payload, strlen(raw_payload), hash);
    
    // Checksum byte derived from SHA256 prefix XOR
    uint8_t checksum = hash[0] ^ hash[1];
    
    snprintf(output_buffer, buffer_len, "CR-%08X-%02X-%04X", ts, checksum, salt);
}

int verify_tracking_id_checksum(const char *tracking_id) {
    if (!tracking_id || strlen(tracking_id) < 18) {
        return 0;
    }
    
    uint32_t ts = 0;
    unsigned int checksum = 0;
    unsigned int salt = 0;
    
    if (sscanf(tracking_id, "CR-%08X-%02X-%04X", &ts, &checksum, &salt) != 3 &&
        sscanf(tracking_id, "cr-%08x-%02x-%04x", &ts, &checksum, &salt) != 3) {
        return 0; // Malformed tracking format
    }
    
    char raw_payload[96];
    snprintf(raw_payload, sizeof(raw_payload), "%08X%04X-%s", ts, (uint16_t)salt, TRACKING_SECRET_KEY);
    
    uint8_t hash[32];
    crypto_sha256((const uint8_t*)raw_payload, strlen(raw_payload), hash);
    uint8_t expected_checksum = hash[0] ^ hash[1];
    
    return (checksum == expected_checksum);
}
