#include "admin_ops.h"
#include "auth.h"
#include "utils/crypto_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cJSON *handle_admin_analytics(const UserRecord *caller) {
    if (caller && caller->role > ROLE_HUB_MANAGER) {
        // Limited customer view or access check
    }

    cJSON *analytics = db_get_admin_analytics_json();
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddItemToObject(response, "data", analytics);
    return response;
}

cJSON *handle_admin_hub_list(const UserRecord *caller) {
    (void)caller;
    cJSON *hubs = db_get_all_hubs_json();
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddItemToObject(response, "hubs", hubs);
    return response;
}

cJSON *handle_admin_status_override(const UserRecord *caller, const char *shipment_id, const char *new_status_str, const char *remarks, double lat, double lon, const char *location_tag) {
    cJSON *response = cJSON_CreateObject();
    if (!caller || !auth_has_permission(caller, ROLE_HUB_MANAGER)) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Unauthorized: Insufficient privileges to override status.");
        return response;
    }

    if (!shipment_id || !new_status_str) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Missing required parameters.");
        return response;
    }

    ShipmentStatus status = string_to_status(new_status_str);
    int res = db_update_shipment_status(shipment_id, status, remarks, caller->id, lat, lon, location_tag);
    if (res != 0) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Shipment record not found.");
        return response;
    }

    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Shipment status updated and audited successfully.");
    return response;
}

cJSON *handle_admin_agent_dispatch(const UserRecord *caller, const char *shipment_id, const char *agent_id) {
    cJSON *response = cJSON_CreateObject();
    if (!caller || !auth_has_permission(caller, ROLE_HUB_MANAGER)) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Unauthorized: Requires manager or admin role.");
        return response;
    }

    int res = db_assign_shipment_agent(shipment_id, agent_id, caller->id);
    if (res != 0) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Failed to assign delivery agent.");
        return response;
    }

    // Set to OUT_FOR_DELIVERY
    UserRecord *agent = db_find_user_by_id(agent_id);
    char rem[256];
    snprintf(rem, sizeof(rem), "Assigned to delivery agent: %.200s", agent ? agent->full_name : "Agent");
    db_update_shipment_status(shipment_id, STATUS_OUT_FOR_DELIVERY, rem, caller->id, 13.0827, 80.2707, "Local Delivery Hub");

    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Agent successfully assigned. Shipment status moved to OUT_FOR_DELIVERY.");
    return response;
}

cJSON *handle_admin_user_list(const UserRecord *caller) {
    (void)caller;
    cJSON *response = cJSON_CreateObject();
    cJSON *users = db_get_all_users_json();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddItemToObject(response, "users", users);
    return response;
}

cJSON *handle_admin_user_create(const UserRecord *caller, const char *email, const char *name, const char *role_str, const char *phone, const char *hub_id, const char *password) {
    (void)caller;
    cJSON *response = cJSON_CreateObject();
    if (!email || !name) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Email and name required.");
        return response;
    }

    UserRecord new_user;
    memset(&new_user, 0, sizeof(new_user));
    snprintf(new_user.id, sizeof(new_user.id), "u%08X", (unsigned int)rand());
    snprintf(new_user.email, sizeof(new_user.email), "%s", email);
    snprintf(new_user.full_name, sizeof(new_user.full_name), "%s", name);
    new_user.role = string_to_role(role_str ? role_str : "standard_customer");
    snprintf(new_user.role_name, sizeof(new_user.role_name), "%s", role_to_string(new_user.role));
    if (phone) snprintf(new_user.phone, sizeof(new_user.phone), "%s", phone);
    if (hub_id) snprintf(new_user.allocated_hub_id, sizeof(new_user.allocated_hub_id), "%s", hub_id);
    snprintf(new_user.api_key, sizeof(new_user.api_key), "crm_%s_%04X", role_str ? role_str : "usr", rand() % 0xFFFF);
    new_user.is_active = true;

    char hash[128];
    crypto_hash_password(password ? password : "Admin@123", "crmgmt_salt", hash, sizeof(hash));
    snprintf(new_user.password_hash, sizeof(new_user.password_hash), "%s", hash);

    int res = db_create_user(&new_user);
    if (res != 0) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Failed to insert user or email exists.");
        return response;
    }

    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "User provisioned successfully.");
    return response;
}

cJSON *handle_admin_audit_logs(const UserRecord *caller, int limit) {
    cJSON *response = cJSON_CreateObject();
    if (!caller || !auth_has_permission(caller, ROLE_HUB_MANAGER)) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Unauthorized.");
        return response;
    }

    cJSON *logs = db_get_audit_logs_json(limit);
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddItemToObject(response, "logs", logs);
    return response;
}
