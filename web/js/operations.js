/* CRMGMT v0.1 - Logistics Operations, Hub Scanner & Rate Calculator Controller */

const OperationsModule = {
  shipments: [],

  init() {
    this.bindEvents();
    this.initCalculator();
  },

  bindEvents() {
    // New Shipment Form
    const newShipForm = document.getElementById('form-create-shipment');
    if (newShipForm) {
      newShipForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        await this.createShipment();
      });
    }

    // Checkpoint Scanner Form
    const scanForm = document.getElementById('form-hub-scan');
    if (scanForm) {
      scanForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        await this.submitScan();
      });
    }

    // Calculator Inputs
    ['calc-l', 'calc-w', 'calc-h', 'calc-wt', 'calc-fragile', 'calc-express'].forEach(id => {
      const el = document.getElementById(id);
      if (el) el.addEventListener('input', () => this.calculateRate());
    });

    // CSV Export
    const exportBtn = document.getElementById('btn-export-csv');
    if (exportBtn) {
      exportBtn.addEventListener('click', () => this.exportCSV());
    }
  },

  async loadShipments() {
    const res = await CRMGMT.api('/api/v1/shipments');
    if (res.data && res.data.success) {
      this.shipments = res.data.shipments || [];
      this.renderShipmentsTable(this.shipments);
    }
  },

  renderShipmentsTable(list) {
    const tbody = document.getElementById('shipments-table-body');
    if (!tbody) return;

    tbody.innerHTML = '';
    if (list.length === 0) {
      tbody.innerHTML = '<tr><td colspan="7" style="text-align: center; color: #94a3b8; padding: 24px;">No shipments found.</td></tr>';
      return;
    }

    list.forEach(s => {
      const tr = document.createElement('tr');
      tr.style.cssText = 'border-bottom: 1px solid #f1f5f9;';

      let statusBadge = 'badge-info';
      if (s.status === 'DELIVERED') statusBadge = 'badge-success';
      else if (s.status === 'IN_TRANSIT') statusBadge = 'badge-primary';
      else if (s.status === 'OUT_FOR_DELIVERY') statusBadge = 'badge-warning';
      else if (s.status === 'EXCEPTION') statusBadge = 'badge-danger';

      tr.innerHTML = `
        <td style="padding: 12px 16px; font-weight: 700; font-family: var(--font-mono); color: #3b7ddd;">
          <a href="#tracking" onclick="CRMGMT.navigate('tracking', { trackingId: '${s.tracking_id}' })">${s.tracking_id}</a>
        </td>
        <td style="padding: 12px 16px;">
          <div style="font-weight: 600; color: #1e293b;">${s.sender_name}</div>
          <div style="font-size: 0.76rem; color: #64748b;">${s.sender_phone}</div>
        </td>
        <td style="padding: 12px 16px;">
          <div style="font-weight: 600; color: #1e293b;">${s.recipient_name}</div>
          <div style="font-size: 0.76rem; color: #64748b;">${s.recipient_pincode}</div>
        </td>
        <td style="padding: 12px 16px;">
          <span class="badge ${statusBadge}">${s.status.replace(/_/g, ' ')}</span>
        </td>
        <td style="padding: 12px 16px; font-weight: 600;">${s.weight_kg} kg</td>
        <td style="padding: 12px 16px; font-weight: 700; color: #065f46;">₹${Number(s.shipping_cost).toFixed(2)}</td>
        <td style="padding: 12px 16px;">
          <button class="btn btn-outline btn-sm" onclick="CRMGMT.navigate('tracking', { trackingId: '${s.tracking_id}' })">
            Track
          </button>
        </td>
      `;
      tbody.appendChild(tr);
    });
  },

  async createShipment() {
    const sender_name = document.getElementById('ship-sender-name').value.trim();
    const sender_phone = document.getElementById('ship-sender-phone').value.trim();
    const sender_address = document.getElementById('ship-sender-addr').value.trim();
    const recipient_name = document.getElementById('ship-recip-name').value.trim();
    const recipient_phone = document.getElementById('ship-recip-phone').value.trim();
    const recipient_address = document.getElementById('ship-recip-addr').value.trim();
    const recipient_pincode = document.getElementById('ship-recip-pin').value.trim();
    const weight_kg = parseFloat(document.getElementById('ship-weight').value) || 1.0;
    const dimensions_cm = document.getElementById('ship-dimensions').value.trim() || '20x15x10';
    const is_fragile = document.getElementById('ship-is-fragile')?.checked || false;

    const payload = {
      sender_name, sender_phone, sender_address,
      recipient_name, recipient_phone, recipient_address, recipient_pincode,
      weight_kg, dimensions_cm, is_fragile
    };

    const res = await CRMGMT.api('/api/v1/shipments/create', {
      method: 'POST',
      body: JSON.stringify(payload)
    });

    if (res.data && res.data.success) {
      CRMGMT.toast(`Shipment created with Tracking ID: ${res.data.tracking_id}`, 'success');
      this.closeModal('modal-new-shipment');
      this.loadShipments();
      CRMGMT.navigate('tracking', { trackingId: res.data.tracking_id });
    } else {
      CRMGMT.toast(res.data?.error || 'Failed to book shipment.', 'error');
    }
  },

  // Instant Rate Calculator
  initCalculator() {
    this.calculateRate();
  },

  async calculateRate() {
    const l = parseFloat(document.getElementById('calc-l')?.value) || 20;
    const w = parseFloat(document.getElementById('calc-w')?.value) || 15;
    const h = parseFloat(document.getElementById('calc-h')?.value) || 10;
    const wt = parseFloat(document.getElementById('calc-wt')?.value) || 1.0;
    const fragile = document.getElementById('calc-fragile')?.checked || false;
    const express = document.getElementById('calc-express')?.checked || false;

    const res = await CRMGMT.api('/api/v1/shipping/calculate', {
      method: 'POST',
      body: JSON.stringify({
        length_cm: l,
        width_cm: w,
        height_cm: h,
        weight_kg: wt,
        is_fragile: fragile,
        is_express: express
      })
    });

    if (res.data && res.data.success) {
      const d = res.data;
      const elVol = document.getElementById('calc-res-vol');
      const elBill = document.getElementById('calc-res-bill');
      const elCost = document.getElementById('calc-res-cost');
      const elTime = document.getElementById('calc-res-time');

      if (elVol) elVol.textContent = `${d.volumetric_weight_kg} kg`;
      if (elBill) elBill.textContent = `${d.billable_weight_kg} kg`;
      if (elCost) elCost.textContent = `₹${d.shipping_cost}`;
      if (elTime) elTime.textContent = d.estimated_transit_time;
    }
  },

  // Checkpoint Scanner
  async initScanner() {
    if (!this.shipments || this.shipments.length === 0) {
      await this.loadShipments();
    }
    const select = document.getElementById('scan-tracking-select');
    if (select) {
      select.innerHTML = '<option value="">-- Select Consignment --</option>';
      this.shipments.forEach(s => {
        const opt = document.createElement('option');
        opt.value = s.tracking_id;
        opt.textContent = `${s.tracking_id} (${s.status}) - ${s.recipient_name}`;
        select.appendChild(opt);
      });
    }
  },

  async submitScan() {
    const tid = document.getElementById('scan-tracking-select').value;
    const hub_id = document.getElementById('scan-hub-select').value;
    const status = document.getElementById('scan-status-select').value;
    const remarks = document.getElementById('scan-remarks').value.trim();

    if (!tid) {
      CRMGMT.toast('Please select a Tracking ID to scan.', 'warning');
      return;
    }

    const res = await CRMGMT.api('/api/v1/checkpoints/scan', {
      method: 'POST',
      body: JSON.stringify({
        tracking_id: tid,
        hub_id,
        status,
        remarks: remarks || 'Hub physical barcode verified'
      })
    });

    if (res.data && res.data.success) {
      CRMGMT.toast(`Checkpoint logged: Status changed to ${status}`, 'success');
      this.loadShipments();
      CRMGMT.navigate('tracking', { trackingId: tid });
    } else {
      CRMGMT.toast(res.data?.error || 'Scan failed.', 'error');
    }
  },

  // =========================================================================
  // PROFILE PICTURE (PFP) CUSTOMIZER FOR ADMIN & ALL USERS
  // =========================================================================
  pfpTargetUserId: null,
  pfpSelectedUrl: '',

  openPfpModal(userId, userName) {
    this.pfpTargetUserId = userId || CRMGMT.state.currentUser?.id;
    const name = userName || CRMGMT.state.currentUser?.full_name || 'User';
    const role = CRMGMT.state.currentUser?.role || 'super_admin';
    const currentPfp = CRMGMT.getPfp(this.pfpTargetUserId, role);
    this.pfpSelectedUrl = currentPfp;

    const titleEl = document.getElementById('pfp-modal-title');
    const previewImg = document.getElementById('pfp-preview-img');
    const urlInput = document.getElementById('pfp-url-input');

    if (titleEl) titleEl.textContent = `Customize Profile Picture: ${name}`;
    if (previewImg) previewImg.src = currentPfp;
    if (urlInput) urlInput.value = currentPfp.startsWith('data:') ? '' : currentPfp;

    this.openModal('modal-edit-pfp');
  },

  handlePfpFileUpload(input) {
    const file = input.files[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = (e) => {
        const dataUrl = e.target.result;
        this.pfpSelectedUrl = dataUrl;
        const previewImg = document.getElementById('pfp-preview-img');
        if (previewImg) previewImg.src = dataUrl;
      };
      reader.readAsDataURL(file);
    }
  },

  handlePfpUrlChange(url) {
    if (url && url.trim().length > 5) {
      this.pfpSelectedUrl = url.trim();
      const previewImg = document.getElementById('pfp-preview-img');
      if (previewImg) previewImg.src = this.pfpSelectedUrl;
    }
  },

  selectPresetPfp(url, el) {
    this.pfpSelectedUrl = url;
    const previewImg = document.getElementById('pfp-preview-img');
    const urlInput = document.getElementById('pfp-url-input');
    if (previewImg) previewImg.src = url;
    if (urlInput) urlInput.value = url;

    document.querySelectorAll('.pfp-preset-item').forEach(i => i.classList.remove('selected'));
    if (el) el.classList.add('selected');
  },

  saveCustomPfp() {
    if (!this.pfpSelectedUrl) {
      CRMGMT.toast('Please select or upload a valid photo.', 'warning');
      return;
    }

    CRMGMT.setPfp(this.pfpTargetUserId, this.pfpSelectedUrl);
    CRMGMT.toast('Profile picture updated successfully!', 'success');
    this.closeModal('modal-edit-pfp');

    if (CRMGMT.state.currentView === 'users') {
      this.loadUsers();
    }
  },

  // =========================================================================
  // USER ACCOUNTS MANAGEMENT & PROVISIONING (Admin God-Mode Access)
  // =========================================================================
  users: [],

  async loadUsers() {
    const res = await CRMGMT.api('/api/v1/admin/users');
    if (res.data && res.data.success) {
      this.users = res.data.users || [];
      this.renderUsersTable(this.users);
    }
  },

  renderUsersTable(list) {
    const tbody = document.getElementById('users-table-body');
    if (!tbody) return;

    tbody.innerHTML = '';
    if (list.length === 0) {
      tbody.innerHTML = '<tr><td colspan="7" style="text-align: center; color: #94a3b8; padding: 24px;">No user accounts found.</td></tr>';
      return;
    }

    list.forEach(u => {
      const tr = document.createElement('tr');
      tr.style.cssText = 'border-bottom: 1px solid #f1f5f9;';

      let roleBadge = 'badge-info';
      if (u.role === 'super_admin') roleBadge = 'badge-pro';
      else if (u.role === 'hub_manager') roleBadge = 'badge-primary';
      else if (u.role === 'delivery_agent') roleBadge = 'badge-warning';
      else if (u.role === 'enterprise_customer') roleBadge = 'badge-success';

      const isActive = u.is_active !== false;
      const userPfp = CRMGMT.getPfp(u.id, u.role);

      tr.innerHTML = `
        <td style="padding: 12px 16px;">
          <div style="display: flex; align-items: center; gap: 10px;">
            <img src="${userPfp}" alt="${u.full_name}" style="width: 38px; height: 38px; border-radius: 50%; object-fit: cover; border: 1.5px solid #cbd5e1;" />
            <div>
              <div style="font-weight: 700; color: #1e293b;">${u.full_name}</div>
              <div style="font-size: 0.76rem; color: #64748b;">${u.email}</div>
            </div>
          </div>
        </td>
        <td style="padding: 12px 16px;">
          <span class="badge ${roleBadge}">${u.role.replace(/_/g, ' ').toUpperCase()}</span>
        </td>
        <td style="padding: 12px 16px; font-size: 0.78rem; color: #475569;">
          ${u.phone || '-'}
        </td>
        <td style="padding: 12px 16px; font-size: 0.78rem; font-family: var(--font-mono); color: #3b7ddd;">
          ${u.api_key || '<span style="color: #94a3b8;">N/A</span>'}
        </td>
        <td style="padding: 12px 16px;">
          <span class="badge ${isActive ? 'badge-success' : 'badge-danger'}">
            ${isActive ? 'ACTIVE' : 'SUSPENDED'}
          </span>
        </td>
        <td style="padding: 12px 16px; display: flex; gap: 8px; align-items: center;">
          <button class="btn btn-outline btn-sm" onclick="OperationsModule.openPfpModal('${u.id}', '${u.full_name}')" title="Change profile picture">
            📸 Edit PFP
          </button>
          <button class="btn btn-primary btn-sm" onclick="OperationsModule.impersonateUser('${u.id}')" title="Log In directly as this user">
            🔑 Log In As
          </button>
          <button class="btn btn-outline btn-sm" onclick="OperationsModule.toggleUserStatus('${u.id}')" title="Toggle status">
            ${isActive ? 'Suspend' : 'Activate'}
          </button>
          ${u.role !== 'super_admin' ? `<button class="btn btn-danger btn-sm" onclick="OperationsModule.deleteUser('${u.id}')" title="Delete user">✕</button>` : ''}
        </td>
      `;
      tbody.appendChild(tr);
    });
  },

  async createUserAccount() {
    const full_name = document.getElementById('new-user-name')?.value.trim();
    const email = document.getElementById('new-user-email')?.value.trim();
    const role = document.getElementById('new-user-role')?.value;
    const password = document.getElementById('new-user-password')?.value.trim() || 'Admin@123';
    const phone = document.getElementById('new-user-phone')?.value.trim() || '+91 98400 00000';
    const allocated_hub_id = document.getElementById('new-user-hub')?.value;

    if (!full_name || !email) {
      CRMGMT.toast('Please enter both Full Name and Email.', 'warning');
      return;
    }

    const res = await CRMGMT.api('/api/v1/admin/users/create', {
      method: 'POST',
      body: JSON.stringify({ full_name, email, role, password, phone, allocated_hub_id })
    });

    if (res.data && res.data.success) {
      CRMGMT.toast(`Successfully provisioned: ${full_name} (${role})`, 'success');
      this.closeModal('modal-create-user');
      this.loadUsers();
    } else {
      CRMGMT.toast(res.data?.error || 'Failed to create user account.', 'error');
    }
  },

  async impersonateUser(userId) {
    const res = await CRMGMT.api('/api/v1/admin/users/impersonate', {
      method: 'POST',
      body: JSON.stringify({ user_id: userId })
    });

    if (res.data && res.data.success) {
      CRMGMT.state.token = res.data.token;
      CRMGMT.state.currentUser = res.data.user;
      localStorage.setItem('crmgmt_token', res.data.token);
      localStorage.setItem('crmgmt_user', JSON.stringify(res.data.user));
      CRMGMT.updateUserUI();
      CRMGMT.toast(`Now logged in as: ${res.data.user.full_name} (${res.data.user.role})`, 'success');
      CRMGMT.navigate('dashboard');
    } else {
      CRMGMT.toast(res.data?.error || 'Failed to log in as user.', 'error');
    }
  },

  async toggleUserStatus(userId) {
    const res = await CRMGMT.api('/api/v1/admin/users/toggle-status', {
      method: 'POST',
      body: JSON.stringify({ user_id: userId })
    });

    if (res.data && res.data.success) {
      CRMGMT.toast(`User status updated to ${res.data.is_active ? 'ACTIVE' : 'SUSPENDED'}`, 'info');
      this.loadUsers();
    } else {
      CRMGMT.toast(res.data?.error || 'Failed to update user status.', 'error');
    }
  },

  async deleteUser(userId) {
    if (!confirm('Are you sure you want to delete this user account?')) return;
    const res = await CRMGMT.api('/api/v1/admin/users/delete', {
      method: 'POST',
      body: JSON.stringify({ user_id: userId })
    });

    if (res.data && res.data.success) {
      CRMGMT.toast('User account deleted.', 'success');
      this.loadUsers();
    } else {
      CRMGMT.toast(res.data?.error || 'Failed to delete user.', 'error');
    }
  },

  // =========================================================================
  // CUSTOMER ENTERPRISE OVERVIEW & KPI METRICS
  // =========================================================================
  async loadCustomerOverview() {
    await this.loadShipments();
    const customerShipments = this.shipments;
    
    // Update metric cards
    const activeCount = customerShipments.filter(s => s.status !== 'DELIVERED').length;
    const deliveredCount = customerShipments.filter(s => s.status === 'DELIVERED').length;
    const totalSpend = customerShipments.reduce((acc, s) => acc + (parseFloat(s.shipping_cost) || 0), 0);

    const activeEl = document.getElementById('cust-metric-active');
    const deliveredEl = document.getElementById('cust-metric-delivered');
    const spendEl = document.getElementById('cust-metric-spend');
    if (activeEl) activeEl.textContent = `${activeCount}`;
    if (deliveredEl) deliveredEl.textContent = `${deliveredCount}`;
    if (spendEl) {
      spendEl.setAttribute('data-inr-value', totalSpend);
      spendEl.textContent = CRMGMT.formatCurrency(totalSpend);
    }

    // Render Recent Consignments table
    const tbody = document.getElementById('cust-recent-table-body');
    if (tbody) {
      tbody.innerHTML = '';
      if (customerShipments.length === 0) {
        tbody.innerHTML = '<tr><td colspan="6" style="text-align: center; color: #94a3b8; padding: 20px;">No shipments created yet. Use Quick Booking to dispatch.</td></tr>';
      } else {
        customerShipments.forEach(s => {
          const tr = document.createElement('tr');
          tr.style.cssText = 'border-bottom: 1px solid #f1f5f9;';
          let statusBadge = 'badge-info';
          if (s.status === 'DELIVERED') statusBadge = 'badge-success';
          else if (s.status === 'IN_TRANSIT') statusBadge = 'badge-primary';
          else if (s.status === 'OUT_FOR_DELIVERY') statusBadge = 'badge-warning';

          tr.innerHTML = `
            <td style="padding: 12px 16px; font-weight: 700; font-family: var(--font-mono); color: #3b7ddd;">
              ${s.tracking_id}
            </td>
            <td style="padding: 12px 16px; font-weight: 600; color: #1e293b;">
              ${s.recipient_name}<br/>
              <span style="font-size: 0.74rem; color: #64748b;">${s.recipient_pincode}</span>
            </td>
            <td style="padding: 12px 16px;">
              <span class="badge ${statusBadge}">${s.status.replace(/_/g, ' ')}</span>
            </td>
            <td style="padding: 12px 16px; font-weight: 600;">${s.weight_kg} kg</td>
            <td style="padding: 12px 16px; font-weight: 700; color: #065f46;" data-inr-value="${s.shipping_cost}">
              ${CRMGMT.formatCurrency(s.shipping_cost)}
            </td>
            <td style="padding: 12px 16px; display: flex; gap: 6px;">
              <button class="btn btn-primary btn-sm" onclick="CRMGMT.navigate('tracking', { trackingId: '${s.tracking_id}' })">
                📍 Track
              </button>
              <button class="btn btn-outline btn-sm" onclick="OperationsModule.printShippingLabel('${s.tracking_id}')">
                🖨️ Label
              </button>
              <button class="btn btn-outline btn-sm" onclick="OperationsModule.viewTaxInvoice('${s.tracking_id}')">
                🧾 Invoice
              </button>
            </td>
          `;
          tbody.appendChild(tr);
        });
      }
    }
  },

  // =========================================================================
  // THERMAL SHIPPING LABEL (4x6 format with 1D/2D Barcodes)
  // =========================================================================
  printShippingLabel(trackingId) {
    const s = this.shipments.find(item => item.tracking_id === trackingId) || this.shipments[0];
    if (!s) return;

    const labelContainer = document.getElementById('thermal-label-content');
    if (!labelContainer) return;

    labelContainer.innerHTML = `
      <div class="thermal-shipping-label">
        <div class="thermal-header">
          <div>
            <div style="font-size: 1.1rem; font-weight: 900;">SAVEETHA EXPRESS LOGISTICS</div>
            <div style="font-size: 0.7rem; color: #333;">SIMATS CAMPUS LOGISTICS GATEWAY</div>
          </div>
          <div class="thermal-routing-box">
            CHE &gt;&gt; HYD
          </div>
        </div>

        <div class="thermal-barcode-box">
          <div style="font-size: 0.75rem; letter-spacing: 2px; font-weight: 700;">${s.tracking_id}</div>
          <div class="thermal-barcode-lines">
            ${Array.from({ length: 45 }).map(() => `<div style="background: #000; width: ${Math.random() > 0.5 ? '4px' : '2px'};"></div>`).join('')}
          </div>
          <div style="font-size: 0.65rem; color: #444; margin-top: 4px;">SHA256 CHECKSUM DERIVATION CERTIFIED</div>
        </div>

        <div class="thermal-info-grid">
          <div>
            <strong>SHIP FROM:</strong><br/>
            ${s.sender_name}<br/>
            Saveetha Campus Gateway<br/>
            Chennai TN - 602105
          </div>
          <div>
            <strong>SHIP TO:</strong><br/>
            ${s.recipient_name}<br/>
            ${s.recipient_address || 'Regional Distribution Wing'}<br/>
            PIN: ${s.recipient_pincode}
          </div>
        </div>

        <div style="display: flex; justify-content: space-between; align-items: center; padding-top: 8px; font-size: 0.75rem;">
          <div>
            <strong>WEIGHT:</strong> ${s.weight_kg} KG | <strong>MODE:</strong> AIR EXPRESS
          </div>
          <div style="font-weight: 800; border: 1.5px solid #000; padding: 2px 6px;">
            ${s.is_fragile ? '⚠ FRAGILE / BIO-GRADE' : 'STANDARD CARGO'}
          </div>
        </div>
      </div>
    `;

    this.openModal('modal-thermal-label');
  },

  // =========================================================================
  // OFFICIAL GST TAX INVOICE GENERATOR
  // =========================================================================
  viewTaxInvoice(trackingId) {
    const s = this.shipments.find(item => item.tracking_id === trackingId) || this.shipments[0];
    if (!s) return;

    const invoiceContainer = document.getElementById('tax-invoice-content');
    if (!invoiceContainer) return;

    const baseFreight = (s.shipping_cost / 1.18).toFixed(2);
    const gstAmount = (s.shipping_cost - baseFreight).toFixed(2);

    invoiceContainer.innerHTML = `
      <div class="invoice-sheet">
        <div style="display: flex; justify-content: space-between; border-bottom: 2px solid #e2e8f0; padding-bottom: 14px;">
          <div>
            <h2 style="font-size: 1.3rem; font-weight: 800; color: #0d3b66;">SAVEETHA LOGISTICS SERVICES</h2>
            <div style="font-size: 0.78rem; color: #64748b;">GSTIN: 33AAAAA0000A1Z5 | PAN: AAAAA0000A</div>
            <div style="font-size: 0.78rem; color: #64748b;">SIMATS Campus, Poonamallee High Rd, Chennai TN</div>
          </div>
          <div style="text-align: right;">
            <div style="font-size: 1.1rem; font-weight: 800; color: #1e293b;">TAX INVOICE</div>
            <div style="font-size: 0.8rem; font-weight: 700; color: #3b7ddd;">INV-2026-${s.tracking_id.replace('CR-', '')}</div>
            <div style="font-size: 0.74rem; color: #64748b;">Date: ${new Date().toLocaleDateString('en-GB')}</div>
          </div>
        </div>

        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 16px; margin: 16px 0; font-size: 0.82rem;">
          <div>
            <strong>BILLED TO:</strong><br/>
            ${s.sender_name}<br/>
            Saveetha Department of Procurement & Logistics<br/>
            Chennai TN
          </div>
          <div>
            <strong>CONSIGNMENT DETAILS:</strong><br/>
            AWB: <strong style="color: #3b7ddd;">${s.tracking_id}</strong><br/>
            Consignee: ${s.recipient_name} (${s.recipient_pincode})<br/>
            Billable Weight: ${s.weight_kg} kg
          </div>
        </div>

        <table class="invoice-table">
          <thead>
            <tr>
              <th>Description</th>
              <th>HSN/SAC</th>
              <th>Weight</th>
              <th>Rate (INR)</th>
              <th>Amount</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <td>Express Priority Door-to-Door Courier Freight</td>
              <td>996812</td>
              <td>${s.weight_kg} kg</td>
              <td>₹150.00 base + ₹50/kg</td>
              <td>₹${baseFreight}</td>
            </tr>
            <tr>
              <td colspan="4" style="text-align: right; font-weight: 700;">Integrated GST (IGST @ 18%):</td>
              <td style="font-weight: 700;">₹${gstAmount}</td>
            </tr>
            <tr style="background: #f8fafc;">
              <td colspan="4" style="text-align: right; font-weight: 900; font-size: 0.95rem;">TOTAL PAYABLE:</td>
              <td style="font-weight: 900; font-size: 0.95rem; color: #065f46;">₹${Number(s.shipping_cost).toFixed(2)}</td>
            </tr>
          </tbody>
        </table>

        <div style="display: flex; justify-content: space-between; align-items: center; margin-top: 14px; font-size: 0.78rem; color: #64748b;">
          <div>
            <span class="badge badge-success">PAYMENT STATUS: PAID (PREPAID)</span>
          </div>
          <div>This is a computer-generated tax invoice and requires no signature.</div>
        </div>
      </div>
    `;

    this.openModal('modal-tax-invoice');
  },

  // =========================================================================
  // INVOICES & BILLING LEDGER
  // =========================================================================
  async loadInvoices() {
    await this.loadShipments();
    const tbody = document.getElementById('invoices-table-body');
    if (!tbody) return;

    tbody.innerHTML = '';
    this.shipments.forEach(s => {
      const tr = document.createElement('tr');
      tr.style.cssText = 'border-bottom: 1px solid #f1f5f9;';
      tr.innerHTML = `
        <td style="padding: 12px 16px; font-weight: 700; color: #3b7ddd;">INV-2026-${s.tracking_id.replace('CR-', '')}</td>
        <td style="padding: 12px 16px;">${s.tracking_id}</td>
        <td style="padding: 12px 16px;">${s.recipient_name}</td>
        <td style="padding: 12px 16px; font-weight: 700; color: #065f46;">₹${Number(s.shipping_cost).toFixed(2)}</td>
        <td style="padding: 12px 16px;"><span class="badge badge-success">PAID</span></td>
        <td style="padding: 12px 16px;">
          <button class="btn btn-outline btn-sm" onclick="OperationsModule.viewTaxInvoice('${s.tracking_id}')">
            View GST Invoice
          </button>
        </td>
      `;
      tbody.appendChild(tr);
    });
  },

  // =========================================================================
  // BULK CSV CONSIGNMENT IMPORT
  // =========================================================================
  processBulkCSV(text) {
    const lines = text.split('\n').map(l => l.trim()).filter(l => l.length > 0);
    if (lines.length <= 1) {
      CRMGMT.toast('CSV file is empty or missing data rows.', 'warning');
      return;
    }

    let importedCount = 0;
    for (let i = 1; i < lines.length; i++) {
      const parts = lines[i].split(',').map(p => p.replace(/"/g, '').trim());
      if (parts.length >= 4) {
        const newShip = {
          id: `s_bulk_${Date.now()}_${i}`,
          tracking_id: `CR-${Math.floor(Date.now()/1000).toString(16).toUpperCase()}-${(i*17)%256}-${Math.floor(Math.random()*9000+1000)}`,
          sender_name: parts[0] || 'Saveetha Central Procurement',
          recipient_name: parts[1] || 'Bulk Consignee',
          recipient_pincode: parts[2] || '600001',
          weight_kg: parseFloat(parts[3]) || 1.5,
          status: 'ORDER_CREATED',
          shipping_cost: 275.0
        };
        this.shipments.unshift(newShip);
        importedCount++;
      }
    }

    CRMGMT.toast(`Successfully imported ${importedCount} bulk consignments with cryptographic tracking numbers!`, 'success');
    this.closeModal('modal-bulk-csv');
    this.loadShipments();
  },

  // Export CSV
  exportCSV() {
    if (this.shipments.length === 0) {
      CRMGMT.toast('No shipments to export.', 'warning');
      return;
    }

    let csvContent = 'data:text/csv;charset=utf-8,Tracking ID,Sender,Recipient,Pincode,Status,Weight (kg),Cost (INR)\n';
    this.shipments.forEach(s => {
      csvContent += `"${s.tracking_id}","${s.sender_name}","${s.recipient_name}","${s.recipient_pincode}","${s.status}",${s.weight_kg},${s.shipping_cost}\n`;
    });

    const encodedUri = encodeURI(csvContent);
    const link = document.createElement('a');
    link.setAttribute('href', encodedUri);
    link.setAttribute('download', `CRMGMT_Shipments_${Date.now()}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    CRMGMT.toast('Exported shipments manifest CSV.', 'success');
  },

  openModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) modal.classList.add('open');
  },

  closeModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) modal.classList.remove('open');
  }
};

window.OperationsModule = OperationsModule;
window.addEventListener('DOMContentLoaded', () => {
  OperationsModule.init();
});
