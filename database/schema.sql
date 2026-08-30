-- CRMGMT v0.1 - Supabase / PostgreSQL 15 Relational Schema
-- Extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

-- 1. Custom Enums
DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'user_role_t') THEN
        CREATE TYPE user_role_t AS ENUM ('super_admin', 'hub_manager', 'delivery_agent', 'enterprise_customer', 'standard_customer');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'shipment_status_t') THEN
        CREATE TYPE shipment_status_t AS ENUM ('ORDER_CREATED', 'PICKED_UP', 'IN_TRANSIT', 'OUT_FOR_DELIVERY', 'DELIVERED', 'FAILED_ATTEMPT', 'RETURNED', 'EXCEPTION');
    END IF;
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'payment_status_t') THEN
        CREATE TYPE payment_status_t AS ENUM ('UNPAID', 'PREPAID', 'COD_PENDING', 'COD_SETTLED', 'REFUNDED');
    END IF;
END $$;

-- 2. Organizations & Hubs
CREATE TABLE IF NOT EXISTS hubs (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    hub_code VARCHAR(16) UNIQUE NOT NULL,
    hub_name VARCHAR(128) NOT NULL,
    latitude DOUBLE PRECISION NOT NULL,
    longitude DOUBLE PRECISION NOT NULL,
    address TEXT NOT NULL,
    capacity INT DEFAULT 10000,
    current_load INT DEFAULT 0,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- 3. Users & Credentials Management
CREATE TABLE IF NOT EXISTS users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    role user_role_t NOT NULL DEFAULT 'standard_customer',
    full_name VARCHAR(128) NOT NULL,
    phone VARCHAR(32) NOT NULL,
    allocated_hub_id UUID REFERENCES hubs(id) ON DELETE SET NULL,
    api_key VARCHAR(64) UNIQUE,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- 4. Shipments Table (Master Logistics Meta)
CREATE TABLE IF NOT EXISTS shipments (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    tracking_id VARCHAR(32) UNIQUE NOT NULL, -- Format: CR-YYYYMMDD-[CHECKSUM]-[RAND] or CR-[HEX_TS]-[CHECKSUM]-[SALT]
    sender_id UUID REFERENCES users(id) ON DELETE SET NULL,
    sender_name VARCHAR(128) NOT NULL,
    sender_phone VARCHAR(32) NOT NULL,
    sender_address TEXT NOT NULL,
    recipient_name VARCHAR(128) NOT NULL,
    recipient_phone VARCHAR(32) NOT NULL,
    recipient_address TEXT NOT NULL,
    recipient_pincode VARCHAR(16) NOT NULL,
    origin_hub_id UUID REFERENCES hubs(id),
    destination_hub_id UUID REFERENCES hubs(id),
    assigned_agent_id UUID REFERENCES users(id),
    current_status shipment_status_t NOT NULL DEFAULT 'ORDER_CREATED',
    weight_kg NUMERIC(8, 2) NOT NULL,
    dimensions_cm VARCHAR(32), -- LxWxH
    volumetric_weight_kg NUMERIC(8, 2) DEFAULT 0.00,
    billable_weight_kg NUMERIC(8, 2) NOT NULL,
    declared_value NUMERIC(12, 2) NOT NULL DEFAULT 0.00,
    shipping_cost NUMERIC(10, 2) NOT NULL,
    payment_status payment_status_t NOT NULL DEFAULT 'PREPAID',
    is_fragile BOOLEAN DEFAULT FALSE,
    is_hazardous BOOLEAN DEFAULT FALSE,
    estimated_delivery TIMESTAMPTZ,
    pod_signature_url TEXT,
    pod_image_url TEXT,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

-- 5. Shipment Checkpoint Event Sourcing (Live Tracking History)
CREATE TABLE IF NOT EXISTS tracking_checkpoints (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    shipment_id UUID NOT NULL REFERENCES shipments(id) ON DELETE CASCADE,
    hub_id UUID REFERENCES hubs(id),
    scanned_by_user_id UUID REFERENCES users(id),
    status shipment_status_t NOT NULL,
    latitude DOUBLE PRECISION,
    longitude DOUBLE PRECISION,
    location_tag VARCHAR(128) NOT NULL,
    remarks TEXT,
    timestamp TIMESTAMPTZ DEFAULT NOW()
);

-- 6. Comprehensive Audit Logs & Security Trails
CREATE TABLE IF NOT EXISTS audit_logs (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    actor_id UUID REFERENCES users(id),
    action_type VARCHAR(64) NOT NULL,
    entity_name VARCHAR(64) NOT NULL,
    entity_id VARCHAR(64) NOT NULL,
    ip_address VARCHAR(45),
    user_agent TEXT,
    payload_snapshot JSONB,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

-- 7. High-Throughput Indexes
CREATE INDEX IF NOT EXISTS idx_shipments_tracking_id ON shipments(tracking_id);
CREATE INDEX IF NOT EXISTS idx_tracking_checkpoints_shipment_id ON tracking_checkpoints(shipment_id);
CREATE INDEX IF NOT EXISTS idx_shipments_status ON shipments(current_status);
CREATE INDEX IF NOT EXISTS idx_shipments_created_at ON shipments(created_at);
CREATE INDEX IF NOT EXISTS idx_hubs_code ON hubs(hub_code);
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);

-- 8. Row Level Security (RLS) Setup
ALTER TABLE hubs ENABLE ROW LEVEL SECURITY;
ALTER TABLE users ENABLE ROW LEVEL SECURITY;
ALTER TABLE shipments ENABLE ROW LEVEL SECURITY;
ALTER TABLE tracking_checkpoints ENABLE ROW LEVEL SECURITY;
ALTER TABLE audit_logs ENABLE ROW LEVEL SECURITY;

-- Public read access to active hubs
DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Public read active hubs') THEN
        CREATE POLICY "Public read active hubs" ON hubs FOR SELECT USING (is_active = TRUE);
    END IF;
END $$;

-- Public tracking checkpoints read
DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Public tracking checkpoints read') THEN
        CREATE POLICY "Public tracking checkpoints read" ON tracking_checkpoints FOR SELECT USING (TRUE);
    END IF;
END $$;

-- Public shipment lookup by tracking_id
DO $$ BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_policies WHERE policyname = 'Public shipment lookup by tracking_id') THEN
        CREATE POLICY "Public shipment lookup by tracking_id" ON shipments FOR SELECT USING (TRUE);
    END IF;
END $$;
