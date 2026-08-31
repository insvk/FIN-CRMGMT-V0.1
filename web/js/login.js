/* CRMGMT v0.1 - Saveetha Authentication & User Registration Controller */

const AuthModule = {
  mode: 'login', // 'login' or 'register'

  selectedRegisterPfp: 'https://images.unsplash.com/photo-1519085360753-af0119f7cbe7?w=160&auto=format&fit=crop&q=80',
  isCustomUploaded: false,

  init() {
    this.bindEvents();
    this.initRegisterPfp();
  },

  initRegisterPfp() {
    this.renderPresets();
    this.updatePreview(this.selectedRegisterPfp);

    const roleSelect = document.getElementById('reg-role');
    if (roleSelect) {
      roleSelect.addEventListener('change', (e) => {
        if (!this.isCustomUploaded) {
          const role = e.target.value;
          const defaultPreset = (CRMGMT.curatedPresets || []).find(p => p.role === role) || (CRMGMT.curatedPresets || [])[8];
          if (defaultPreset) {
            this.selectPreset(defaultPreset.url, null, false);
          }
        }
      });
    }
  },

  renderPresets() {
    const container = document.getElementById('reg-pfp-presets-grid');
    if (!container || !CRMGMT.curatedPresets) return;
    container.innerHTML = '';

    CRMGMT.curatedPresets.forEach((p) => {
      const img = document.createElement('img');
      img.src = p.url;
      img.className = `pfp-preset-item reg-preset-item ${p.url === this.selectedRegisterPfp ? 'selected' : ''}`;
      img.title = `${p.name} (${p.role.replace('_', ' ')})`;
      img.alt = p.name;
      img.onclick = () => this.selectPreset(p.url, img, true);
      container.appendChild(img);
    });
  },

  selectPreset(url, el, manual = true) {
    this.selectedRegisterPfp = url;
    if (manual) this.isCustomUploaded = false;
    this.updatePreview(url);

    document.querySelectorAll('.reg-preset-item').forEach(i => {
      if (i.src === url || (el && i === el)) {
        i.classList.add('selected');
      } else {
        i.classList.remove('selected');
      }
    });

    const urlInput = document.getElementById('reg-pfp-url-input');
    if (urlInput) urlInput.value = url.startsWith('data:') ? '' : url;
  },

  async handleFileUpload(input) {
    const file = input.files?.[0];
    if (!file) return;

    try {
      CRMGMT.toast('Optimizing and loading photo...', 'info');
      const dataUrl = await CRMGMT.compressImageFile(file, 256, 256, 0.85);
      this.selectedRegisterPfp = dataUrl;
      this.isCustomUploaded = true;
      this.updatePreview(dataUrl);

      document.querySelectorAll('.reg-preset-item').forEach(i => i.classList.remove('selected'));
      const urlInput = document.getElementById('reg-pfp-url-input');
      if (urlInput) urlInput.value = '';
      CRMGMT.toast('Profile photo selected!', 'success');
    } catch (err) {
      CRMGMT.toast(err.message || 'Failed to process image.', 'error');
    }
  },

  handleUrlChange(url) {
    if (url && url.trim().length > 5) {
      this.selectedRegisterPfp = url.trim();
      this.isCustomUploaded = true;
      this.updatePreview(this.selectedRegisterPfp);
      document.querySelectorAll('.reg-preset-item').forEach(i => {
        if (i.src === this.selectedRegisterPfp) i.classList.add('selected');
        else i.classList.remove('selected');
      });
    }
  },

  updatePreview(url) {
    const preview = document.getElementById('reg-pfp-preview');
    if (preview) preview.src = url;
  },

  bindEvents() {
    // Password toggle
    const pwToggle = document.getElementById('login-pw-toggle');
    const pwInput = document.getElementById('login-password');
    if (pwToggle && pwInput) {
      pwToggle.addEventListener('click', () => {
        const type = pwInput.getAttribute('type') === 'password' ? 'text' : 'password';
        pwInput.setAttribute('type', type);
        pwToggle.innerHTML = type === 'password'
          ? `<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>`
          : `<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line></svg>`;
      });
    }

    // Login Form Submit
    const loginForm = document.getElementById('form-login');
    if (loginForm) {
      loginForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const email = document.getElementById('login-email').value.trim();
        const password = document.getElementById('login-password').value.trim();

        if (!email || !password) {
          CRMGMT.toast('Please enter both Email and Password.', 'error');
          return;
        }

        const btn = document.getElementById('btn-login-submit');
        if (btn) { btn.textContent = 'Authenticating...'; btn.disabled = true; }

        const res = await CRMGMT.api('/api/v1/auth/login', {
          method: 'POST',
          body: JSON.stringify({ email, password })
        });

        if (btn) { btn.textContent = 'Log In'; btn.disabled = false; }

        if (res.data && res.data.success) {
          CRMGMT.state.token = res.data.token;
          CRMGMT.state.currentUser = res.data.user;

          localStorage.setItem('crmgmt_token', res.data.token);
          localStorage.setItem('crmgmt_user', JSON.stringify(res.data.user));
          if (res.data.user.avatar_url) {
            CRMGMT.setPfp(res.data.user.id, res.data.user.avatar_url, false);
          }

          CRMGMT.updateUserUI();
          CRMGMT.toast(`Welcome back, ${res.data.user.full_name}!`, 'success');
          const isCustomer = res.data.user.role === 'standard_customer' || res.data.user.role === 'enterprise_customer';
          CRMGMT.navigate(isCustomer ? 'customer_dashboard' : 'dashboard');
        } else {
          CRMGMT.toast(res.data?.error || 'Authentication failed. Please verify your credentials.', 'error');
        }
      });
    }

    // Register Form Submit
    const regForm = document.getElementById('form-register');
    if (regForm) {
      regForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const full_name = document.getElementById('reg-name').value.trim();
        const email = document.getElementById('reg-email').value.trim();
        const password = document.getElementById('reg-password').value.trim();
        const phone = document.getElementById('reg-phone').value.trim();
        const role = document.getElementById('reg-role').value;
        const avatar_url = AuthModule.selectedRegisterPfp;

        if (!email || !password || !full_name) {
          CRMGMT.toast('Please fill in all mandatory fields.', 'error');
          return;
        }

        const btn = regForm.querySelector('button[type="submit"]');
        if (btn) { btn.textContent = 'Creating Account...'; btn.disabled = true; }

        const res = await CRMGMT.api('/api/v1/auth/register', {
          method: 'POST',
          body: JSON.stringify({ full_name, email, password, phone, role, avatar_url })
        });

        if (btn) { btn.textContent = 'Register Account'; btn.disabled = false; }

        if (res.data && res.data.success) {
          CRMGMT.state.token = res.data.token;
          CRMGMT.state.currentUser = res.data.user;
          localStorage.setItem('crmgmt_token', res.data.token);
          localStorage.setItem('crmgmt_user', JSON.stringify(res.data.user));
          if (avatar_url) {
            CRMGMT.setPfp(res.data.user.id, avatar_url, false);
          }
          CRMGMT.updateUserUI();
          CRMGMT.toast(`Account registered successfully! Welcome ${full_name}.`, 'success');
          const isCustomer = role === 'standard_customer' || role === 'enterprise_customer';
          CRMGMT.navigate(isCustomer ? 'customer_dashboard' : 'dashboard');
        } else {
          CRMGMT.toast(res.data?.error || 'Registration failed.', 'error');
        }
      });
    }

    // Google Sign-in Trigger
    const googleBtn = document.getElementById('btn-google-login');
    if (googleBtn) {
      googleBtn.addEventListener('click', () => {
        CRMGMT.toast('Google Workspace Single Sign-On (Saveetha Enterprise Domain) initialized.', 'info');
      });
    }
  },

  // Toggle Login vs Register view
  switchMode(mode) {
    this.mode = mode;
    const loginSection = document.getElementById('section-login-form');
    const regSection = document.getElementById('section-register-form');

    if (mode === 'register') {
      if (loginSection) loginSection.style.display = 'none';
      if (regSection) regSection.style.display = 'block';
      this.initRegisterPfp();
    } else {
      if (loginSection) loginSection.style.display = 'block';
      if (regSection) regSection.style.display = 'none';
    }
  }
};

window.AuthModule = AuthModule;
window.switchAuthMode = (mode) => AuthModule.switchMode(mode);

window.addEventListener('DOMContentLoaded', () => {
  AuthModule.init();
});
