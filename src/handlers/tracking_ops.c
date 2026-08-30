#include "tracking_ops.h"
#include "tracking_engine.h"
#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

cJSON *handle_public_tracking(const char *tracking_id) {
    cJSON *response = cJSON_CreateObject();
    if (!tracking_id || strlen(tracking_id) == 0) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Please provide a valid Tracking Number.");
        return response;
    }

    // Checksum verification
    int is_checksum_valid = verify_tracking_id_checksum(tracking_id);
    ShipmentRecord *s = db_find_shipment_by_tracking_id(tracking_id);

    if (!s) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddBoolToObject(response, "checksum_valid", is_checksum_valid ? true : false);
        cJSON_AddStringToObject(response, "error", "No shipment found matching this Tracking ID.");
        return response;
    }

    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddBoolToObject(response, "checksum_valid", true);

    cJSON *shipment_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(shipment_obj, "id", s->id);
    cJSON_AddStringToObject(shipment_obj, "tracking_id", s->tracking_id);
    cJSON_AddStringToObject(shipment_obj, "status", s->status_name);
    cJSON_AddStringToObject(shipment_obj, "sender_name", s->sender_name);
    cJSON_AddStringToObject(shipment_obj, "sender_city", "Chennai");
    cJSON_AddStringToObject(shipment_obj, "recipient_name", s->recipient_name);
    cJSON_AddStringToObject(shipment_obj, "recipient_address", s->recipient_address);
    cJSON_AddStringToObject(shipment_obj, "recipient_pincode", s->recipient_pincode);
    cJSON_AddNumberToObject(shipment_obj, "weight_kg", s->weight_kg);
    cJSON_AddStringToObject(shipment_obj, "dimensions_cm", s->dimensions_cm);
    cJSON_AddNumberToObject(shipment_obj, "shipping_cost", s->shipping_cost);
    cJSON_AddStringToObject(shipment_obj, "payment_status", s->payment_status_name);
    cJSON_AddBoolToObject(shipment_obj, "is_fragile", s->is_fragile);
    cJSON_AddStringToObject(shipment_obj, "estimated_delivery", s->estimated_delivery);
    cJSON_AddStringToObject(shipment_obj, "created_at", s->created_at);
    cJSON_AddStringToObject(shipment_obj, "updated_at", s->updated_at);
    if (strlen(s->pod_signature_url) > 0) {
        cJSON_AddStringToObject(shipment_obj, "pod_signature_url", s->pod_signature_url);
    }
    cJSON_AddItemToObject(response, "shipment", shipment_obj);

    // Origin & Destination Hubs info for map
    HubRecord *orig = db_find_hub_by_id(s->origin_hub_id);
    HubRecord *dest = db_find_hub_by_id(s->destination_hub_id);

    cJSON *route = cJSON_CreateObject();
    if (orig) {
        cJSON *orig_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(orig_obj, "name", orig->hub_name);
        cJSON_AddNumberToObject(orig_obj, "lat", orig->latitude);
        cJSON_AddNumberToObject(orig_obj, "lng", orig->longitude);
        cJSON_AddItemToObject(route, "origin", orig_obj);
    }
    if (dest) {
        cJSON *dest_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(dest_obj, "name", dest->hub_name);
        cJSON_AddNumberToObject(dest_obj, "lat", dest->latitude);
        cJSON_AddNumberToObject(dest_obj, "lng", dest->longitude);
        cJSON_AddItemToObject(route, "destination", dest_obj);
    }
    cJSON_AddItemToObject(response, "route", route);

    // Checkpoint history
    cJSON *checkpoints = db_get_checkpoints_for_shipment_json(s->id);
    cJSON_AddItemToObject(response, "checkpoints", checkpoints);

    return response;
}

cJSON *handle_checkpoint_scan(const UserRecord *caller, const char *tracking_id, const char *hub_id, const char *status_str, double lat, double lon, const char *location_tag, const char *remarks) {
    cJSON *response = cJSON_CreateObject();
    if (!caller || !auth_has_permission(caller, ROLE_DELIVERY_AGENT)) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Unauthorized: Delivery agent or hub manager login required.");
        return response;
    }

    if (!tracking_id || !status_str) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Missing tracking_id or status.");
        return response;
    }

    ShipmentRecord *s = db_find_shipment_by_tracking_id(tracking_id);
    if (!s) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Shipment tracking number not found.");
        return response;
    }

    ShipmentStatus new_status = string_to_status(status_str);
    HubRecord *hub = hub_id ? db_find_hub_by_id(hub_id) : NULL;
    if (hub && (lat == 0.0 || lon == 0.0)) {
        lat = hub->latitude;
        lon = hub->longitude;
    }

    const char *tag = location_tag;
    if (!tag && hub) tag = hub->hub_name;
    if (!tag) tag = "Logistics Transit Station";

    int res = db_update_shipment_status(s->id, new_status, remarks, caller->id, lat, lon, tag);
    if (res != 0) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Failed to update shipment status.");
        return response;
    }

    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Checkpoint scanned and status updated successfully.");
    cJSON_AddStringToObject(response, "tracking_id", s->tracking_id);
    cJSON_AddStringToObject(response, "new_status", status_str);
    return response;
}

cJSON *handle_proof_of_delivery(const UserRecord *caller, const char *shipment_id, const char *signature_base64, const char *image_url) {
    cJSON *response = cJSON_CreateObject();
    if (!caller || !auth_has_permission(caller, ROLE_DELIVERY_AGENT)) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Unauthorized.");
        return response;
    }

    if (!shipment_id || (!signature_base64 && !image_url)) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Missing required shipment ID or signature payload.");
        return response;
    }

    int res = db_update_shipment_pod(shipment_id, signature_base64, image_url, caller->id);
    if (res != 0) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Shipment record not found.");
        return response;
    }

    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Proof of Delivery (POD) signed successfully. Shipment marked as DELIVERED.");
    return response;
}
