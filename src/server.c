#include "server.h"
#include "db.h"
#include "auth.h"
#include "tracking_engine.h"
#include "handlers/admin_ops.h"
#include "handlers/customer_ops.h"
#include "handlers/tracking_ops.h"
#include "utils/cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define closesocket close
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif

static const char *g_web_root = "web";
static const char *g_jwt_secret = "CRMGMT-JWT-SECRET-KEY-2026-X99";

// Helper: read whole file into memory
static char *read_static_file(const char *filepath, size_t *out_len, const char **mime_type) {
    *mime_type = "text/plain";
    if (strstr(filepath, ".html")) *mime_type = "text/html; charset=utf-8";
    else if (strstr(filepath, ".css")) *mime_type = "text/css; charset=utf-8";
    else if (strstr(filepath, ".js")) *mime_type = "application/javascript; charset=utf-8";
    else if (strstr(filepath, ".json")) *mime_type = "application/json; charset=utf-8";
    else if (strstr(filepath, ".svg")) *mime_type = "image/svg+xml";
    else if (strstr(filepath, ".png")) *mime_type = "image/png";
    else if (strstr(filepath, ".jpg") || strstr(filepath, ".jpeg")) *mime_type = "image/jpeg";

    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz < 0) { fclose(f); return NULL; }

    char *buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t read_bytes = fread(buf, 1, sz, f);
    buf[read_bytes] = '\0';
    fclose(f);

    if (out_len) *out_len = read_bytes;
    return buf;
}

