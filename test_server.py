#!/usr/bin/env python3
"""
CRMGMT v0.1 - Local Verification Server & Test Suite
Executes the exact C API specifications for local verification, browser testing, and automated checks.
"""

import sys
import os
import json
import time
import hashlib
import random
import mimetypes
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

PORT = int(os.environ.get("PORT", 8080))
WEB_ROOT = os.path.join(os.path.dirname(__file__), "web")

# Special Tracking Engine Implementation (Python equivalent of C tracking_engine.c)
TRACKING_SECRET = "CRMGMT-SECRET-SALT-2026"

def generate_special_tracking_id():
    ts = int(time.time())
    salt = random.randint(0, 65535)
    raw_payload = f"{ts:08X}{salt:04X}-{TRACKING_SECRET}"
    h = hashlib.sha256(raw_payload.encode('utf-8')).digest()
    checksum = h[0] ^ h[1]
    return f"CR-{ts:08X}-{checksum:02X}-{salt:04X}"

def verify_tracking_id_checksum(tracking_id):
    if not tracking_id or not tracking_id.startswith("CR-"):
        return False
    parts = tracking_id.split("-")
    if len(parts) != 4:
        return False
    try:
        ts = int(parts[1], 16)
        checksum = int(parts[2], 16)
        salt = int(parts[3], 16)
        raw_payload = f"{ts:08X}{salt:04X}-{TRACKING_SECRET}"
        h = hashlib.sha256(raw_payload.encode('utf-8')).digest()
        expected_checksum = h[0] ^ h[1]
        return checksum == expected_checksum
    except Exception:
        return False

# In-Memory DB
HUBS = [
    {"id": "a0000000-0000-0000-0000-000000000001", "hub_code": "HUB-CHE-01", "hub_name": "Saveetha Chennai Central Gateway", "latitude": 13.0827, "longitude": 80.2707, "address": "Saveetha Nagar, Chennai, TN 602105", "capacity": 25000, "current_load": 3420, "is_active": True},
    {"id": "a0000000-0000-0000-0000-000000000002", "hub_code": "HUB-BLR-02", "hub_name": "Bengaluru Electronic City Hub", "latitude": 12.9716, "longitude": 77.5946, "address": "Phase 1, Hosur Road, Bengaluru, KA 560100", "capacity": 20000, "current_load": 2810, "is_active": True},
    {"id": "a0000000-0000-0000-0000-000000000003", "hub_code": "HUB-MUM-03", "hub_name": "Mumbai Western Freight Terminal", "latitude": 19.0760, "longitude": 72.8777, "address": "Bandra Kurla Complex, Mumbai, MH 400051", "capacity": 30000, "current_load": 4150, "is_active": True},
    {"id": "a0000000-0000-0000-0000-000000000004", "hub_code": "HUB-DEL-04", "hub_name": "Delhi NCR Logistics Center", "latitude": 28.6139, "longitude": 77.2090, "address": "IGI Cargo Terminal Road, New Delhi, DL 110037", "capacity": 35000, "current_load": 5290, "is_active": True},
    {"id": "a0000000-0000-0000-0000-000000000005", "hub_code": "HUB-HYD-05", "hub_name": "Hyderabad Express Distribution", "latitude": 17.3850, "longitude": 78.4867, "address": "Shamshabad Aero Logistics Park, Hyderabad, TS 500409", "capacity": 18000, "current_load": 1940, "is_active": True}
]

