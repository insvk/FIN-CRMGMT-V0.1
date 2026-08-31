#ifndef AUTH_H
#define AUTH_H

#include "db.h"
#include "utils/cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

// Authenticate user with email and password, returning a JWT token
int auth_login(const char *email, const char *password, char *token_out, size_t token_len, UserRecord **user_out);

// Register a new user
int auth_register(const char *email, const char *password, const char *full_name, const char *phone, UserRole role, const char *hub_id, const char *avatar_url, char *token_out, size_t token_len, UserRecord **user_out);

// Generate JWT token for user
int auth_create_jwt(const UserRecord *user, const char *secret, char *token_out, size_t token_len);

// Verify JWT token and retrieve corresponding user
int auth_verify_jwt(const char *token, const char *secret, UserRecord **user_out);

// Extract bearer token from Authorization header string
const char *auth_extract_bearer_token(const char *auth_header);

// Check if user has required role
bool auth_has_permission(const UserRecord *user, UserRole minimum_role);

#ifdef __cplusplus
}
#endif

#endif // AUTH_H