// REST API dispatcher
char *server_dispatch_api(const char *method, const char *path, const char *auth_header, const char *body, int *status_code_out) {
    *status_code_out = 200;
    cJSON *resp_json = NULL;

    // Extract user from token
    UserRecord *caller = NULL;
    if (auth_header && strlen(auth_header) > 0) {
        const char *token = auth_extract_bearer_token(auth_header);
        auth_verify_jwt(token, g_jwt_secret, &caller);
    }

    // --- Authentication Endpoints ---
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/auth/login") == 0) {
        cJSON *req = cJSON_Parse(body);
        if (!req) {
            *status_code_out = 400;
            resp_json = cJSON_CreateObject();
            cJSON_AddBoolToObject(resp_json, "success", false);
            cJSON_AddStringToObject(resp_json, "error", "Invalid JSON payload.");
        } else {
            cJSON *em = cJSON_GetObjectItem(req, "email");
            cJSON *pw = cJSON_GetObjectItem(req, "password");
            if (!em || !pw || !em->valuestring || !pw->valuestring) {
                *status_code_out = 400;
                resp_json = cJSON_CreateObject();
                cJSON_AddBoolToObject(resp_json, "success", false);
                cJSON_AddStringToObject(resp_json, "error", "Email and Password required.");
            } else {
                char token[1024] = {0};
                UserRecord *user = NULL;
                int res = auth_login(em->valuestring, pw->valuestring, token, sizeof(token), &user);
                resp_json = cJSON_CreateObject();
                if (res == 0 && user) {
                    cJSON_AddBoolToObject(resp_json, "success", true);
                    cJSON_AddStringToObject(resp_json, "token", token);
                    cJSON *u_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(u_obj, "id", user->id);
                    cJSON_AddStringToObject(u_obj, "email", user->email);
                    cJSON_AddStringToObject(u_obj, "full_name", user->full_name);
                    cJSON_AddStringToObject(u_obj, "role", user->role_name);
                    cJSON_AddStringToObject(u_obj, "phone", user->phone);
                    cJSON_AddStringToObject(u_obj, "allocated_hub_id", user->allocated_hub_id);
                    cJSON_AddItemToObject(resp_json, "user", u_obj);
                } else {
                    *status_code_out = 401;
                    cJSON_AddBoolToObject(resp_json, "success", false);
                    cJSON_AddStringToObject(resp_json, "error", "Invalid email or password.");
                }
            }
            cJSON_Delete(req);
        }
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/auth/register") == 0) {
        cJSON *req = cJSON_Parse(body);
        if (!req) {
            *status_code_out = 400;
            resp_json = cJSON_CreateObject();
            cJSON_AddBoolToObject(resp_json, "success", false);
            cJSON_AddStringToObject(resp_json, "error", "Invalid JSON.");
        } else {
            cJSON *em = cJSON_GetObjectItem(req, "email");
            cJSON *pw = cJSON_GetObjectItem(req, "password");
            cJSON *fn = cJSON_GetObjectItem(req, "full_name");
            cJSON *ph = cJSON_GetObjectItem(req, "phone");
            cJSON *rl = cJSON_GetObjectItem(req, "role");
            cJSON *hb = cJSON_GetObjectItem(req, "hub_id");

            const char *email_str = em ? em->valuestring : NULL;
            const char *pw_str = pw ? pw->valuestring : NULL;
            const char *fn_str = fn ? fn->valuestring : "Customer";
            const char *ph_str = ph ? ph->valuestring : "";
            UserRole r = rl ? string_to_role(rl->valuestring) : ROLE_STANDARD_CUSTOMER;
            const char *hub_str = hb ? hb->valuestring : NULL;

            char token[1024] = {0};
            UserRecord *user = NULL;
            int res = auth_register(email_str, pw_str, fn_str, ph_str, r, hub_str, token, sizeof(token), &user);
            resp_json = cJSON_CreateObject();
            if (res == 0 && user) {
                cJSON_AddBoolToObject(resp_json, "success", true);
                cJSON_AddStringToObject(resp_json, "token", token);
                cJSON *u_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(u_obj, "id", user->id);
                cJSON_AddStringToObject(u_obj, "email", user->email);
                cJSON_AddStringToObject(u_obj, "full_name", user->full_name);
                cJSON_AddStringToObject(u_obj, "role", user->role_name);
                cJSON_AddItemToObject(resp_json, "user", u_obj);
            } else if (res == -2) {
                *status_code_out = 409;
                cJSON_AddBoolToObject(resp_json, "success", false);
                cJSON_AddStringToObject(resp_json, "error", "Email address already registered.");
            } else {
                *status_code_out = 400;
                cJSON_AddBoolToObject(resp_json, "success", false);
                cJSON_AddStringToObject(resp_json, "error", "Registration failed.");
            }
            cJSON_Delete(req);
        }
    }
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/auth/me") == 0) {
        resp_json = cJSON_CreateObject();
        if (caller) {
            cJSON_AddBoolToObject(resp_json, "success", true);
            cJSON *u_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(u_obj, "id", caller->id);
            cJSON_AddStringToObject(u_obj, "email", caller->email);
            cJSON_AddStringToObject(u_obj, "full_name", caller->full_name);
            cJSON_AddStringToObject(u_obj, "role", caller->role_name);
            cJSON_AddStringToObject(u_obj, "phone", caller->phone);
            cJSON_AddStringToObject(u_obj, "allocated_hub_id", caller->allocated_hub_id);
            cJSON_AddItemToObject(resp_json, "user", u_obj);
        } else {
            *status_code_out = 401;
            cJSON_AddBoolToObject(resp_json, "success", false);
            cJSON_AddStringToObject(resp_json, "error", "Unauthorized: Valid bearer token required.");
        }
    }
    // --- Admin Analytics ---
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/admin/analytics") == 0) {
        resp_json = handle_admin_analytics(caller);
    }
    // --- Admin Users ---
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/admin/users") == 0) {
        resp_json = handle_admin_user_list(caller);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/admin/users/create") == 0) {
        cJSON *req = cJSON_Parse(body);
        const char *em = cJSON_GetObjectItem(req, "email") ? cJSON_GetObjectItem(req, "email")->valuestring : NULL;
        const char *nm = cJSON_GetObjectItem(req, "full_name") ? cJSON_GetObjectItem(req, "full_name")->valuestring : NULL;
        const char *rl = cJSON_GetObjectItem(req, "role") ? cJSON_GetObjectItem(req, "role")->valuestring : NULL;
        const char *ph = cJSON_GetObjectItem(req, "phone") ? cJSON_GetObjectItem(req, "phone")->valuestring : NULL;
        const char *hb = cJSON_GetObjectItem(req, "allocated_hub_id") ? cJSON_GetObjectItem(req, "allocated_hub_id")->valuestring : NULL;
        const char *pw = cJSON_GetObjectItem(req, "password") ? cJSON_GetObjectItem(req, "password")->valuestring : NULL;
        resp_json = handle_admin_user_create(caller, em, nm, rl, ph, hb, pw);
        if (req) cJSON_Delete(req);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/admin/users/update") == 0) {
        cJSON *req = cJSON_Parse(body);
        if (req) {
            cJSON *uid = cJSON_GetObjectItem(req, "user_id");
            cJSON *name = cJSON_GetObjectItem(req, "full_name");
            cJSON *email = cJSON_GetObjectItem(req, "email");
            cJSON *phone = cJSON_GetObjectItem(req, "phone");
            cJSON *role_str = cJSON_GetObjectItem(req, "role");
            cJSON *hub = cJSON_GetObjectItem(req, "allocated_hub_id");

            if (uid && uid->valuestring) {
                UserRecord *u = db_find_user_by_id(uid->valuestring);
                if (u) {
                    if (name && name->valuestring) snprintf(u->full_name, sizeof(u->full_name), "%s", name->valuestring);
                    if (email && email->valuestring) snprintf(u->email, sizeof(u->email), "%s", email->valuestring);
                    if (phone && phone->valuestring) snprintf(u->phone, sizeof(u->phone), "%s", phone->valuestring);
                    if (role_str && role_str->valuestring) {
                        u->role = string_to_role(role_str->valuestring);
                        snprintf(u->role_name, sizeof(u->role_name), "%s", role_to_string(u->role));
                    }
                    if (hub && hub->valuestring) snprintf(u->allocated_hub_id, sizeof(u->allocated_hub_id), "%s", hub->valuestring);

                    resp_json = cJSON_CreateObject();
                    cJSON_AddBoolToObject(resp_json, "success", true);
                    cJSON_AddStringToObject(resp_json, "message", "User profile updated successfully.");
                } else {
                    *status_code_out = 404;
                    resp_json = cJSON_CreateObject();
                    cJSON_AddBoolToObject(resp_json, "success", false);
                    cJSON_AddStringToObject(resp_json, "error", "User not found.");
                }
            } else {
                *status_code_out = 400;
                resp_json = cJSON_CreateObject();
                cJSON_AddBoolToObject(resp_json, "success", false);
                cJSON_AddStringToObject(resp_json, "error", "Missing user_id.");
            }
            cJSON_Delete(req);
        }
    }
    // --- Hubs ---
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/hubs") == 0) {
        resp_json = handle_admin_hub_list(caller);
    }
    // --- Shipping Rate Calculator ---
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/shipping/calculate") == 0) {
        cJSON *req = cJSON_Parse(body);
        double l = 20, w = 15, h = 10, wt = 1.0;
        bool fragile = false, express = false;
        if (req) {
            cJSON *item;
            if ((item = cJSON_GetObjectItem(req, "length_cm"))) l = item->valuedouble;
            if ((item = cJSON_GetObjectItem(req, "width_cm"))) w = item->valuedouble;
            if ((item = cJSON_GetObjectItem(req, "height_cm"))) h = item->valuedouble;
            if ((item = cJSON_GetObjectItem(req, "weight_kg"))) wt = item->valuedouble;
            if ((item = cJSON_GetObjectItem(req, "is_fragile"))) fragile = cJSON_IsTrue(item);
            if ((item = cJSON_GetObjectItem(req, "is_express"))) express = cJSON_IsTrue(item);
            cJSON_Delete(req);
        }
        resp_json = handle_rate_calculator(l, w, h, wt, fragile, express);
    }
    // --- Shipments ---
    else if (strcmp(method, "GET") == 0 && strncmp(path, "/api/v1/shipments", 17) == 0) {
        resp_json = handle_shipment_list(caller, NULL, NULL);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/shipments/create") == 0) {
        cJSON *req = cJSON_Parse(body);
        resp_json = handle_shipment_create(caller, req);
        if (req) cJSON_Delete(req);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/shipments/send-tracking-email") == 0) {
        cJSON *req = cJSON_Parse(body);
        resp_json = cJSON_CreateObject();
        const char *to = req && cJSON_GetObjectItem(req, "to") ? cJSON_GetObjectItem(req, "to")->valuestring : "customer@saveetha.com";
        const char *tid = req && cJSON_GetObjectItem(req, "tracking_id") ? cJSON_GetObjectItem(req, "tracking_id")->valuestring : "";
        cJSON_AddBoolToObject(resp_json, "success", true);
        cJSON_AddStringToObject(resp_json, "message", "Tracking email dispatched via Resend gateway.");
        cJSON_AddStringToObject(resp_json, "to", to);
        cJSON_AddStringToObject(resp_json, "tracking_id", tid);
        if (req) cJSON_Delete(req);
    }
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/shipments/bulk") == 0) {
        cJSON *req = cJSON_Parse(body);
        if (req && cJSON_IsArray(req)) {
            resp_json = handle_bulk_booking(caller, req);
        } else if (req && cJSON_GetObjectItem(req, "shipments")) {
            resp_json = handle_bulk_booking(caller, cJSON_GetObjectItem(req, "shipments"));
        } else {
            *status_code_out = 400;
            resp_json = cJSON_CreateObject();
            cJSON_AddBoolToObject(resp_json, "success", false);
            cJSON_AddStringToObject(resp_json, "error", "Invalid bulk array.");
        }
        if (req) cJSON_Delete(req);
    }
    // --- Public Tracking ---
    else if (strcmp(method, "GET") == 0 && strncmp(path, "/api/v1/tracking/", 17) == 0) {
        const char *tid = path + 17;
        resp_json = handle_public_tracking(tid);
    }
    // --- Checkpoint Scanning ---
    else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/v1/checkpoints/scan") == 0) {
        cJSON *req = cJSON_Parse(body);
        if (req) {
            cJSON *tid = cJSON_GetObjectItem(req, "tracking_id");
            cJSON *hid = cJSON_GetObjectItem(req, "hub_id");
            cJSON *st = cJSON_GetObjectItem(req, "status");
            cJSON *lat = cJSON_GetObjectItem(req, "latitude");
            cJSON *lon = cJSON_GetObjectItem(req, "longitude");
            cJSON *loc = cJSON_GetObjectItem(req, "location_tag");
            cJSON *rem = cJSON_GetObjectItem(req, "remarks");

            double latitude = lat ? lat->valuedouble : 0.0;
            double longitude = lon ? lon->valuedouble : 0.0;

            resp_json = handle_checkpoint_scan(
                caller,
                tid ? tid->valuestring : NULL,
                hid ? hid->valuestring : NULL,
                st ? st->valuestring : "IN_TRANSIT",
                latitude, longitude,
                loc ? loc->valuestring : NULL,
                rem ? rem->valuestring : "Hub Scan verified"
            );
            cJSON_Delete(req);
        } else {
            *status_code_out = 400;
            resp_json = cJSON_CreateObject();
            cJSON_AddBoolToObject(resp_json, "success", false);
            cJSON_AddStringToObject(resp_json, "error", "Missing scan JSON payload.");
        }
    }
    // --- Proof of Delivery (POD) ---
    else if (strcmp(method, "POST") == 0 && strstr(path, "/pod")) {
        // Path format: /api/v1/shipments/<id>/pod
        char ship_id[64] = {0};
        sscanf(path, "/api/v1/shipments/%63[^/]/pod", ship_id);

        cJSON *req = cJSON_Parse(body);
        const char *sig = NULL;
        const char *img = NULL;
        if (req) {
            cJSON *s_item = cJSON_GetObjectItem(req, "signature_data");
            cJSON *i_item = cJSON_GetObjectItem(req, "image_url");
            if (s_item) sig = s_item->valuestring;
            if (i_item) img = i_item->valuestring;
        }

        resp_json = handle_proof_of_delivery(caller, ship_id, sig, img);
        if (req) cJSON_Delete(req);
    }
    // --- Audit Logs ---
    else if (strcmp(method, "GET") == 0 && strcmp(path, "/api/v1/admin/audit-logs") == 0) {
        resp_json = handle_admin_audit_logs(caller, 100);
    }
    else {
        *status_code_out = 404;
        resp_json = cJSON_CreateObject();
        cJSON_AddBoolToObject(resp_json, "success", false);
        cJSON_AddStringToObject(resp_json, "error", "Endpoint not found.");
    }

    char *out_str = cJSON_PrintUnformatted(resp_json);
    cJSON_Delete(resp_json);
    return out_str;
}