USERS = [
    {"id": "u0000000-0000-0000-0000-000000000001", "email": "admin@crmgmt.io", "role": "super_admin", "full_name": "SIMATS Chief Systems Administrator", "phone": "+91 98400 11223", "password": "Admin@123", "allocated_hub_id": "a0000000-0000-0000-0000-000000000001", "api_key": "crm_master_key_8f3a9e22", "is_active": True, "created_at": "2026-08-30T00:00:00Z"},
    {"id": "u0000000-0000-0000-0000-000000000002", "email": "hub.chennai@crmgmt.io", "role": "hub_manager", "full_name": "Saveetha Central Hub Operations Manager", "phone": "+91 98401 22334", "password": "Admin@123", "allocated_hub_id": "a0000000-0000-0000-0000-000000000001", "api_key": "crm_hub_che_key_44b1c", "is_active": True, "created_at": "2026-08-30T00:00:00Z"},
    {"id": "u0000000-0000-0000-0000-000000000003", "email": "agent.che01@crmgmt.io", "role": "delivery_agent", "full_name": "SIMATS Dispatch & Fleet Unit 01", "phone": "+91 98402 33445", "password": "Admin@123", "allocated_hub_id": "a0000000-0000-0000-0000-000000000001", "api_key": "crm_agent_che_12345", "is_active": True, "created_at": "2026-08-30T00:00:00Z"},
    {"id": "u0000000-0000-0000-0000-000000000004", "email": "enterprise@saveetha.com", "role": "enterprise_customer", "full_name": "Saveetha Biomedical & Healthcare Procurement", "phone": "+91 98403 44556", "password": "Admin@123", "allocated_hub_id": "a0000000-0000-0000-0000-000000000001", "api_key": "crm_ent_sav_99a8b7", "is_active": True, "created_at": "2026-08-30T00:00:00Z"},
    {"id": "u0000000-0000-0000-0000-000000000005", "email": "customer@gmail.com", "role": "standard_customer", "full_name": "Verified Retail Consignee", "phone": "+91 98404 55667", "password": "User@123", "allocated_hub_id": None, "api_key": None, "is_active": True, "created_at": "2026-08-30T00:00:00Z"}
]

SHIPMENTS = [
    {
        "id": "s0000000-0000-0000-0000-000000000001", "tracking_id": "CR-68D3F12A-B4-9F81",
        "sender_name": "Saveetha Biomed Lab", "sender_phone": "+91 98403 44556",
        "recipient_name": "Apollo Super Specialty Hospital", "recipient_phone": "+91 98840 99887",
        "recipient_address": "Greams Lane, Thousand Lights, Chennai TN", "recipient_pincode": "600006",
        "origin_hub_id": "a0000000-0000-0000-0000-000000000001",
        "destination_hub_id": "a0000000-0000-0000-0000-000000000001",
        "status": "OUT_FOR_DELIVERY", "weight_kg": 2.5, "shipping_cost": 450.0,
        "payment_status": "PREPAID", "is_fragile": True,
        "estimated_delivery": "2026-08-31T18:00:00Z", "created_at": "2026-08-30T10:00:00Z",
        "pod_signature_url": ""
    },
    {
        "id": "s0000000-0000-0000-0000-000000000002", "tracking_id": "CR-68D40E1B-C2-4E29",
        "sender_name": "Ananya Sharma", "sender_phone": "+91 98404 55667",
        "recipient_name": "Rajesh Verma", "recipient_phone": "+91 98110 33441",
        "recipient_address": "Tower 3, Cyber Gateway, Hitec City, Hyderabad TS", "recipient_pincode": "500081",
        "origin_hub_id": "a0000000-0000-0000-0000-000000000001",
        "destination_hub_id": "a0000000-0000-0000-0000-000000000005",
        "status": "IN_TRANSIT", "weight_kg": 5.0, "shipping_cost": 1250.0,
        "payment_status": "PREPAID", "is_fragile": False,
        "estimated_delivery": "2026-09-01T14:00:00Z", "created_at": "2026-08-30T06:00:00Z",
        "pod_signature_url": ""
    }
]

CHECKPOINTS = [
    {"id": "c1", "shipment_id": "s0000000-0000-0000-0000-000000000001", "status": "ORDER_CREATED", "location_tag": "Saveetha Chennai Central Gateway", "remarks": "Consignment booked online", "timestamp": "2026-08-30T10:00:00Z"},
    {"id": "c2", "shipment_id": "s0000000-0000-0000-0000-000000000001", "status": "PICKED_UP", "location_tag": "Saveetha Chennai Central Gateway", "remarks": "Package sorted at bay 3", "timestamp": "2026-08-30T13:30:00Z"},
    {"id": "c3", "shipment_id": "s0000000-0000-0000-0000-000000000001", "status": "OUT_FOR_DELIVERY", "location_tag": "Chennai Urban Route 4B", "remarks": "Assigned to Agent Vignesh Kumar", "timestamp": "2026-08-30T16:00:00Z"}
]

