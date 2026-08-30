#include "auth.h"
#include "utils/crypto_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_JWT_SECRET "CRMGMT-JWT-SECRET-KEY-2026-X99"

int auth_create_jwt(const UserRecord *user, const char *secret, char *token_out, size_t token_len) {
    if (!user || !token_out || token_len < 128) return -1;
    const char *jwt_sec = (secret && strlen(secret) > 0) ? secret : DEFAULT_JWT_SECRET;

    // Header: {"alg":"HS256","typ":"JWT"}
    const char *header_json = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    char header_b64[64] = {0};
    crypto_base64url_encode((const uint8_t*)header_json, strlen(header_json), header_b64, sizeof(header_b64));

    // Payload: {"sub":"...", "email":"...", "role":"...", "name":"...", "exp":...}
    time_t now = time(NULL);
    time_t exp = now + (7 * 24 * 3600); // 7 days validity

    char payload_json[512];
    snprintf(payload_json, sizeof(payload_json),
             "{\"sub\":\"%s\",\"email\":\"%s\",\"role\":\"%s\",\"name\":\"%s\",\"exp\":%ld}",
             user->id, user->email, user->role_name, user->full_name, (long)exp);

    char payload_b64[768] = {0};
    crypto_base64url_encode((const uint8_t*)payload_json, strlen(payload_json), payload_b64, sizeof(payload_b64));

    // Unsigned token: header.payload
    char unsigned_token[1024];
    snprintf(unsigned_token, sizeof(unsigned_token), "%s.%s", header_b64, payload_b64);

    // Signature: HMAC-SHA256(unsigned_token, secret)
    uint8_t hmac[32];
    crypto_hmac_sha256((const uint8_t*)jwt_sec, strlen(jwt_sec),
                       (const uint8_t*)unsigned_token, strlen(unsigned_token), hmac);

    char sig_b64[64] = {0};
    crypto_base64url_encode(hmac, 32, sig_b64, sizeof(sig_b64));

    // Final token
    snprintf(token_out, token_len, "%s.%s", unsigned_token, sig_b64);
    return 0;
}

int auth_verify_jwt(const char *token, const char *secret, UserRecord **user_out) {
    if (!token || !user_out) return -1;
    *user_out = NULL;
    const char *jwt_sec = (secret && strlen(secret) > 0) ? secret : DEFAULT_JWT_SECRET;

    char token_copy[1024];
    snprintf(token_copy, sizeof(token_copy), "%s", token);

    char *dot1 = strchr(token_copy, '.');
    if (!dot1) return -1;
    char *dot2 = strchr(dot1 + 1, '.');
    if (!dot2) return -1;

    *dot1 = '\0';
    *dot2 = '\0';

    const char *header_b64 = token_copy;
    const char *payload_b64 = dot1 + 1;
    const char *sig_b64 = dot2 + 1;

    // Verify signature
    char unsigned_token[1024];
    snprintf(unsigned_token, sizeof(unsigned_token), "%s.%s", header_b64, payload_b64);

    uint8_t expected_hmac[32];
    crypto_hmac_sha256((const uint8_t*)jwt_sec, strlen(jwt_sec),
                       (const uint8_t*)unsigned_token, strlen(unsigned_token), expected_hmac);

    char expected_sig_b64[64] = {0};
    crypto_base64url_encode(expected_hmac, 32, expected_sig_b64, sizeof(expected_sig_b64));

    if (strcmp(sig_b64, expected_sig_b64) != 0) {
        return -2; // Signature invalid
    }

    // Decode payload
    uint8_t payload_json[512] = {0};
    crypto_base64url_decode(payload_b64, payload_json, sizeof(payload_json) - 1);

    cJSON *payload = cJSON_Parse((char*)payload_json);
    if (!payload) return -3;

    cJSON *sub_item = cJSON_GetObjectItem(payload, "sub");
    cJSON *email_item = cJSON_GetObjectItem(payload, "email");
    cJSON *exp_item = cJSON_GetObjectItem(payload, "exp");

    if (!sub_item || !exp_item) {
        cJSON_Delete(payload);
        return -4;
    }

    time_t now = time(NULL);
    if (exp_item->valuedouble < (double)now) {
        cJSON_Delete(payload);
        return -5; // Token expired
    }

    UserRecord *u = db_find_user_by_id(sub_item->valuestring);
    if (!u && email_item) {
        u = db_find_user_by_email(email_item->valuestring);
    }

    cJSON_Delete(payload);
    if (!u) return -6;

    *user_out = u;
    return 0;
}

int auth_login(const char *email, const char *password, char *token_out, size_t token_len, UserRecord **user_out) {
    if (!email || !password || !token_out || !user_out) return -1;
    *user_out = NULL;

    // Check API master key bypass
    UserRecord *key_user = db_find_user_by_api_key(password);
    if (key_user) {
        *user_out = key_user;
        return auth_create_jwt(key_user, NULL, token_out, token_len);
    }

    UserRecord *user = db_find_user_by_email(email);
    if (!user) {
        return -1; // User not found
    }

    if (!crypto_verify_password(password, user->password_hash)) {
        return -2; // Invalid password
    }

    *user_out = user;
    return auth_create_jwt(user, NULL, token_out, token_len);
}

int auth_register(const char *email, const char *password, const char *full_name, const char *phone, UserRole role, const char *hub_id, char *token_out, size_t token_len, UserRecord **user_out) {
    if (!email || !password || !full_name || !user_out) return -1;
    *user_out = NULL;

    if (db_find_user_by_email(email)) {
        return -2; // Email already registered
    }

    UserRecord new_user;
    memset(&new_user, 0, sizeof(new_user));

    snprintf(new_user.email, sizeof(new_user.email), "%s", email);
    crypto_hash_password(password, "crmgmt_salt", new_user.password_hash, sizeof(new_user.password_hash));
    new_user.role = role;
    snprintf(new_user.full_name, sizeof(new_user.full_name), "%s", full_name);
    if (phone) snprintf(new_user.phone, sizeof(new_user.phone), "%s", phone);
    if (hub_id) snprintf(new_user.allocated_hub_id, sizeof(new_user.allocated_hub_id), "%s", hub_id);

    char rand_key[33];
    crypto_generate_random_hex(rand_key, 16);
    snprintf(new_user.api_key, sizeof(new_user.api_key), "crm_key_%s", rand_key);

    if (db_create_user(&new_user) != 0) {
        return -3;
    }

    UserRecord *created = db_find_user_by_email(email);
    *user_out = created;

    if (token_out && token_len > 0) {
        auth_create_jwt(created, NULL, token_out, token_len);
    }
    return 0;
}

const char *auth_extract_bearer_token(const char *auth_header) {
    if (!auth_header) return NULL;
    if (strncasecmp(auth_header, "Bearer ", 7) == 0) {
        return auth_header + 7;
    }
    return auth_header;
}

bool auth_has_permission(const UserRecord *user, UserRole minimum_role) {
    if (!user) return false;
    if (user->role == ROLE_SUPER_ADMIN) return true;
    return user->role <= minimum_role;
}
