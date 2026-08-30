#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int port;
    const char *db_url;
    const char *jwt_secret;
    const char *web_root;
    volatile bool *running_flag;
} ServerConfig;

// Initialize and start the HTTP server event loop
int server_start(ServerConfig *config);

// Request dispatch helper for unit tests or embedded calls
char *server_dispatch_api(const char *method, const char *path, const char *auth_header, const char *body, int *status_code_out);

#ifdef __cplusplus
}
#endif

#endif // SERVER_H