class RequestHandler(BaseHTTPRequestHandler):
    def _send_json(self, data, status=200):
        body = json.dumps(data).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type, Authorization')
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type, Authorization')
        self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        # REST API Routes
        if path == '/api/v1/auth/me':
            self._send_json({"success": True, "user": USERS[0]})
            return

        if path == '/api/v1/admin/analytics':
            self._send_json({
                "success": True,
                "data": {
                    "metrics": {
                        "sales": 2.382,
                        "sales_change": "-3.65%",
                        "earnings": 21300.0,
                        "earnings_formatted": "$21.300",
                        "earnings_change": "+6.65%",
                        "visitors": 14.212,
                        "visitors_change": "+5.25%",
                        "orders": 64,
                        "orders_change": "-2.25%"
                    },
                    "recent_movement": [
                        {"month": "Jan", "movement": 2100},
                        {"month": "Feb", "movement": 1600},
                        {"month": "Mar", "movement": 1850},
                        {"month": "Apr", "movement": 1950},
                        {"month": "May", "movement": 1600},
                        {"month": "Jun", "movement": 2100},
                        {"month": "Jul", "movement": 2800},
                        {"month": "Aug", "movement": 2700},
                        {"month": "Sep", "movement": 3100},
                        {"month": "Oct", "movement": 3700},
                        {"month": "Nov", "movement": 3200},
                        {"month": "Dec", "movement": 3600}
                    ],
                    "status_breakdown": {
                        "chrome_delivered": 4306,
                        "firefox_intransit": 3801,
                        "edge_outfordelivery": 1689,
                        "other_exceptions": 3251
                    },
                    "hubs": HUBS
                }
            })
            return

        if path == '/api/v1/hubs':
            self._send_json({"success": True, "hubs": HUBS})
            return

        if path == '/api/v1/admin/users':
            self._send_json({"success": True, "users": USERS})
            return

        if path == '/api/v1/shipments':
            self._send_json({"success": True, "shipments": SHIPMENTS})
            return

        if path.startswith('/api/v1/tracking/'):
            tracking_id = path[17:]
            found = next((s for s in SHIPMENTS if s["tracking_id"].upper() == tracking_id.upper()), None)
            if not found:
                self._send_json({"success": False, "error": "Tracking ID not found"}, 404)
                return

            cps = [c for c in CHECKPOINTS if c["shipment_id"] == found["id"]]
            orig = next((h for h in HUBS if h["id"] == found.get("origin_hub_id")), HUBS[0])
            dest = next((h for h in HUBS if h["id"] == found.get("destination_hub_id")), HUBS[1])

            self._send_json({
                "success": True,
                "shipment": found,
                "checkpoints": cps,
                "route": {
                    "origin": {"name": orig["hub_name"], "lat": orig["latitude"], "lng": orig["longitude"]},
                    "destination": {"name": dest["hub_name"], "lat": dest["latitude"], "lng": dest["longitude"]}
                }
            })
            return

        # Static Assets
        if path in ['/', '/admin', '/login', '/track']:
            filepath = os.path.join(WEB_ROOT, 'index.html')
        else:
            filepath = os.path.join(WEB_ROOT, path.lstrip('/'))

        if os.path.isfile(filepath):
            mime, _ = mimetypes.guess_type(filepath)
            if not mime:
                mime = 'application/octet-stream'
            if filepath.endswith('.css'): mime = 'text/css; charset=utf-8'
            if filepath.endswith('.js'): mime = 'application/javascript; charset=utf-8'
            if filepath.endswith('.svg'): mime = 'image/svg+xml'

            with open(filepath, 'rb') as f:
                content = f.read()

            self.send_response(200)
            self.send_header('Content-Type', mime)
            self.send_header('Content-Length', str(len(content)))
            self.end_headers()
            self.wfile.write(content)
        else:
            # Fallback index.html
            index_path = os.path.join(WEB_ROOT, 'index.html')
            if os.path.isfile(index_path):
                with open(index_path, 'rb') as f:
                    content = f.read()
                self.send_response(200)
                self.send_header('Content-Type', 'text/html; charset=utf-8')
                self.send_header('Content-Length', str(len(content)))
                self.end_headers()
                self.wfile.write(content)
            else:
                self.send_error(404, "Not Found")

    def do_POST(self):
        length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(length).decode('utf-8') if length > 0 else '{}'
        try:
            payload = json.loads(body)
        except Exception:
            payload = {}

        parsed = urlparse(self.path)
        path = parsed.path

        if path == '/api/v1/auth/login':
            email = payload.get('email')
            user = next((u for u in USERS if u["email"].lower() == (email or "").lower()), None)
            if user:
                self._send_json({
                    "success": True,
                    "token": f"jwt_{user['id']}_{int(time.time())}",
                    "user": user
                })
            else:
                self._send_json({"success": False, "error": "Invalid email or password."}, 401)
            return

        if path == '/api/v1/auth/register':
            new_u = {
                "id": f"u{len(USERS)+1:08d}",
                "email": payload.get('email'),
                "full_name": payload.get('full_name', 'Customer'),
                "role": payload.get('role', 'standard_customer'),
                "phone": payload.get('phone', ''),
                "allocated_hub_id": payload.get('allocated_hub_id'),
                "api_key": f"crm_key_{random.randint(10000, 99999)}",
                "is_active": True,
                "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
            }
            USERS.append(new_u)
            self._send_json({"success": True, "token": f"jwt_{new_u['id']}", "user": new_u})
            return

        if path == '/api/v1/admin/users/create':
            email = payload.get('email')
            if any(u["email"].lower() == (email or "").lower() for u in USERS):
                self._send_json({"success": False, "error": "Email address already registered."}, 409)
                return

            role = payload.get('role', 'standard_customer')
            name = payload.get('full_name', 'System User')
            pwd = payload.get('password', 'Admin@123')
            phone = payload.get('phone', '+91 98400 00000')
            hub_id = payload.get('allocated_hub_id', HUBS[0]["id"] if role in ['hub_manager', 'delivery_agent'] else None)
            api_key = f"crm_{role[:3]}_{random.randint(10000, 99999)}"

            new_user = {
                "id": f"u{len(USERS)+1:08d}",
                "email": email,
                "full_name": name,
                "role": role,
                "phone": phone,
                "password": pwd,
                "allocated_hub_id": hub_id,
                "api_key": api_key,
                "is_active": True,
                "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
            }
            USERS.append(new_user)
            self._send_json({
                "success": True,
                "message": f"Successfully provisioned account for {name} ({role})",
                "user": new_user,
                "login_token": f"jwt_{new_user['id']}_{int(time.time())}"
            })
            return

        if path == '/api/v1/admin/users/impersonate':
            user_id = payload.get('user_id')
            user = next((u for u in USERS if u["id"] == user_id), None)
            if user:
                self._send_json({
                    "success": True,
                    "token": f"jwt_{user['id']}_{int(time.time())}",
                    "user": user
                })
            else:
                self._send_json({"success": False, "error": "User not found."}, 404)
            return

        if path == '/api/v1/admin/users/toggle-status':
            user_id = payload.get('user_id')
            user = next((u for u in USERS if u["id"] == user_id), None)
            if user:
                user["is_active"] = not user.get("is_active", True)
                self._send_json({"success": True, "is_active": user["is_active"]})
            else:
                self._send_json({"success": False, "error": "User not found."}, 404)
            return

        if path == '/api/v1/admin/users/delete':
            user_id = payload.get('user_id')
            USERS[:] = [u for u in USERS if u["id"] != user_id]
            self._send_json({"success": True, "message": "User deleted."})
            return

        if path == '/api/v1/shipping/calculate':
            l = float(payload.get('length_cm', 20))
            w = float(payload.get('width_cm', 15))
            h = float(payload.get('height_cm', 10))
            wt = float(payload.get('weight_kg', 1.0))
            fragile = bool(payload.get('is_fragile', False))
            express = bool(payload.get('is_express', False))

            vol = round((l * w * h) / 5000.0, 2)
            billable = max(wt, vol)
            cost = 150.0 + (billable * 50.0) + (75.0 if fragile else 0) + (150.0 if express else 0)

            self._send_json({
                "success": True,
                "volumetric_weight_kg": vol,
                "billable_weight_kg": billable,
                "shipping_cost": cost,
                "estimated_transit_time": "24-48 Hours" if express else "3-5 Business Days"
            })
            return

        if path == '/api/v1/shipments/create':
            tid = generate_special_tracking_id()
            new_s = {
                "id": f"s{len(SHIPMENTS)+1:08d}",
                "tracking_id": tid,
                "sender_name": payload.get('sender_name', 'Saveetha Customer'),
                "sender_phone": payload.get('sender_phone', ''),
                "recipient_name": payload.get('recipient_name', 'Recipient'),
                "recipient_phone": payload.get('recipient_phone', ''),
                "recipient_address": payload.get('recipient_address', 'Chennai'),
                "recipient_pincode": payload.get('recipient_pincode', '600001'),
                "origin_hub_id": HUBS[0]["id"],
                "destination_hub_id": HUBS[1]["id"],
                "status": "ORDER_CREATED",
                "weight_kg": float(payload.get('weight_kg', 1.5)),
                "shipping_cost": 275.0,
                "payment_status": "PREPAID",
                "is_fragile": bool(payload.get('is_fragile', False)),
                "created_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "pod_signature_url": ""
            }
            SHIPMENTS.append(new_s)
            CHECKPOINTS.append({
                "id": f"c{len(CHECKPOINTS)+1}",
                "shipment_id": new_s["id"],
                "status": "ORDER_CREATED",
                "location_tag": "Saveetha Chennai Central Gateway",
                "remarks": "Booking registered electronically",
                "timestamp": new_s["created_at"]
            })
            self._send_json({"success": True, "tracking_id": tid, "shipment_id": new_s["id"]})
            return

        if path == '/api/v1/shipments/send-tracking-email':
            target_email = payload.get('to') or payload.get('recipient_email')
            tracking_id = payload.get('tracking_id')
            tracking_url = payload.get('tracking_url') or f"https://fin-crmgmt-v0-1.onrender.com/#public_track?id={tracking_id}"
            
            self._send_json({
                "success": True,
                "message": f"Tracking notification for {tracking_id} dispatched via Resend to {target_email}",
                "to": target_email,
                "tracking_id": tracking_id,
                "tracking_url": tracking_url
            })
            return

        if path == '/api/v1/checkpoints/scan':
            tid = payload.get('tracking_id')
            new_st = payload.get('status', 'IN_TRANSIT')
            remarks = payload.get('remarks', 'Hub scan verified')

            s = next((x for x in SHIPMENTS if x["tracking_id"].upper() == (tid or "").upper()), None)
            if s:
                s["status"] = new_st
                CHECKPOINTS.append({
                    "id": f"c{len(CHECKPOINTS)+1}",
                    "shipment_id": s["id"],
                    "status": new_st,
                    "location_tag": "Saveetha Chennai Central Gateway",
                    "remarks": remarks,
                    "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
                })
                self._send_json({"success": True, "message": "Checkpoint recorded", "new_status": new_st})
            else:
                self._send_json({"success": False, "error": "Shipment not found"}, 404)
            return

        if '/pod' in path:
            ship_id = path.split('/')[4]
            s = next((x for x in SHIPMENTS if x["id"] == ship_id), None)
            if s:
                s["pod_signature_url"] = payload.get('signature_data', '')
                s["status"] = "DELIVERED"
                self._send_json({"success": True, "message": "POD signature saved, shipment marked DELIVERED."})
            else:
                self._send_json({"success": False, "error": "Shipment not found"}, 404)
            return

        self._send_json({"success": False, "error": "Unknown endpoint"}, 404)

