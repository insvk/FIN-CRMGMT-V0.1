#ifndef ADMIN_OPS_H
#define ADMIN_OPS_H

#include "db.h"
#include "utils/cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

// Returns analytics dashboard data matching Reference Image 1 metrics
cJSON *handle_admin_analytics(const UserRecord *caller);

// Hub management
cJSON *handle_admin_hub_list(const UserRecord *caller);

// Manual status override with mandatory audit remarks
cJSON *handle_admin_status_override(const UserRecord *caller, const char *shipment_id, const char *new_status_str, const char *remarks, double lat, double lon, const char *location_tag);

// User Management
cJSON *handle_admin_user_list(const UserRecord *caller);
cJSON *handle_admin_user_create(const UserRecord *caller, const char *email, const char *name, const char *role_str, const char *phone, const char *hub_id, const char *password, const char *avatar_url);

// Retrieve system audit logs
cJSON *handle_admin_audit_logs(const UserRecord *caller, int limit);

#ifdef __cplusplus
}
#endif

#endif // ADMIN_OPS_H
