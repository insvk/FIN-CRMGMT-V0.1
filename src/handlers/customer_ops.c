#include "customer_ops.h"
#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

cJSON *handle_rate_calculator(double length_cm, double width_cm, double height_cm, double actual_weight_kg, bool is_fragile, bool is_express) {
    if (length_cm <= 0) length_cm = 10.0;
    if (width_cm <= 0) width_cm = 10.0;
    if (height_cm <= 0) height_cm = 10.0;
    if (actual_weight_kg <= 0) actual_weight_kg = 0.5;

    double vol_weight = (length_cm * width_cm * height_cm) / 5000.0;
    double billable_weight = (actual_weight_kg > vol_weight) ? actual_weight_kg : vol_weight;

    double base_tariff = 150.0;
    double weight_cost = billable_weight * 50.0;
    double fragile_surcharge = is_fragile ? 75.0 : 0.0;
    double express_surcharge = is_express ? 150.0 : 0.0;

    double total_cost = base_tariff + weight_cost + fragile_surcharge + express_surcharge;

    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "actual_weight_kg", actual_weight_kg);
    cJSON_AddNumberToObject(response, "volumetric_weight_kg", round(vol_weight * 100.0) / 100.0);
    cJSON_AddNumberToObject(response, "billable_weight_kg", round(billable_weight * 100.0) / 100.0);
    cJSON_AddNumberToObject(response, "base_tariff", base_tariff);
    cJSON_AddNumberToObject(response, "shipping_cost", round(total_cost * 100.0) / 100.0);
    cJSON_AddStringToObject(response, "currency", "INR");
    cJSON_AddStringToObject(response, "estimated_transit_time", is_express ? "24-48 Hours" : "3-5 Business Days");
    return response;
}

cJSON *handle_shipment_create(const UserRecord *caller, const cJSON *payload) {
    cJSON *response = cJSON_CreateObject();
    if (!payload) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Invalid JSON payload.");
        return response;
    }

    ShipmentRecord s;
    memset(&s, 0, sizeof(s));

    if (caller) {
        snprintf(s.sender_id, sizeof(s.sender_id), "%s", caller->id);
    }

    cJSON *item = NULL;
    if ((item = cJSON_GetObjectItem(payload, "sender_name")) && item->valuestring)
        snprintf(s.sender_name, sizeof(s.sender_name), "%s", item->valuestring);
    else if (caller)
        snprintf(s.sender_name, sizeof(s.sender_name), "%s", caller->full_name);

    if ((item = cJSON_GetObjectItem(payload, "sender_phone")) && item->valuestring)
        snprintf(s.sender_phone, sizeof(s.sender_phone), "%s", item->valuestring);
    else if (caller)
        snprintf(s.sender_phone, sizeof(s.sender_phone), "%s", caller->phone);

    if ((item = cJSON_GetObjectItem(payload, "sender_address")) && item->valuestring)
        snprintf(s.sender_address, sizeof(s.sender_address), "%s", item->valuestring);

    if ((item = cJSON_GetObjectItem(payload, "recipient_name")) && item->valuestring)
        snprintf(s.recipient_name, sizeof(s.recipient_name), "%s", item->valuestring);

    if ((item = cJSON_GetObjectItem(payload, "recipient_phone")) && item->valuestring)
        snprintf(s.recipient_phone, sizeof(s.recipient_phone), "%s", item->valuestring);

    if ((item = cJSON_GetObjectItem(payload, "recipient_address")) && item->valuestring)
        snprintf(s.recipient_address, sizeof(s.recipient_address), "%s", item->valuestring);

    if ((item = cJSON_GetObjectItem(payload, "recipient_pincode")) && item->valuestring)
        snprintf(s.recipient_pincode, sizeof(s.recipient_pincode), "%s", item->valuestring);

    if ((item = cJSON_GetObjectItem(payload, "origin_hub_id")) && item->valuestring)
        snprintf(s.origin_hub_id, sizeof(s.origin_hub_id), "%s", item->valuestring);
    else
        snprintf(s.origin_hub_id, sizeof(s.origin_hub_id), "a0000000-0000-0000-0000-000000000001");

    if ((item = cJSON_GetObjectItem(payload, "destination_hub_id")) && item->valuestring)
        snprintf(s.destination_hub_id, sizeof(s.destination_hub_id), "%s", item->valuestring);
    else
        snprintf(s.destination_hub_id, sizeof(s.destination_hub_id), "a0000000-0000-0000-0000-000000000002");

    if ((item = cJSON_GetObjectItem(payload, "weight_kg")) && cJSON_IsNumber(item))
        s.weight_kg = item->valuedouble;
    else
        s.weight_kg = 1.0;

    if ((item = cJSON_GetObjectItem(payload, "dimensions_cm")) && item->valuestring)
        snprintf(s.dimensions_cm, sizeof(s.dimensions_cm), "%s", item->valuestring);
    else
        snprintf(s.dimensions_cm, sizeof(s.dimensions_cm), "20x15x10");

    double l = 20, w = 15, h = 10;
    sscanf(s.dimensions_cm, "%lfx%lfx%lf", &l, &w, &h);
    s.volumetric_weight_kg = (l * w * h) / 5000.0;
    s.billable_weight_kg = (s.weight_kg > s.volumetric_weight_kg) ? s.weight_kg : s.volumetric_weight_kg;

    if ((item = cJSON_GetObjectItem(payload, "declared_value")) && cJSON_IsNumber(item))
        s.declared_value = item->valuedouble;

    if ((item = cJSON_GetObjectItem(payload, "is_fragile")))
        s.is_fragile = cJSON_IsTrue(item);

    if ((item = cJSON_GetObjectItem(payload, "is_hazardous")))
        s.is_hazardous = cJSON_IsTrue(item);

    s.shipping_cost = 150.0 + (s.billable_weight_kg * 50.0) + (s.is_fragile ? 75.0 : 0.0);
    s.payment_status = PAYMENT_PREPAID;
    s.status = STATUS_ORDER_CREATED;

    int res = db_create_shipment(&s);
    if (res != 0) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Failed to book shipment.");
        return response;
    }

    ShipmentRecord *created = db_find_shipment_by_tracking_id(s.tracking_id);

    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddStringToObject(response, "message", "Shipment booked successfully.");
    cJSON_AddStringToObject(response, "tracking_id", s.tracking_id);
    if (created) {
        cJSON_AddStringToObject(response, "shipment_id", created->id);
        cJSON_AddNumberToObject(response, "shipping_cost", created->shipping_cost);
    }
    return response;
}

