/* CRMGMT v0.1 - Live Tracking, GPS Simulator & Proof of Delivery (POD) Controller */

const TrackingModule = {
  trackingMap: null,
  routePolyline: null,
  truckMarker: null,
  sigCanvas: null,
  sigCtx: null,
  isDrawing: false,
  currentShipment: null,

  init() {
    this.bindEvents();
    this.initSignaturePad();
  },

  bindEvents() {
    const trackBtn = document.getElementById('btn-track-lookup');
    const trackInput = document.getElementById('input-track-id');

    if (trackBtn && trackInput) {
      trackBtn.addEventListener('click', () => {
        const tid = trackInput.value.trim();
        if (tid) this.lookup(tid);
        else CRMGMT.toast('Please enter a Tracking Number.', 'warning');
      });

      trackInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
          const tid = trackInput.value.trim();
          if (tid) this.lookup(tid);
        }
      });
    }

    // Modal close
    const closeModalBtn = document.getElementById('btn-close-pod-modal');
    if (closeModalBtn) {
      closeModalBtn.addEventListener('click', () => this.closePodModal());
    }

    const clearSigBtn = document.getElementById('btn-clear-signature');
    if (clearSigBtn) {
      clearSigBtn.addEventListener('click', () => this.clearSignature());
    }

    const submitPodBtn = document.getElementById('btn-submit-pod');
    if (submitPodBtn) {
      submitPodBtn.addEventListener('click', () => this.submitPOD());
    }
  },

  async lookup(trackingId) {
    const res = await CRMGMT.api(`/api/v1/tracking/${encodeURIComponent(trackingId)}`);
    const container = document.getElementById('tracking-result-box');

    if (res.data && res.data.success && res.data.shipment) {
      this.currentShipment = res.data.shipment;
      this.renderTrackingDetails(res.data);
      if (container) container.style.display = 'block';
    } else {
      CRMGMT.toast(res.data?.error || 'Tracking number not found or invalid format.', 'error');
      if (container) container.style.display = 'none';
    }
  },

  renderTrackingDetails(data) {
    const s = data.shipment;
    const checkpoints = data.checkpoints || [];
    const route = data.route || {};

    // Header Meta
    document.getElementById('track-disp-id').textContent = s.tracking_id;
    document.getElementById('track-disp-status').textContent = s.status.replace(/_/g, ' ');
    document.getElementById('track-disp-sender').textContent = s.sender_name;
    document.getElementById('track-disp-recip').textContent = s.recipient_name;
    document.getElementById('track-disp-dest').textContent = s.recipient_address;
    document.getElementById('track-disp-weight').textContent = `${s.weight_kg} kg`;
    document.getElementById('track-disp-eta').textContent = s.estimated_delivery ? new Date(s.estimated_delivery).toLocaleString() : 'Within 24 Hours';

    // Update Progress Stepper
    this.updateStepper(s.status);

    // Render Checkpoint Timeline
    const cpContainer = document.getElementById('checkpoint-timeline-list');
    if (cpContainer) {
      cpContainer.innerHTML = '';
      if (checkpoints.length === 0) {
        cpContainer.innerHTML = '<div style="color: #94a3b8; padding: 12px;">No checkpoint updates recorded yet.</div>';
      } else {
        checkpoints.slice().reverse().forEach((cp, idx) => {
          const item = document.createElement('div');
          item.className = 'timeline-checkpoint-item';
          item.style.cssText = 'display: flex; gap: 14px; margin-bottom: 16px; position: relative;';
          item.innerHTML = `
            <div style="display: flex; flex-direction: column; align-items: center;">
              <div style="width: 12px; height: 12px; border-radius: 50%; background: ${idx === 0 ? '#3b7ddd' : '#94a3b8'}; border: 2px solid #fff; box-shadow: 0 0 0 2px ${idx === 0 ? '#3b7ddd' : '#cbd5e1'};"></div>
              ${idx < checkpoints.length - 1 ? '<div style="width: 2px; flex: 1; background: #e2e8f0; margin-top: 4px;"></div>' : ''}
            </div>
            <div style="flex: 1;">
              <div style="font-weight: 700; color: #1e293b; font-size: 0.88rem;">${cp.status.replace(/_/g, ' ')} - ${cp.location_tag}</div>
              <div style="font-size: 0.78rem; color: #64748b; margin-top: 2px;">${cp.remarks || 'Scanned in transit'}</div>
              <div style="font-size: 0.72rem; color: #94a3b8; margin-top: 4px;">${new Date(cp.timestamp).toLocaleString()}</div>
            </div>
          `;
          cpContainer.appendChild(item);
        });
      }
    }

    // Render POD Button or Signature if available
    const podActionBox = document.getElementById('pod-action-container');
    if (podActionBox) {
      if (s.pod_signature_url) {
        podActionBox.innerHTML = `
          <div style="background: #e0f9f1; border: 1px solid #a7f3d0; border-radius: 6px; padding: 12px; display: flex; align-items: center; justify-content: space-between;">
            <div>
              <div style="font-weight: 700; color: #065f46;">Proof of Delivery Verified</div>
              <div style="font-size: 0.76rem; color: #047857;">Signed upon delivery</div>
            </div>
            <img src="${s.pod_signature_url}" alt="POD Signature" style="max-height: 40px; border: 1px solid #cbd5e1; background: #fff; border-radius: 4px; padding: 2px;" />
          </div>
        `;
      } else if (s.status === 'OUT_FOR_DELIVERY' || CRMGMT.state.currentUser?.role === 'delivery_agent' || CRMGMT.state.currentUser?.role === 'super_admin') {
        podActionBox.innerHTML = `
          <button class="btn btn-primary btn-sm" onclick="TrackingModule.openPodModal('${s.id}')">
            Capture Proof of Delivery (POD Signature)
          </button>
        `;
      } else {
        podActionBox.innerHTML = '';
      }
    }

    // Update GPS Route Map
    this.renderTrackingMap(route, checkpoints, s.status);
  },

  updateStepper(status) {
    const steps = ['ORDER_CREATED', 'PICKED_UP', 'IN_TRANSIT', 'OUT_FOR_DELIVERY', 'DELIVERED'];
    const statusIdx = steps.indexOf(status);

    const progressBar = document.getElementById('stepper-progress');
    if (progressBar) {
      const pct = statusIdx >= 0 ? (statusIdx / (steps.length - 1)) * 100 : 0;
      progressBar.style.width = `${pct}%`;
    }

    steps.forEach((st, idx) => {
      const node = document.getElementById(`step-node-${st}`);
      if (!node) return;
      node.classList.remove('completed', 'active');
      if (idx < statusIdx) {
        node.classList.add('completed');
      } else if (idx === statusIdx) {
        node.classList.add('active');
      }
    });
  },

  renderTrackingMap(route, checkpoints, status) {
    const mapEl = document.getElementById('tracking-live-map');
    if (!mapEl || typeof L === 'undefined') return;

    if (!this.trackingMap) {
      this.trackingMap = L.map('tracking-live-map', { attributionControl: false }).setView([13.0827, 80.2707], 6);

      const googleStreets = L.tileLayer('https://mt{s}.google.com/vt/lyrs=m&x={x}&y={y}&z={z}', {
        maxZoom: 20,
        subdomains: ['0', '1', '2', '3']
      });

      const googleHybrid = L.tileLayer('https://mt{s}.google.com/vt/lyrs=y&x={x}&y={y}&z={z}', {
        maxZoom: 20,
        subdomains: ['0', '1', '2', '3']
      });

      googleStreets.addTo(this.trackingMap);

      const baseMaps = {
        "Google Roadmap": googleStreets,
        "Google Satellite": googleHybrid
      };
      L.control.layers(baseMaps, null, { position: 'topright' }).addTo(this.trackingMap);
    }

    // Clean previous layers
    if (this.routePolyline) this.trackingMap.removeLayer(this.routePolyline);
    if (this.truckMarker) this.trackingMap.removeLayer(this.truckMarker);

    const origCoords = route.origin ? [route.origin.lat, route.origin.lng] : [13.0827, 80.2707];
    const destCoords = route.destination ? [route.destination.lat, route.destination.lng] : [12.9716, 77.5946];

    // Hub Markers
    L.marker(origCoords).addTo(this.trackingMap).bindPopup(`<b>Origin:</b> ${route.origin?.name || 'Saveetha Hub'}`);
    L.marker(destCoords).addTo(this.trackingMap).bindPopup(`<b>Destination:</b> ${route.destination?.name || 'Destination Hub'}`);

    // Route line
    this.routePolyline = L.polyline([origCoords, destCoords], {
      color: '#3b7ddd',
      weight: 3.5,
      dashArray: '8, 8',
      opacity: 0.8
    }).addTo(this.trackingMap);

    // Calculate vehicle position
    let vehiclePos = origCoords;
    if (status === 'IN_TRANSIT') {
      vehiclePos = [(origCoords[0] + destCoords[0]) / 2, (origCoords[1] + destCoords[1]) / 2];
    } else if (status === 'OUT_FOR_DELIVERY' || status === 'DELIVERED') {
      vehiclePos = destCoords;
    }

    const truckIcon = L.divIcon({
      className: 'truck-pulse-icon',
      html: `<div style="width: 32px; height: 32px; background: #5046e4; color: #fff; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 16px; box-shadow: 0 0 15px rgba(80,70,228,0.7); border: 2px solid #fff;">🚚</div>`,
      iconSize: [32, 32],
      iconAnchor: [16, 16]
    });

    this.truckMarker = L.marker(vehiclePos, { icon: truckIcon })
      .addTo(this.trackingMap)
      .bindPopup(`<b>Live Courier Position</b><br/>Status: ${status}`);

    const bounds = L.latLngBounds([origCoords, destCoords, vehiclePos]);
    this.trackingMap.fitBounds(bounds, { padding: [40, 40] });

    setTimeout(() => this.trackingMap.invalidateSize(), 200);
  },

  // POD Signature Pad
  initSignaturePad() {
    this.sigCanvas = document.getElementById('signature-canvas');
    if (!this.sigCanvas) return;

    this.sigCtx = this.sigCanvas.getContext('2d');
    this.sigCtx.strokeStyle = '#1e293b';
    this.sigCtx.lineWidth = 2.5;
    this.sigCtx.lineCap = 'round';

    const getPos = (e) => {
      const rect = this.sigCanvas.getBoundingClientRect();
      const clientX = e.touches ? e.touches[0].clientX : e.clientX;
      const clientY = e.touches ? e.touches[0].clientY : e.clientY;
      return {
        x: clientX - rect.left,
        y: clientY - rect.top
      };
    };

    const startDraw = (e) => {
      e.preventDefault();
      this.isDrawing = true;
      const pos = getPos(e);
      this.sigCtx.beginPath();
      this.sigCtx.moveTo(pos.x, pos.y);
    };

    const draw = (e) => {
      if (!this.isDrawing) return;
      e.preventDefault();
      const pos = getPos(e);
      this.sigCtx.lineTo(pos.x, pos.y);
      this.sigCtx.stroke();
    };

    const stopDraw = () => {
      this.isDrawing = false;
    };

    this.sigCanvas.addEventListener('mousedown', startDraw);
    this.sigCanvas.addEventListener('mousemove', draw);
    window.addEventListener('mouseup', stopDraw);

    this.sigCanvas.addEventListener('touchstart', startDraw);
    this.sigCanvas.addEventListener('touchmove', draw);
    window.addEventListener('touchend', stopDraw);
  },

  clearSignature() {
    if (!this.sigCanvas || !this.sigCtx) return;
    this.sigCtx.clearRect(0, 0, this.sigCanvas.width, this.sigCanvas.height);
  },

  openPodModal(shipmentId) {
    const modal = document.getElementById('pod-signature-modal');
    if (modal) {
      modal.classList.add('open');
      this.clearSignature();
    }
  },

  closePodModal() {
    const modal = document.getElementById('pod-signature-modal');
    if (modal) modal.classList.remove('open');
  },

  async submitPOD() {
    if (!this.currentShipment || !this.sigCanvas) return;

    const dataUrl = this.sigCanvas.toDataURL('image/png');
    const res = await CRMGMT.api(`/api/v1/shipments/${this.currentShipment.id}/pod`, {
      method: 'POST',
      body: JSON.stringify({ signature_data: dataUrl })
    });

    if (res.data && res.data.success) {
      CRMGMT.toast('Proof of Delivery signed and verified!', 'success');
      this.closePodModal();
      this.lookup(this.currentShipment.tracking_id);
    } else {
      CRMGMT.toast(res.data?.error || 'Failed to submit POD.', 'error');
    }
  }
};

window.TrackingModule = TrackingModule;
window.addEventListener('DOMContentLoaded', () => {
  TrackingModule.init();
});
