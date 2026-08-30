#ifndef CUSTOMER_OPS_H
#define CUSTOMER_OPS_H

#include "db.h"
#include "utils/cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create a single shipment booking
cJSON *handle_shipment_create(const UserRecord *caller, const cJSON *payload);

// Calculate volumetric weight and instant shipping tariff
cJSON *handle_rate_calculator(double length_cm, double width_cm, double height_cm, double actual_weight_kg, bool is_fragile, bool is_express);

// List shipments for customer or admin
cJSON *handle_shipment_list(const UserRecord *caller, const char *status, const char *hub_id);

// Bulk booking from parsed JSON array
cJSON *handle_bulk_booking(const UserRecord *caller, const cJSON *shipments_array);

#ifdef __cplusplus
}
#endif

#endif // CUSTOMER_OPS_H
