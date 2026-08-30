-- CRMGMT v0.1 Seed Data
-- 1. Insert Logistic Hubs
INSERT INTO hubs (id, hub_code, hub_name, latitude, longitude, address, capacity, current_load, is_active) VALUES
('a0000000-0000-0000-0000-000000000001', 'HUB-CHE-01', 'Saveetha Chennai Central Gateway', 13.0827, 80.2707, 'Saveetha Nagar, Poonamallee High Rd, Chennai, TN 602105', 25000, 3420, TRUE),
('a0000000-0000-0000-0000-000000000002', 'HUB-BLR-02', 'Bengaluru Electronic City Hub', 12.9716, 77.5946, 'Phase 1, Hosur Road, Bengaluru, KA 560100', 20000, 2810, TRUE),
('a0000000-0000-0000-0000-000000000003', 'HUB-MUM-03', 'Mumbai Western Freight Terminal', 19.0760, 72.8777, 'Bandra Kurla Complex, Cargo Wing, Mumbai, MH 400051', 30000, 4150, TRUE),
('a0000000-0000-0000-0000-000000000004', 'HUB-DEL-04', 'Delhi NCR Logistics Center', 28.6139, 77.2090, 'IGI Cargo Terminal Road, New Delhi, DL 110037', 35000, 5290, TRUE),
('a0000000-0000-0000-0000-000000000005', 'HUB-HYD-05', 'Hyderabad Express Distribution', 17.3850, 78.4867, 'Shamshabad Aero Logistics Park, Hyderabad, TS 500409', 18000, 1940, TRUE),
('a0000000-0000-0000-0000-000000000006', 'HUB-CCU-06', 'Kolkata Eastern Logistics Yard', 22.5726, 88.3639, 'Strand Road, Port Area, Kolkata, WB 700001', 15000, 1420, TRUE)
ON CONFLICT (id) DO NOTHING;