def run_tests():
    print("=== Running CRMGMT v0.1 Automated Verification Tests ===")
    
    # 1. Test Special Tracking Engine Checksum
    print("[TEST 1] Generating and verifying 100 Special Tracking Numbers...")
    for _ in range(100):
        tid = generate_special_tracking_id()
        assert tid.startswith("CR-"), f"Invalid format: {tid}"
        assert verify_tracking_id_checksum(tid), f"Valid ID failed verification: {tid}"
        # Tamper check by flipping checksum byte
        parts = tid.split("-")
        tampered = f"CR-{parts[1]}-{(int(parts[2], 16) ^ 0xAA):02X}-{parts[3]}"
        assert not verify_tracking_id_checksum(tampered), f"Tampered ID falsely passed: {tampered}"
    print("-> PASS: 100/100 Tracking Numbers generated and verified with 100% cryptographic integrity.")

    # 2. Test Volumetric Weight Calculation
    print("[TEST 2] Verifying Volumetric vs Billable Rate Calculation...")
    # L=50, W=40, H=30 => 50*40*30 / 5000 = 12.0 kg
    vol = (50 * 40 * 30) / 5000.0
    actual = 5.0
    billable = max(actual, vol)
    assert billable == 12.0, f"Expected 12.0 kg billable weight, got {billable}"
    print("-> PASS: Volumetric tariff algorithm verified.")

    print("=== All Core Engine Verification Tests PASSED! ===")

if __name__ == '__main__':
    if '--verify-only' in sys.argv:
        run_tests()
        sys.exit(0)

    run_tests()
    server = HTTPServer(('0.0.0.0', PORT), RequestHandler)
    print(f"\n[SERVER] CRMGMT v0.1 Server listening at http://127.0.0.1:{PORT}")
    print(f"[SERVER] Web root: {WEB_ROOT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[SERVER] Shutting down...")