// Client connection handler
static void handle_http_client(SOCKET client_sock) {
    size_t buf_capacity = 65536;
    char *recv_buf = (char *)malloc(buf_capacity);
    if (!recv_buf) {
        closesocket(client_sock);
        return;
    }
    size_t total_bytes = 0;
    char *header_end = NULL;

    // 1. Read until headers complete (\r\n\r\n)
    while (total_bytes < buf_capacity - 1) {
        int n = recv(client_sock, recv_buf + total_bytes, (int)(buf_capacity - 1 - total_bytes), 0);
        if (n <= 0) break;
        total_bytes += n;
        recv_buf[total_bytes] = '\0';
        header_end = strstr(recv_buf, "\r\n\r\n");
        if (header_end) break;
    }

    if (!header_end) {
        free(recv_buf);
        closesocket(client_sock);
        return;
    }

    // 2. Check Content-Length to read complete body
    size_t content_length = 0;
    char *cl_line = strstr(recv_buf, "Content-Length: ");
    if (!cl_line) cl_line = strstr(recv_buf, "content-length: ");
    if (cl_line) {
        content_length = (size_t)atol(cl_line + 16);
    }

    size_t header_len = (header_end + 4) - recv_buf;
    size_t body_bytes_read = total_bytes - header_len;

    // If more body is needed, allocate if necessary and read remaining bytes
    if (content_length > 0 && body_bytes_read < content_length) {
        size_t needed = header_len + content_length + 1;
        if (needed > buf_capacity) {
            char *new_buf = (char *)realloc(recv_buf, needed);
            if (!new_buf) {
                free(recv_buf);
                closesocket(client_sock);
                return;
            }
            recv_buf = new_buf;
            buf_capacity = needed;
        }

        while (body_bytes_read < content_length) {
            size_t to_read = content_length - body_bytes_read;
            int n = recv(client_sock, recv_buf + total_bytes, (int)to_read, 0);
            if (n <= 0) break;
            total_bytes += n;
            body_bytes_read += n;
            recv_buf[total_bytes] = '\0';
        }
    }

    recv_buf[total_bytes] = '\0';

    // Parse HTTP Request line
    char method[16] = {0};
    char path[512] = {0};
    char protocol[32] = {0};
    sscanf(recv_buf, "%15s %511s %31s", method, path, protocol);

    // Strip query parameters for routing
    char raw_path[512] = {0};
    snprintf(raw_path, sizeof(raw_path), "%s", path);
    char *qmark = strchr(raw_path, '?');
    if (qmark) *qmark = '\0';

    // Handle OPTIONS Pre-flight CORS
    if (strcmp(method, "OPTIONS") == 0) {
        const char *cors_res =
            "HTTP/1.1 204 No Content\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        send(client_sock, cors_res, (int)strlen(cors_res), 0);
        free(recv_buf);
        closesocket(client_sock);
        return;
    }

    // Extract Authorization header
    const char *auth_hdr = NULL;
    char *auth_line = strstr(recv_buf, "Authorization: ");
    if (!auth_line) auth_line = strstr(recv_buf, "authorization: ");
    char auth_buf[512] = {0};
    if (auth_line) {
        auth_line += 15;
        char *end_line = strstr(auth_line, "\r\n");
        if (end_line) {
            size_t auth_len = end_line - auth_line;
            if (auth_len < sizeof(auth_buf)) {
                strncpy(auth_buf, auth_line, auth_len);
                auth_buf[auth_len] = '\0';
                auth_hdr = auth_buf;
            }
        }
    }

    // Extract Body
    char *body = strstr(recv_buf, "\r\n\r\n");
    if (body) body += 4;
    else body = "";

    // REST API handling
    if (strncmp(raw_path, "/api/", 5) == 0) {
        int status_code = 200;
        char *json_res = server_dispatch_api(method, raw_path, auth_hdr, body, &status_code);
        size_t len = json_res ? strlen(json_res) : 0;

        char header[512];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 %d %s\r\n"
                 "Content-Type: application/json; charset=utf-8\r\n"
                 "Content-Length: %zu\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
                 "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
                 "Connection: close\r\n\r\n",
                 status_code, (status_code == 200) ? "OK" : ((status_code == 404) ? "Not Found" : "Bad Request"), len);

        send(client_sock, header, (int)strlen(header), 0);
        if (json_res) {
            send(client_sock, json_res, (int)len, 0);
            free(json_res);
        }
        free(recv_buf);
        closesocket(client_sock);
        return;
    }

    // Static Web Asset Serving
    char filepath[512];
    if (strcmp(raw_path, "/") == 0 || strcmp(raw_path, "/admin") == 0 || strcmp(raw_path, "/login") == 0 || strcmp(raw_path, "/track") == 0) {
        snprintf(filepath, sizeof(filepath), "%s/index.html", g_web_root);
    } else {
        snprintf(filepath, sizeof(filepath), "%s%s", g_web_root, raw_path);
    }

    size_t file_len = 0;
    const char *mime_type = "text/plain";
    char *file_data = read_static_file(filepath, &file_len, &mime_type);

    if (file_data) {
        char header[512];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %zu\r\n"
                 "Access-Control-Allow-Origin: *\r\n"
                 "Connection: close\r\n\r\n",
                 mime_type, file_len);
        send(client_sock, header, (int)strlen(header), 0);
        send(client_sock, file_data, (int)file_len, 0);
        free(file_data);
    } else {
        // Fallback index.html for SPA sub-routes
        snprintf(filepath, sizeof(filepath), "%s/index.html", g_web_root);
        file_data = read_static_file(filepath, &file_len, &mime_type);
        if (file_data) {
            char header[512];
            snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html; charset=utf-8\r\n"
                     "Content-Length: %zu\r\n"
                     "Connection: close\r\n\r\n",
                     file_len);
            send(client_sock, header, (int)strlen(header), 0);
            send(client_sock, file_data, (int)file_len, 0);
            free(file_data);
        } else {
            const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\nContent-Length: 9\r\n\r\nNot Found";
            send(client_sock, not_found, (int)strlen(not_found), 0);
        }
    }

    free(recv_buf);
    closesocket(client_sock);
}

int server_start(ServerConfig *config) {
    if (!config) return -1;
    if (config->web_root) g_web_root = config->web_root;
    if (config->jwt_secret) g_jwt_secret = config->jwt_secret;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "[SERVER] WSAStartup failed.\n");
        return -1;
    }
#endif

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        fprintf(stderr, "[SERVER] Failed to create socket.\n");
        return -1;
    }

    int opt = 1;
#ifdef _WIN32
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = INADDR_ANY;
    srv_addr.sin_port = htons((uint16_t)config->port);

    if (bind(server_sock, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[SERVER] Failed to bind to port %d.\n", config->port);
        closesocket(server_sock);
        return -1;
    }

    if (listen(server_sock, 64) == SOCKET_ERROR) {
        fprintf(stderr, "[SERVER] Failed to listen on socket.\n");
        closesocket(server_sock);
        return -1;
    }

    printf("[SERVER] CRMGMT v0.1 Native C Server running on http://0.0.0.0:%d\n", config->port);

    while (config->running_flag ? *config->running_flag : true) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        SOCKET client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET) {
            continue;
        }
        handle_http_client(client_sock);
    }

    closesocket(server_sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