-- 2. Insert Users (Admin, Managers, Delivery Agents, Enterprise & Standard Customers)
-- Note: password_hash uses PBKDF2/SHA256 representation for password "Admin@123" and "User@123"
INSERT INTO users (id, email, password_hash, role, full_name, phone, allocated_hub_id, api_key, is_active) VALUES
('u0000000-0000-0000-0000-000000000001', 'admin@crmgmt.io', 'pbkdf2_sha256$260000$crmgmt_salt$8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'super_admin', 'SIMATS Chief Systems Administrator', '+91 98400 11223', 'a0000000-0000-0000-0000-000000000001', 'crm_master_key_8f3a9e22', TRUE),
('u0000000-0000-0000-0000-000000000002', 'hub.chennai@crmgmt.io', 'pbkdf2_sha256$260000$crmgmt_salt$8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'hub_manager', 'Saveetha Central Hub Operations Manager', '+91 98401 22334', 'a0000000-0000-0000-0000-000000000001', 'crm_hub_che_key_44b1c', TRUE),
('u0000000-0000-0000-0000-000000000003', 'agent.che01@crmgmt.io', 'pbkdf2_sha256$260000$crmgmt_salt$8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'delivery_agent', 'SIMATS Dispatch & Fleet Unit 01', '+91 98402 33445', 'a0000000-0000-0000-0000-000000000001', 'crm_agent_che_12345', TRUE),
('u0000000-0000-0000-0000-000000000004', 'enterprise@saveetha.com', 'pbkdf2_sha256$260000$crmgmt_salt$8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'enterprise_customer', 'Saveetha Biomedical & Healthcare Procurement', '+91 98403 44556', 'a0000000-0000-0000-0000-000000000001', 'crm_ent_sav_99a8b7', TRUE),
('u0000000-0000-0000-0000-000000000005', 'customer@gmail.com', 'pbkdf2_sha256$260000$crmgmt_salt$8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'standard_customer', 'Verified Retail Consignee', '+91 98404 55667', NULL, NULL, TRUE)
ON CONFLICT (id) DO NOTHING;

-- 3. Insert Initial Shipments with Checksum Tracking Numbers
INSERT INTO shipments (
    id, tracking_id, sender_id, sender_name, sender_phone, sender_address,
    recipient_name, recipient_phone, recipient_address, recipient_pincode,
    origin_hub_id, destination_hub_id, assigned_agent_id,
    current_status, weight_kg, dimensions_cm, volumetric_weight_kg, billable_weight_kg,
    declared_value, shipping_cost, payment_status, is_fragile, is_hazardous,
    estimated_delivery, created_at, updated_at
) VALUES
(
    's0000000-0000-0000-0000-000000000001', 'CR-68D3F12A-B4-9F81', 'u0000000-0000-0000-0000-000000000004',
    'Saveetha Institute Biomed Lab', '+91 98403 44556', 'SIMATS Campus, 162 Poonamallee High Rd, Chennai TN',
    'Apollo Super Specialty Hospital', '+91 98840 99887', 'Greams Lane, Thousand Lights, Chennai TN', '600006',
    'a0000000-0000-0000-0000-000000000001', 'a0000000-0000-0000-0000-000000000001', 'u0000000-0000-0000-0000-000000000003',
    'OUT_FOR_DELIVERY', 2.50, '30x20x15', 1.80, 2.50,
    15000.00, 450.00, 'PREPAID', TRUE, FALSE,
    NOW() + INTERVAL '4 hours', NOW() - INTERVAL '1 day', NOW()
),
(
    's0000000-0000-0000-0000-000000000002', 'CR-68D40E1B-C2-4E29', 'u0000000-0000-0000-0000-000000000005',
    'Ananya Sharma', '+91 98404 55667', 'Flat 402, Green Meadows, Thiruvanmiyur, Chennai TN',
    'Rajesh Verma', '+91 98110 33441', 'Tower 3, Cyber Gateway, Hitec City, Hyderabad TS', '500081',
    'a0000000-0000-0000-0000-000000000001', 'a0000000-0000-0000-0000-000000000005', NULL,
    'IN_TRANSIT', 5.00, '40x35x25', 7.00, 7.00,
    8500.00, 1250.00, 'PREPAID', FALSE, FALSE,
    NOW() + INTERVAL '1 day', NOW() - INTERVAL '12 hours', NOW()
),
(
    's0000000-0000-0000-0000-000000000003', 'CR-68D41A9C-7F-33A1', 'u0000000-0000-0000-0000-000000000004',
    'Saveetha Tech R&D Lab', '+91 98403 44556', 'Saveetha Nagar, Chennai TN',
    'Infosys STPI Development Hub', '+91 98450 77112', 'Electronics City Phase 1, Bangalore KA', '560100',
    'a0000000-0000-0000-0000-000000000001', 'a0000000-0000-0000-0000-000000000002', NULL,
    'PICKED_UP', 1.20, '20x15x10', 0.60, 1.20,
    3500.00, 320.00, 'PREPAID', TRUE, FALSE,
    NOW() + INTERVAL '2 days', NOW() - INTERVAL '3 hours', NOW()
),
(
    's0000000-0000-0000-0000-000000000004', 'CR-68D4255E-A1-19B4', 'u0000000-0000-0000-0000-000000000005',
    'TechNova Electronics', '+91 98200 44551', 'SEEPZ Industrial Area, Andheri East, Mumbai MH',
    'Meera Krishnan', '+91 98408 12345', 'Boat Club Road, R.A. Puram, Chennai TN', '600028',
    'a0000000-0000-0000-0000-000000000003', 'a0000000-0000-0000-0000-000000000001', 'u0000000-0000-0000-0000-000000000003',
    'DELIVERED', 0.80, '15x10x5', 0.15, 0.80,
    4200.00, 280.00, 'PREPAID', FALSE, FALSE,
    NOW() - INTERVAL '2 hours', NOW() - INTERVAL '2 days', NOW() - INTERVAL '2 hours'
)
ON CONFLICT (id) DO NOTHING;

-- 4. Insert Checkpoints
INSERT INTO tracking_checkpoints (id, shipment_id, hub_id, scanned_by_user_id, status, latitude, longitude, location_tag, remarks, timestamp) VALUES
('c0000000-0000-0000-0000-000000000001', 's0000000-0000-0000-0000-000000000001', 'a0000000-0000-0000-0000-000000000001', 'u0000000-0000-0000-0000-000000000002', 'ORDER_CREATED', 13.0827, 80.2707, 'Saveetha Chennai Hub', 'Parcel booking registered electronically', NOW() - INTERVAL '24 hours'),
('c0000000-0000-0000-0000-000000000002', 's0000000-0000-0000-0000-000000000001', 'a0000000-0000-0000-0000-000000000001', 'u0000000-0000-0000-0000-000000000002', 'PICKED_UP', 13.0827, 80.2707, 'Saveetha Chennai Hub', 'Package received at sorting dock', NOW() - INTERVAL '18 hours'),
('c0000000-0000-0000-0000-000000000003', 's0000000-0000-0000-0000-000000000001', 'a0000000-0000-0000-0000-000000000001', 'u0000000-0000-0000-0000-000000000003', 'OUT_FOR_DELIVERY', 13.0600, 80.2500, 'Chennai Urban Route 4B', 'Assigned to SIMATS Dispatch & Fleet Unit 01 for final delivery', NOW() - INTERVAL '2 hours'),
('c0000000-0000-0000-0000-000000000004', 's0000000-0000-0000-0000-000000000002', 'a0000000-0000-0000-0000-000000000001', 'u0000000-0000-0000-0000-000000000002', 'ORDER_CREATED', 13.0827, 80.2707, 'Saveetha Chennai Hub', 'Consignment booked online', NOW() - INTERVAL '12 hours'),
('c0000000-0000-0000-0000-000000000005', 's0000000-0000-0000-0000-000000000002', 'a0000000-0000-0000-0000-000000000001', 'u0000000-0000-0000-0000-000000000002', 'IN_TRANSIT', 15.2340, 79.3780, 'NH44 Interstate Transit corridor', 'Dispatched on Express Truck CH-HYD-882', NOW() - INTERVAL '4 hours')
ON CONFLICT (id) DO NOTHING;

-- 5. Audit Log Seed
INSERT INTO audit_logs (id, actor_id, action_type, entity_name, entity_id, ip_address, payload_snapshot, created_at) VALUES
('d0000000-0000-0000-0000-000000000001', 'u0000000-0000-0000-0000-000000000001', 'SYSTEM_INIT', 'SYSTEM', 'CRMGMT-v0.1', '127.0.0.1', '{"version": "0.1.0", "status": "ONLINE"}'::jsonb, NOW() - INTERVAL '7 days'),
('d0000000-0000-0000-0000-000000000002', 'u0000000-0000-0000-0000-000000000001', 'HUB_DEPLOY', 'HUB', 'HUB-CHE-01', '127.0.0.1', '{"hub": "Saveetha Chennai Central Gateway", "capacity": 25000}'::jsonb, NOW() - INTERVAL '6 days')
ON CONFLICT (id) DO NOTHING;
