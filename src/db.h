#ifndef DB_H
#define DB_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "utils/cJSON.h"

#define MAX_UUID_LEN 64
#define MAX_STR_LEN 256
#define MAX_TEXT_LEN 2048

typedef enum {
    ROLE_SUPER_ADMIN = 0,
    ROLE_HUB_MANAGER,
    ROLE_DELIVERY_AGENT,
    ROLE_ENTERPRISE_CUSTOMER,
    ROLE_STANDARD_CUSTOMER
} UserRole;

typedef enum {
    STATUS_ORDER_CREATED = 0,
    STATUS_PICKED_UP,
    STATUS_IN_TRANSIT,
    STATUS_OUT_FOR_DELIVERY,
    STATUS_DELIVERED,
    STATUS_FAILED_ATTEMPT,
    STATUS_RETURNED,
    STATUS_EXCEPTION
} ShipmentStatus;

typedef enum {
    PAYMENT_UNPAID = 0,
    PAYMENT_PREPAID,
    PAYMENT_COD_PENDING,
    PAYMENT_COD_SETTLED,
    PAYMENT_REFUNDED
} PaymentStatus;

// Data models
typedef struct {
    char id[MAX_UUID_LEN];
    char hub_code[32];
    char hub_name[MAX_STR_LEN];
    double latitude;
    double longitude;
    char address[MAX_TEXT_LEN];
    int capacity;
    int current_load;
    bool is_active;
    char created_at[64];
} HubRecord;

typedef struct {
    char id[MAX_UUID_LEN];
    char email[MAX_STR_LEN];
    char password_hash[MAX_STR_LEN];
    UserRole role;
    char role_name[32];
    char full_name[MAX_STR_LEN];
    char phone[64];
    char allocated_hub_id[MAX_UUID_LEN];
    char api_key[MAX_STR_LEN];
    char avatar_url[MAX_TEXT_LEN];
    bool is_active;
    char created_at[64];
} UserRecord;

typedef struct {
    char id[MAX_UUID_LEN];
    char tracking_id[64];
    char sender_id[MAX_UUID_LEN];
    char sender_name[MAX_STR_LEN];
    char sender_phone[64];
    char sender_address[MAX_TEXT_LEN];
    char recipient_name[MAX_STR_LEN];
    char recipient_phone[64];
    char recipient_address[MAX_TEXT_LEN];
    char recipient_pincode[32];
    char origin_hub_id[MAX_UUID_LEN];
    char destination_hub_id[MAX_UUID_LEN];
    char assigned_agent_id[MAX_UUID_LEN];
    ShipmentStatus status;
    char status_name[32];
    double weight_kg;
    char dimensions_cm[64];
    double volumetric_weight_kg;
    double billable_weight_kg;
    double declared_value;
    double shipping_cost;
    PaymentStatus payment_status;
    char payment_status_name[32];
    bool is_fragile;
    bool is_hazardous;
    char estimated_delivery[64];
    char pod_signature_url[MAX_TEXT_LEN];
    char pod_image_url[MAX_TEXT_LEN];
    char created_at[64];
    char updated_at[64];
} ShipmentRecord;

typedef struct {
    char id[MAX_UUID_LEN];
    char shipment_id[MAX_UUID_LEN];
    char hub_id[MAX_UUID_LEN];
    char scanned_by_user_id[MAX_UUID_LEN];
    ShipmentStatus status;
    char status_name[32];
    double latitude;
    double longitude;
    char location_tag[MAX_STR_LEN];
    char remarks[MAX_TEXT_LEN];
    char timestamp[64];
} CheckpointRecord;

typedef struct {
    char id[MAX_UUID_LEN];
    char actor_id[MAX_UUID_LEN];
    char action_type[64];
    char entity_name[64];
    char entity_id[64];
    char ip_address[64];
    char payload_json[MAX_TEXT_LEN];
    char created_at[64];
} AuditRecord;

#ifdef __cplusplus
extern "C" {
#endif

// DB Lifecycle
int db_init(const char *conn_string);
void db_close(void);
bool db_is_connected(void);

// Hub Queries
cJSON *db_get_all_hubs_json(void);
HubRecord *db_find_hub_by_id(const char *hub_id);
HubRecord *db_find_hub_by_code(const char *hub_code);

// User Queries
cJSON *db_get_all_users_json(void);
UserRecord *db_find_user_by_email(const char *email);
UserRecord *db_find_user_by_id(const char *user_id);
UserRecord *db_find_user_by_api_key(const char *api_key);
int db_create_user(UserRecord *new_user);
int db_update_user_pfp(const char *user_id, const char *avatar_url);

// Shipment Queries
cJSON *db_get_shipments_json(const char *status_filter, const char *hub_filter, const char *customer_id, const char *agent_id);
ShipmentRecord *db_find_shipment_by_tracking_id(const char *tracking_id);
ShipmentRecord *db_find_shipment_by_id(const char *shipment_id);
int db_create_shipment(ShipmentRecord *shipment);
int db_update_shipment_status(const char *shipment_id, ShipmentStatus new_status, const char *remarks, const char *actor_id, double lat, double lon, const char *location_tag);
int db_assign_shipment_agent(const char *shipment_id, const char *agent_id, const char *actor_id);
int db_update_shipment_pod(const char *shipment_id, const char *signature_data, const char *image_url, const char *actor_id);

// Checkpoints
cJSON *db_get_checkpoints_for_shipment_json(const char *shipment_id);
int db_add_checkpoint(CheckpointRecord *checkpoint);

// Audit Trails
int db_log_audit(const char *actor_id, const char *action_type, const char *entity_name, const char *entity_id, const char *ip_address, const char *payload_json);
cJSON *db_get_audit_logs_json(int limit);

// Admin Analytics
cJSON *db_get_admin_analytics_json(void);

// Enum helpers
const char *role_to_string(UserRole role);
UserRole string_to_role(const char *str);
const char *status_to_string(ShipmentStatus status);
ShipmentStatus string_to_status(const char *str);
const char *payment_status_to_string(PaymentStatus payment);
PaymentStatus string_to_payment_status(const char *str);

#ifdef __cplusplus
}
#endif

#endif // DB_H