cJSON *handle_shipment_list(const UserRecord *caller, const char *status, const char *hub_id) {
    const char *customer_id = NULL;
    const char *agent_id = NULL;

    if (caller) {
        if (caller->role == ROLE_STANDARD_CUSTOMER || caller->role == ROLE_ENTERPRISE_CUSTOMER) {
            customer_id = caller->id;
        } else if (caller->role == ROLE_DELIVERY_AGENT) {
            agent_id = caller->id;
        }
    }

    cJSON *shipments = db_get_shipments_json(status, hub_id, customer_id, agent_id);
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddItemToObject(response, "shipments", shipments);
    return response;
}

cJSON *handle_bulk_booking(const UserRecord *caller, const cJSON *shipments_array) {
    cJSON *response = cJSON_CreateObject();
    if (!cJSON_IsArray(shipments_array)) {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Expected JSON array of shipments.");
        return response;
    }

    int count = cJSON_GetArraySize(shipments_array);
    cJSON *created_ids = cJSON_CreateArray();
    int success_count = 0;

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(shipments_array, i);
        cJSON *res = handle_shipment_create(caller, item);
        cJSON *success_field = cJSON_GetObjectItem(res, "success");
        if (success_field && cJSON_IsTrue(success_field)) {
            cJSON *tid = cJSON_GetObjectItem(res, "tracking_id");
            if (tid) cJSON_AddItemToArray(created_ids, cJSON_CreateString(tid->valuestring));
            success_count++;
        }
        cJSON_Delete(res);
    }

    cJSON_AddBoolToObject(response, "success", true);
    cJSON_AddNumberToObject(response, "total_submitted", count);
    cJSON_AddNumberToObject(response, "success_count", success_count);
    cJSON_AddItemToObject(response, "tracking_ids", created_ids);
    return response;
}
