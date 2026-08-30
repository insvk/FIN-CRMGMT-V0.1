#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "server.h"
#include "db.h"
#include "tracking_engine.h"

static volatile bool g_keep_running = true;

static void handle_signal(int sig) {
    (void)sig;
    printf("\n[CRMGMT] Intercepted shutdown signal. Gracefully stopping server...\n");
    g_keep_running = false;
}

int main(int argc, char *argv[]) {
    srand((unsigned int)time(NULL));

    int port = 8080;
    const char *db_url = getenv("SUPABASE_DB_URL");
    if (!db_url || strlen(db_url) == 0) {
        db_url = getenv("DATABASE_URL");
    }
    const char *jwt_secret = getenv("JWT_SECRET");
    const char *web_root = "web";

    // Read port from environment if provided (Render sets PORT)
    const char *env_port = getenv("PORT");
    if (env_port && strlen(env_port) > 0) {
        port = atoi(env_port);
    }

    // Parse CLI arguments
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--db") == 0) && i + 1 < argc) {
            db_url = argv[++i];
        } else if ((strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--web") == 0) && i + 1 < argc) {
            web_root = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("CRMGMT v0.1 - Native C Courier Management Microservice\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("  -p, --port <port>       Port to listen on (default: 8080 or $PORT)\n");
            printf("  -d, --db <url>          Supabase / PostgreSQL connection string\n");
            printf("  -w, --web <dir>         Web root directory (default: web)\n");
            printf("  -h, --help              Show this help menu\n");
            return 0;
        }
    }

    printf("========================================================\n");
    printf("        CRMGMT v0.1 - Enterprise Logistics Engine        \n");
    printf("        Runtime: C17 / Non-blocking Multi-threaded       \n");
    printf("        Auth & Database: Supabase PostgreSQL 15          \n");
    printf("========================================================\n");

    // Initialize Database
    if (db_init(db_url) != 0) {
        fprintf(stderr, "[CRMGMT] Database initialization warning.\n");
    }

    // Register signal handlers
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    ServerConfig config;
    config.port = port;
    config.db_url = db_url;
    config.jwt_secret = jwt_secret ? jwt_secret : "CRMGMT-JWT-SECRET-KEY-2026-X99";
    config.web_root = web_root;
    config.running_flag = &g_keep_running;

    int res = server_start(&config);

    printf("[CRMGMT] Cleaning up resources...\n");
    db_close();
    printf("[CRMGMT] Server stopped cleanly. Goodbye!\n");
    return res;
}
