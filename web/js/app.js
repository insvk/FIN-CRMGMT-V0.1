/* CRMGMT v0.1 - Main Application State & Client Layer */

const CRMGMT = {
  state: {
    currentUser: null,
    token: null,
    currentView: 'login', // 'login', 'dashboard', 'customer_dashboard', 'shipments', 'tracking', 'scanner', 'calculator', 'users', 'invoices', 'bulk_dispatch'
    hubs: [],
    activeTrackingId: null,
    currency: 'INR', // 'INR', 'USD', 'EUR'
    currencyRates: { INR: 1.0, USD: 0.012, EUR: 0.011 },
    currencySymbols: { INR: '₹', USD: '$', EUR: '€' },
    notifications: [
      { id: 1, title: 'Consignment Out for Delivery', body: 'CR-68D3F12A-B4-9F81 assigned to SIMATS Dispatch Unit 01.', time: '10 mins ago', type: 'delivery', unread: true },
      { id: 2, title: 'Interstate Hub Departure', body: 'Express Truck CH-HYD-882 departed Saveetha Chennai Central Gateway.', time: '1 hour ago', type: 'transit', unread: true },
      { id: 3, title: 'Security Audit Verified', body: 'Cryptographic SHA256 checksum verified for consignment CR-68D4255E-A1-19B4.', time: '3 hours ago', type: 'security', unread: false },
      { id: 4, title: 'System Heartbeat OK', body: 'Supabase PostgreSQL 15 & C17 Server running with 0% dropped packets.', time: '5 hours ago', type: 'system', unread: false }
    ]
  },

  // Currency Converter & Formatter
  setCurrency(curr) {
    this.state.currency = curr;
    document.querySelectorAll('.currency-label').forEach(el => el.textContent = curr);
    document.querySelectorAll('[data-inr-value]').forEach(el => {
      const val = parseFloat(el.getAttribute('data-inr-value') || '0');
      el.textContent = this.formatCurrency(val);
    });
    this.toast(`Switched currency to ${curr} (${this.state.currencySymbols[curr]})`, 'info');
  },

  formatCurrency(amountInINR) {
    const rate = this.state.currencyRates[this.state.currency] || 1.0;
    const symbol = this.state.currencySymbols[this.state.currency] || '₹';
    const converted = amountInINR * rate;
    return `${symbol}${converted.toLocaleString('en-IN', { minimumFractionDigits: 2, maximumFractionDigits: 2 })}`;
  },

  // Toggle Notification Drawer
  toggleNotifications() {
    const el = document.getElementById('notification-dropdown');
    if (!el) return;
    el.classList.toggle('open');
    this.renderNotifications();
  },

  renderNotifications() {
    const container = document.getElementById('notif-list-container');
    const badge = document.getElementById('notif-badge-count');
    if (!container) return;

    const unreadCount = this.state.notifications.filter(n => n.unread).length;
    if (badge) {
      badge.textContent = unreadCount;
      badge.style.display = unreadCount > 0 ? 'inline-block' : 'none';
    }

    container.innerHTML = '';
    this.state.notifications.forEach(n => {
      const div = document.createElement('div');
      div.className = 'notif-item';
      div.style.background = n.unread ? '#f8fafc' : '#ffffff';
      
      let icon = '📦';
      let iconBg = '#e0f2fe';
      if (n.type === 'transit') { icon = '🚚'; iconBg = '#fef3c7'; }
      else if (n.type === 'security') { icon = '🛡️'; iconBg = '#dcfce7'; }
      else if (n.type === 'system') { icon = '⚡'; iconBg = '#f3e8ff'; }

      div.innerHTML = `
        <div class="notif-icon-circle" style="background: ${iconBg};">${icon}</div>
        <div style="flex: 1;">
          <div style="font-size: 0.82rem; font-weight: 700; color: #1e293b;">${n.title}</div>
          <div style="font-size: 0.76rem; color: #64748b; margin-top: 2px;">${n.body}</div>
          <div style="font-size: 0.7rem; color: #94a3b8; margin-top: 4px;">${n.time}</div>
        </div>
      `;
      div.onclick = () => {
        n.unread = false;
        this.renderNotifications();
      };
      container.appendChild(div);
    });
  },

  // API Client
  async api(endpoint, options = {}) {
    const defaultHeaders = {
      'Content-Type': 'application/json'
    };

    if (this.state.token) {
      defaultHeaders['Authorization'] = `Bearer ${this.state.token}`;
    }

    options.headers = { ...defaultHeaders, ...(options.headers || {}) };

    try {
      const res = await fetch(endpoint, options);
      const data = await res.json();
      return { status: res.status, ok: res.ok, data };
    } catch (err) {
      console.warn(`[API] Fetch error for ${endpoint}, falling back if offline:`, err);
      // Fallback for standalone / client-side mock if backend is unavailable
      return this.mockApiFallback(endpoint, options);
    }
  },

  // Fallback simulator for offline browser preview
  mockApiFallback(endpoint, options) {
    console.log(`[MOCK_API] Handling offline request for ${endpoint}`);
    if (endpoint === '/api/v1/auth/login') {
      const body = JSON.parse(options.body || '{}');
      return {
        status: 200, ok: true,
        data: {
          success: true,
          token: 'mock_jwt_token_crmgmt_v01',
          user: {
            id: 'u0000000-0000-0000-0000-000000000001',
            email: body.email || 'admin@crmgmt.io',
            full_name: 'SIMATS Chief Systems Administrator',
            role: body.email?.includes('admin') ? 'super_admin' : (body.email?.includes('hub') ? 'hub_manager' : 'standard_customer'),
            phone: '+91 98400 11223'
          }
        }
      };
    }
    return { status: 200, ok: true, data: { success: true } };
  },

  // Toast notifications
  toast(message, type = 'info') {
    const container = document.getElementById('toast-container');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `
      <div style="font-size: 1.1rem; line-height: 1;">
        ${type === 'success' ? '✓' : (type === 'error' ? '✕' : 'ℹ')}
      </div>
      <div>
        <div style="font-weight: 600;">${type.toUpperCase()}</div>
        <div style="color: #64748b; font-size: 0.8rem; margin-top: 2px;">${message}</div>
      </div>
    `;

    container.appendChild(toast);
    setTimeout(() => {
      toast.style.animation = 'fadeOut 0.3s ease-out forwards';
      setTimeout(() => toast.remove(), 300);
    }, 4000);
  },

  // View Routing
  navigate(viewName, params = {}) {
    this.state.currentView = viewName;
    const authView = document.getElementById('view-auth');
    const mainApp = document.getElementById('view-main-app');

    // Deactivate all subviews
    document.querySelectorAll('.view-section, .dashboard-subview').forEach(el => {
      el.classList.remove('active');
    });

    // Update URL hash without reload
    if (window.location.hash !== `#${viewName}`) {
      window.location.hash = `#${viewName}`;
    }

    if (viewName === 'login' || viewName === 'register') {
      if (authView) authView.style.display = 'flex';
      if (mainApp) mainApp.style.display = 'none';

      if (viewName === 'register' && window.switchAuthMode) {
        window.switchAuthMode('register');
      } else if (window.switchAuthMode) {
        window.switchAuthMode('login');
      }
    } else {
      if (authView) authView.style.display = 'none';
      if (mainApp) mainApp.style.display = 'flex';

      // Auto-route customer to customer_dashboard if generic dashboard is requested
      const isCustomer = this.state.currentUser && (this.state.currentUser.role === 'standard_customer' || this.state.currentUser.role === 'enterprise_customer');
      let effectiveView = viewName;
      if (viewName === 'dashboard' && isCustomer) {
        effectiveView = 'customer_dashboard';
      }

      // Activate specific dashboard sub-view
      const targetSubView = document.getElementById(`subview-${effectiveView}`);
      if (targetSubView) {
        targetSubView.classList.add('active');
      } else {
        const defaultDash = document.getElementById(isCustomer ? 'subview-customer_dashboard' : 'subview-dashboard');
        if (defaultDash) defaultDash.classList.add('active');
      }

      // Update active sidebar nav
      document.querySelectorAll('.sidebar-link, .sidebar-sublink').forEach(link => {
        if (link.getAttribute('data-view') === effectiveView) {
          link.classList.add('active');
        } else {
          link.classList.remove('active');
        }
      });

      // View-specific triggers
      if (effectiveView === 'dashboard' && window.DashboardModule) {
        window.DashboardModule.loadAnalytics();
      } else if (effectiveView === 'customer_dashboard' && window.OperationsModule) {
        window.OperationsModule.loadCustomerOverview();
      } else if (effectiveView === 'tracking' && window.TrackingModule) {
        if (params.trackingId) {
          window.TrackingModule.lookup(params.trackingId);
        }
      } else if (effectiveView === 'shipments' && window.OperationsModule) {
        window.OperationsModule.loadShipments();
      } else if (effectiveView === 'scanner' && window.OperationsModule) {
        window.OperationsModule.initScanner();
      } else if (effectiveView === 'calculator' && window.OperationsModule) {
        window.OperationsModule.initCalculator();
      } else if (effectiveView === 'users' && window.OperationsModule) {
        window.OperationsModule.loadUsers();
      } else if (effectiveView === 'invoices' && window.OperationsModule) {
        window.OperationsModule.loadInvoices();
      }
    }
  },

  // Dynamic Role-Based Access Control (RBAC) UI Filtering
  switchRole(role) {
    if (!this.state.currentUser) return;
    this.state.currentUser.role = role;
    this.updateUserUI();
    this.toast(`Switched active portal view to: ${role.toUpperCase().replace('_', ' ')}`, 'info');
    
    // Smart redirect based on role
    if (role === 'standard_customer' || role === 'enterprise_customer') {
      this.navigate('customer_dashboard');
    } else {
      this.navigate('dashboard');
    }
  },

  updateUserUI() {
    const u = this.state.currentUser;
    if (!u) return;

    document.querySelectorAll('.user-name-display').forEach(el => el.textContent = u.full_name || 'SIMATS Chief Systems Administrator');
    document.querySelectorAll('.user-role-display').forEach(el => el.textContent = (u.role || 'Super Admin').replace('_', ' ').toUpperCase());
    document.querySelectorAll('.user-email-display').forEach(el => el.textContent = u.email);

    const roleSelector = document.getElementById('quick-role-select');
    if (roleSelector) roleSelector.value = u.role || 'super_admin';

    // Show/Hide Role-Specific Sidebar navigation
    const isCustomer = u.role === 'standard_customer' || u.role === 'enterprise_customer';
    const isAgent = u.role === 'delivery_agent';
    const isAdminOrManager = u.role === 'super_admin' || u.role === 'hub_manager';

    document.querySelectorAll('.nav-admin-only').forEach(el => {
      el.style.display = isAdminOrManager ? 'flex' : 'none';
    });
    document.querySelectorAll('.nav-customer-only').forEach(el => {
      el.style.display = isCustomer ? 'flex' : 'none';
    });
    document.querySelectorAll('.nav-agent-only').forEach(el => {
      el.style.display = (isAgent || isAdminOrManager) ? 'flex' : 'none';
    });
  },

  // Bootstrap Application
  init() {
    this.renderNotifications();

    // Check saved session
    const savedToken = localStorage.getItem('crmgmt_token');
    const savedUser = localStorage.getItem('crmgmt_user');

    if (savedToken && savedUser) {
      try {
        this.state.token = savedToken;
        this.state.currentUser = JSON.parse(savedUser);
        this.updateUserUI();
        const isCustomer = this.state.currentUser.role === 'standard_customer' || this.state.currentUser.role === 'enterprise_customer';
        this.navigate(isCustomer ? 'customer_dashboard' : 'dashboard');
      } catch (e) {
        localStorage.clear();
        this.navigate('login');
      }
    } else {
      this.navigate('login');
    }

    // Handle hash route changes
    window.addEventListener('hashchange', () => {
      const hash = window.location.hash.replace('#', '') || 'login';
      this.navigate(hash);
    });

    // Sidebar navigation clicks
    document.querySelectorAll('[data-view]').forEach(item => {
      item.addEventListener('click', (e) => {
        e.preventDefault();
        const v = item.getAttribute('data-view');
        this.navigate(v);
      });
    });

    // Mobile sidebar toggle
    const toggleBtn = document.getElementById('sidebar-toggle-btn');
    const sidebar = document.getElementById('main-sidebar');
    if (toggleBtn && sidebar) {
      toggleBtn.addEventListener('click', () => {
        sidebar.classList.toggle('mobile-open');
      });
    }

    // Close notification dropdown when clicking outside
    document.addEventListener('click', (e) => {
      const notifBtn = document.getElementById('btn-notif-toggle');
      const dropdown = document.getElementById('notification-dropdown');
      if (dropdown && notifBtn && !notifBtn.contains(e.target) && !dropdown.contains(e.target)) {
        dropdown.classList.remove('open');
      }
    });
  }
};

window.addEventListener('DOMContentLoaded', () => {
  CRMGMT.init();
});
