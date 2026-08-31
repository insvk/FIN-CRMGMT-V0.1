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
            full_name: 'Naresh S (SIMATS Chief Systems Administrator)',
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
    let parsedView = viewName;
    let effectiveParams = { ...params };
    if (typeof viewName === 'string' && viewName.includes('?')) {
      const parts = viewName.split('?');
      parsedView = parts[0];
      const searchParams = new URLSearchParams(parts[1]);
      if (searchParams.get('id')) effectiveParams.trackingId = searchParams.get('id');
      if (searchParams.get('track')) effectiveParams.trackingId = searchParams.get('track');
    }

    this.state.currentView = parsedView;
    const authView = document.getElementById('view-auth');
    const mainApp = document.getElementById('view-main-app');
    const publicTrackView = document.getElementById('view-public-tracking');

    // Deactivate all subviews
    document.querySelectorAll('.view-section, .dashboard-subview').forEach(el => {
      el.classList.remove('active');
    });

    // Public Live Tracking Portal (No Login Required)
    if (parsedView === 'public_track' || parsedView === 'track') {
      if (authView) authView.style.display = 'none';
      if (mainApp) mainApp.style.display = 'none';
      if (publicTrackView) publicTrackView.style.display = 'block';

      const targetId = effectiveParams.trackingId || 'CR-68D3F12A-B4-9F81';
      if (window.TrackingModule) {
        window.TrackingModule.lookupPublic(targetId);
      }
      return;
    }

    if (publicTrackView) publicTrackView.style.display = 'none';

    // Update URL hash without reload
    if (window.location.hash !== `#${parsedView}`) {
      window.location.hash = `#${parsedView}`;
    }

    if (parsedView === 'login' || parsedView === 'register') {
      if (authView) authView.style.display = 'flex';
      if (mainApp) mainApp.style.display = 'none';

      if (parsedView === 'register' && window.switchAuthMode) {
        window.switchAuthMode('register');
      } else if (window.switchAuthMode) {
        window.switchAuthMode('login');
      }
    } else {
      if (authView) authView.style.display = 'none';
      if (mainApp) mainApp.style.display = 'flex';

      // Auto-route customer to customer_dashboard if generic dashboard is requested
      const isCustomer = this.state.currentUser && (this.state.currentUser.role === 'standard_customer' || this.state.currentUser.role === 'enterprise_customer');
      let effectiveView = parsedView;
      if (parsedView === 'dashboard' && isCustomer) {
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
        setTimeout(() => {
          if (window.DashboardModule.hubMap) {
            window.DashboardModule.hubMap.invalidateSize();
          }
        }, 200);
      } else if (effectiveView === 'customer_dashboard' && window.OperationsModule) {
        window.OperationsModule.loadCustomerOverview();
      } else if (effectiveView === 'tracking' && window.TrackingModule) {
        if (effectiveParams.trackingId) {
          window.TrackingModule.lookup(effectiveParams.trackingId);
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

  // Curated Preset Avatars
  curatedPresets: [
    { name: 'Executive Administrator', url: 'https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=160&auto=format&fit=crop&q=80', role: 'super_admin' },
    { name: 'Chief Systems Architect', url: 'https://images.unsplash.com/photo-1507003211169-0a1dd7228f2d?w=160&auto=format&fit=crop&q=80', role: 'super_admin' },
    { name: 'Hub Operations Manager', url: 'https://images.unsplash.com/photo-1573496359142-b8d87734a5a2?w=160&auto=format&fit=crop&q=80', role: 'hub_manager' },
    { name: 'Hub Logistics Lead', url: 'https://images.unsplash.com/photo-1560250097-0b93528c311a?w=160&auto=format&fit=crop&q=80', role: 'hub_manager' },
    { name: 'Fleet Dispatch Agent', url: 'https://images.unsplash.com/photo-1500648767791-00dcc994a43e?w=160&auto=format&fit=crop&q=80', role: 'delivery_agent' },
    { name: 'Express Delivery Driver', url: 'https://images.unsplash.com/photo-1539571696357-5a69c17a67c6?w=160&auto=format&fit=crop&q=80', role: 'delivery_agent' },
    { name: 'Healthcare Procurement Lead', url: 'https://images.unsplash.com/photo-1580489944761-15a19d654956?w=160&auto=format&fit=crop&q=80', role: 'enterprise_customer' },
    { name: 'Enterprise Corporate Client', url: 'https://images.unsplash.com/photo-1573497019940-1c28c88b4f3e?w=160&auto=format&fit=crop&q=80', role: 'enterprise_customer' },
    { name: 'Retail Consignee & Client', url: 'https://images.unsplash.com/photo-1519085360753-af0119f7cbe7?w=160&auto=format&fit=crop&q=80', role: 'standard_customer' },
    { name: 'Client Logistics Liaison', url: 'https://images.unsplash.com/photo-1472099645785-5658abf4ff4e?w=160&auto=format&fit=crop&q=80', role: 'standard_customer' },
    { name: 'Saveetha BioMed Specialist', url: 'https://images.unsplash.com/photo-1622253692010-333f2da6031d?w=160&auto=format&fit=crop&q=80', role: 'enterprise_customer' },
    { name: 'Global Supply Analyst', url: 'https://images.unsplash.com/photo-1544005313-94ddf0286df2?w=160&auto=format&fit=crop&q=80', role: 'standard_customer' }
  ],

  // Client-side image compression & square cropping
  compressImageFile(file, maxWidth = 256, maxHeight = 256, quality = 0.85) {
    return new Promise((resolve, reject) => {
      if (!file || !file.type.startsWith('image/')) {
        return reject(new Error('Please select a valid image file.'));
      }
      const reader = new FileReader();
      reader.onload = (e) => {
        const img = new Image();
        img.onload = () => {
          const canvas = document.createElement('canvas');
          const minDim = Math.min(img.width, img.height);
          const startX = (img.width - minDim) / 2;
          const startY = (img.height - minDim) / 2;

          canvas.width = Math.min(maxWidth, minDim);
          canvas.height = Math.min(maxHeight, minDim);

          const ctx = canvas.getContext('2d');
          // Fill background with white to prevent black artifacts on transparent PNGs/WebPs
          ctx.fillStyle = '#ffffff';
          ctx.fillRect(0, 0, canvas.width, canvas.height);
          ctx.drawImage(img, startX, startY, minDim, minDim, 0, 0, canvas.width, canvas.height);

          try {
            const dataUrl = canvas.toDataURL('image/jpeg', quality);
            resolve(dataUrl);
          } catch (err) {
            resolve(e.target.result);
          }
        };
        img.onerror = () => reject(new Error('Failed to load image.'));
        img.src = e.target.result;
      };
      reader.onerror = () => reject(new Error('Failed to read file.'));
      reader.readAsDataURL(file);
    });
  },

  // Profile Picture (PFP) Manager
  getPfp(userId, role, userObj) {
    if (userObj && userObj.avatar_url && typeof userObj.avatar_url === 'string' && userObj.avatar_url.trim().length > 0) {
      return userObj.avatar_url.trim();
    }
    try {
      const pfps = JSON.parse(localStorage.getItem('crmgmt_custom_pfps') || '{}');
      if (userId && pfps[userId]) return pfps[userId];
    } catch(e) {}

    if (userId && this.state.currentUser && this.state.currentUser.id === userId && this.state.currentUser.avatar_url) {
      return this.state.currentUser.avatar_url;
    }

    if (window.OperationsModule && window.OperationsModule.users) {
      const u = window.OperationsModule.users.find(x => x.id === userId);
      if (u && u.avatar_url) return u.avatar_url;
    }

    // Default curated avatars
    if (role === 'hub_manager') return 'https://images.unsplash.com/photo-1573496359142-b8d87734a5a2?w=120&auto=format&fit=crop&q=80';
    if (role === 'delivery_agent') return 'https://images.unsplash.com/photo-1500648767791-00dcc994a43e?w=120&auto=format&fit=crop&q=80';
    if (role === 'enterprise_customer') return 'https://images.unsplash.com/photo-1580489944761-15a19d654956?w=120&auto=format&fit=crop&q=80';
    if (role === 'standard_customer') return 'https://images.unsplash.com/photo-1519085360753-af0119f7cbe7?w=120&auto=format&fit=crop&q=80';
    return 'https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=120&auto=format&fit=crop&q=80';
  },

  async setPfp(userId, pfpUrl, syncToServer = true) {
    if (!userId || !pfpUrl) return;
    try {
      const pfps = JSON.parse(localStorage.getItem('crmgmt_custom_pfps') || '{}');
      pfps[userId] = pfpUrl;
      localStorage.setItem('crmgmt_custom_pfps', JSON.stringify(pfps));
    } catch(e) {}

    if (this.state.currentUser && (this.state.currentUser.id === userId || !userId)) {
      this.state.currentUser.avatar_url = pfpUrl;
      localStorage.setItem('crmgmt_user', JSON.stringify(this.state.currentUser));
    }

    if (window.OperationsModule && window.OperationsModule.users) {
      const u = window.OperationsModule.users.find(x => x.id === userId);
      if (u) u.avatar_url = pfpUrl;
    }

    this.updateUserUI();

    if (syncToServer) {
      try {
        await this.api('/api/v1/auth/pfp', {
          method: 'POST',
          body: JSON.stringify({ user_id: userId, avatar_url: pfpUrl })
        });
      } catch (err) {
        console.warn('Could not sync avatar to server:', err);
      }
    }
  },

  updateUserUI() {
    const u = this.state.currentUser;
    if (!u) return;

    document.querySelectorAll('.user-name-display').forEach(el => el.textContent = u.full_name || 'Naresh S (SIMATS Chief Systems Administrator)');
    document.querySelectorAll('.user-role-display').forEach(el => el.textContent = (u.role || 'Super Admin').replace(/_/g, ' ').toUpperCase());
    document.querySelectorAll('.user-email-display').forEach(el => el.textContent = u.email || '');

    // Update Profile Picture with automatic fallback on broken links
    const activePfp = this.getPfp(u.id, u.role, u);
    document.querySelectorAll('.sidebar-avatar, .nav-avatar-img, .current-user-avatar, .cust-overview-avatar').forEach(el => {
      el.src = activePfp;
      el.onerror = () => {
        el.onerror = null;
        el.src = 'https://images.unsplash.com/photo-1534528741775-53994a69daeb?w=120&auto=format&fit=crop&q=80';
      };
    });

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

    // Check if initial URL specifies public tracking
    const initialHash = window.location.hash.replace('#', '');
    const urlParams = new URLSearchParams(window.location.search);
    const queryTrack = urlParams.get('track') || urlParams.get('id');

    if (initialHash.startsWith('public_track') || initialHash.startsWith('track') || queryTrack) {
      const tid = queryTrack || initialHash.split('?id=')[1] || initialHash.split('?track=')[1] || 'CR-68D3F12A-B4-9F81';
      this.navigate('public_track', { trackingId: tid });
    } else {
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
