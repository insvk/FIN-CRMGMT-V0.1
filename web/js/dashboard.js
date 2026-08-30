/* CRMGMT v0.1 - AdminKit Pro Analytics Dashboard Controller */

const DashboardModule = {
  splineChart: null,
  donutChart: null,
  hubMap: null,
  mapMarkers: [],

  init() {
    this.initCalendar();
    this.initMap();
  },

  async loadAnalytics() {
    const res = await CRMGMT.api('/api/v1/admin/analytics');
    let data = null;

    if (res.data && res.data.success && res.data.data) {
      data = res.data.data;
    } else {
      // Offline fallback values
      data = {
        metrics: {
          sales: 2.382,
          sales_change: '-3.65%',
          earnings_formatted: '$21.300',
          earnings_change: '+6.65%',
          visitors: 14.212,
          visitors_change: '+5.25%',
          orders: 64,
          orders_change: '-2.25%'
        },
        recent_movement: [
          { month: 'Jan', movement: 2100 },
          { month: 'Feb', movement: 1600 },
          { month: 'Mar', movement: 1850 },
          { month: 'Apr', movement: 1950 },
          { month: 'May', movement: 1600 },
          { month: 'Jun', movement: 2100 },
          { month: 'Jul', movement: 2800 },
          { month: 'Aug', movement: 2700 },
          { month: 'Sep', movement: 3100 },
          { month: 'Oct', movement: 3700 },
          { month: 'Nov', movement: 3200 },
          { month: 'Dec', movement: 3600 }
        ],
        status_breakdown: {
          chrome_delivered: 4306,
          firefox_intransit: 3801,
          edge_outfordelivery: 1689,
          other_exceptions: 3251
        },
        hubs: [
          { hub_name: 'Saveetha Chennai Central Gateway', latitude: 13.0827, longitude: 80.2707, capacity: 25000, current_load: 3420 },
          { hub_name: 'Bengaluru Electronic City Hub', latitude: 12.9716, longitude: 77.5946, capacity: 20000, current_load: 2810 },
          { hub_name: 'Mumbai Western Freight Terminal', latitude: 19.0760, longitude: 72.8777, capacity: 30000, current_load: 4150 },
          { hub_name: 'Delhi NCR Logistics Center', latitude: 28.6139, longitude: 77.2090, capacity: 35000, current_load: 5290 },
          { hub_name: 'Hyderabad Express Distribution', latitude: 17.3850, longitude: 78.4867, capacity: 18000, current_load: 1940 },
          { hub_name: 'Kolkata Eastern Logistics Yard', latitude: 22.5726, 88.3639, capacity: 15000, current_load: 1420 }
        ]
      };
    }

    this.renderMetrics(data.metrics);
    this.renderSplineChart(data.recent_movement);
    this.renderDonutChart(data.status_breakdown);
    this.updateHubMap(data.hubs);
  },

  renderMetrics(m) {
    if (!m) return;
    const elSales = document.getElementById('metric-sales-val');
    const elSalesChg = document.getElementById('metric-sales-chg');
    const elEarn = document.getElementById('metric-earnings-val');
    const elEarnChg = document.getElementById('metric-earnings-chg');
    const elVisit = document.getElementById('metric-visitors-val');
    const elVisitChg = document.getElementById('metric-visitors-chg');
    const elOrders = document.getElementById('metric-orders-val');
    const elOrdersChg = document.getElementById('metric-orders-chg');

    if (elSales) elSales.textContent = Number(m.sales).toFixed(3);
    if (elSalesChg) elSalesChg.textContent = `${m.sales_change} Since last week`;
    if (elEarn) elEarn.textContent = m.earnings_formatted || `$${m.earnings}`;
    if (elEarnChg) elEarnChg.textContent = `${m.earnings_change} Since last week`;
    if (elVisit) elVisit.textContent = Number(m.visitors).toFixed(3);
    if (elVisitChg) elVisitChg.textContent = `${m.visitors_change} Since last week`;
    if (elOrders) elOrders.textContent = m.orders;
    if (elOrdersChg) elOrdersChg.textContent = `${m.orders_change} Since last week`;
  },

  renderSplineChart(movementData) {
    const canvas = document.getElementById('recent-movement-chart');
    if (!canvas || typeof Chart === 'undefined') return;

    const labels = movementData.map(d => d.month);
    const values = movementData.map(d => d.movement);

    if (this.splineChart) {
      this.splineChart.destroy();
    }

    const ctx = canvas.getContext('2d');
    const gradient = ctx.createLinearGradient(0, 0, 0, 240);
    gradient.addColorStop(0, 'rgba(59, 125, 221, 0.28)');
    gradient.addColorStop(1, 'rgba(59, 125, 221, 0.00)');

    this.splineChart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: labels,
        datasets: [{
          label: 'Parcels in Movement',
          data: values,
          borderColor: '#3b7ddd',
          backgroundColor: gradient,
          fill: true,
          tension: 0.42, // Smooth cubic spline matching screenshot
          borderWidth: 2.8,
          pointBackgroundColor: '#3b7ddd',
          pointBorderColor: '#ffffff',
          pointBorderWidth: 2,
          pointRadius: 4,
          pointHoverRadius: 6
        }]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
          legend: { display: false },
          tooltip: {
            backgroundColor: '#1e293b',
            titleFont: { size: 12, family: 'Inter' },
            bodyFont: { size: 12, family: 'Inter' },
            padding: 10,
            cornerRadius: 6
          }
        },
        scales: {
          x: {
            grid: { display: false },
            ticks: { color: '#94a3b8', font: { size: 11, family: 'Inter' } }
          },
          y: {
            min: 1000,
            max: 4000,
            ticks: {
              stepSize: 1000,
              color: '#94a3b8',
              font: { size: 11, family: 'Inter' }
            },
            grid: {
              color: '#f1f5f9'
            }
          }
        }
      }
    });
  },

  renderDonutChart(bd) {
    const canvas = document.getElementById('browser-usage-donut');
    if (!canvas || typeof Chart === 'undefined') return;

    if (this.donutChart) {
      this.donutChart.destroy();
    }

    const ctx = canvas.getContext('2d');
    this.donutChart = new Chart(ctx, {
      type: 'doughnut',
      data: {
        labels: ['Chrome / Delivered', 'Firefox / In-Transit', 'Edge / Out for Delivery', 'Other / Exceptions'],
        datasets: [{
          data: [
            bd?.chrome_delivered || 4306,
            bd?.firefox_intransit || 3801,
            bd?.edge_outfordelivery || 1689,
            bd?.other_exceptions || 3251
          ],
          backgroundColor: [
            '#3b7ddd', // Chrome blue
            '#dc3545', // Firefox / transit red
            '#fcb92c', // Edge yellow
            '#1cbb8c'  // Green
          ],
          borderWidth: 0,
          hoverOffset: 4
        }]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        cutout: '72%',
        plugins: {
          legend: { display: false }
        }
      }
    });
  },

  initCalendar() {
    const container = document.getElementById('calendar-days-grid');
    if (!container) return;

    container.innerHTML = '';
    // Days of week header
    const daysHeader = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
    daysHeader.forEach(d => {
      const el = document.createElement('div');
      el.className = 'calendar-day-header';
      el.textContent = d;
      container.appendChild(el);
    });

    // Days grid for February 2024 (Matches Reference Image 1: starts on Thursday 1st, 28th active / current)
    const prevDays = [28, 29, 30, 31]; // Jan trailing
    prevDays.forEach(d => {
      const el = document.createElement('div');
      el.className = 'calendar-day other-month';
      el.textContent = d;
      container.appendChild(el);
    });

    for (let day = 1; day <= 29; day++) {
      const el = document.createElement('div');
      el.className = 'calendar-day';
      if (day === 20 || day === 28) {
        el.classList.add('active');
      }
      el.textContent = day;
      container.appendChild(el);
    }
  },

  initMap() {
    const mapEl = document.getElementById('telemetry-map-container');
    if (!mapEl || typeof L === 'undefined') return;

    if (!this.hubMap) {
      this.hubMap = L.map('telemetry-map-container', {
        zoomControl: true,
        attributionControl: false
      }).setView([20.5937, 78.9629], 4);

      // Google Maps Tile Layers (Free, Fast, No API Key needed)
      const googleStreets = L.tileLayer('https://mt{s}.google.com/vt/lyrs=m&x={x}&y={y}&z={z}', {
        maxZoom: 20,
        subdomains: ['0', '1', '2', '3']
      });

      const googleHybrid = L.tileLayer('https://mt{s}.google.com/vt/lyrs=y&x={x}&y={y}&z={z}', {
        maxZoom: 20,
        subdomains: ['0', '1', '2', '3']
      });

      const googleTerrain = L.tileLayer('https://mt{s}.google.com/vt/lyrs=p&x={x}&y={y}&z={z}', {
        maxZoom: 20,
        subdomains: ['0', '1', '2', '3']
      });

      // Default: Google Streets
      googleStreets.addTo(this.hubMap);

      // Add Base Layer Switcher control
      const baseMaps = {
        "Google Roadmap": googleStreets,
        "Google Satellite": googleHybrid,
        "Google Terrain": googleTerrain
      };
      L.control.layers(baseMaps, null, { position: 'topright' }).addTo(this.hubMap);
    }
  },

  updateHubMap(hubs) {
    if (!this.hubMap || !hubs) return;

    // Clear existing markers
    this.mapMarkers.forEach(m => this.hubMap.removeLayer(m));
    this.mapMarkers = [];

    const hubIcon = L.divIcon({
      className: 'custom-hub-marker',
      html: `<div style="width: 14px; height: 14px; background: #3b7ddd; border: 2.5px solid #ffffff; border-radius: 50%; box-shadow: 0 0 10px rgba(59, 125, 221, 0.8);"></div>`,
      iconSize: [14, 14],
      iconAnchor: [7, 7]
    });

    hubs.forEach(hub => {
      const marker = L.marker([hub.latitude, hub.longitude], { icon: hubIcon })
        .addTo(this.hubMap)
        .bindPopup(`
          <div style="font-family: Inter, sans-serif; font-size: 12px; padding: 4px;">
            <strong style="color: #1e293b;">${hub.hub_name}</strong><br/>
            <span style="color: #64748b;">Capacity: ${hub.capacity} parcels</span><br/>
            <span style="color: #16a34a; font-weight: 600;">Status: Operational</span>
          </div>
        `);
      this.mapMarkers.push(marker);
    });

    // Draw active animated transit corridors between Saveetha Chennai Central and other hubs
    const routes = [
      [[13.0827, 80.2707], [12.9716, 77.5946]], // Chennai -> Bangalore
      [[13.0827, 80.2707], [17.3850, 78.4867]], // Chennai -> Hyderabad
      [[13.0827, 80.2707], [19.0760, 72.8777]], // Chennai -> Mumbai
      [[19.0760, 72.8777], [28.6139, 77.2090]]  // Mumbai -> Delhi
    ];

    routes.forEach(coords => {
      const line = L.polyline(coords, {
        color: '#3b7ddd',
        weight: 2.5,
        opacity: 0.7,
        dashArray: '6, 8'
      }).addTo(this.hubMap);
      this.mapMarkers.push(line);
    });

    setTimeout(() => {
      this.hubMap.invalidateSize();
    }, 200);
  }
};

window.DashboardModule = DashboardModule;
window.addEventListener('DOMContentLoaded', () => {
  DashboardModule.init();
});
