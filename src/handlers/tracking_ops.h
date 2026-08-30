#ifndef TRACKING_OPS_H
#define TRACKING_OPS_H

#include "db.h"
#include "utils/cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

// Public tracking lookup by tracking ID
cJSON *handle_public_tracking(const char *tracking_id);

// Checkpoint scanner (Hub scan-in, status advance, GPS tagging)
cJSON *handle_checkpoint_scan(const UserRecord *caller, const char *tracking_id, const char *hub_id, const char *status_str, double lat, double lon, const char *location_tag, const char *remarks);

// Proof of Delivery (POD) signature upload
cJSON *handle_proof_of_delivery(const UserRecord *caller, const char *shipment_id, const char *signature_base64, const char *image_url);

#ifdef __cplusplus
}
#endif

#endif // TRACKING_OPS_H
