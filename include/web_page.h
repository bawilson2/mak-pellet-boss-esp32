#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Configuration Page (Captive Portal & Settings)
// -----------------------------------------------------------------------------
const char SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>MAK Grill - Configuration</title>
  <style>
    :root {
      --bg: #0f1115;
      --card: #181b21;
      --accent: #ff6d00;
      --blue: #2979ff;
      --text: #f0f2f5;
      --subtext: #8a93a0;
      --border: #2b313c;
    }
    * { box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background: var(--bg);
      color: var(--text);
      margin: 0;
      padding: 16px 12px;
      display: flex;
      justify-content: center;
    }
    .container { width: 100%; max-width: 440px; }
    .top-bar {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 14px;
      padding: 0 4px;
    }
    .back-link {
      color: var(--subtext);
      text-decoration: none;
      font-size: 0.85rem;
      font-weight: 700;
    }
    .back-link:active { color: var(--text); }
    .card {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 20px 18px;
      margin-bottom: 16px;
      box-shadow: 0 4px 16px rgba(0,0,0,0.4);
    }
    h2 { font-size: 1.15rem; margin: 0 0 6px 0; }
    .subtext { color: var(--subtext); font-size: 0.82rem; margin-bottom: 16px; line-height: 1.4; }
    .label-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 6px;
    }
    label { font-size: 0.75rem; font-weight: 700; color: var(--subtext); letter-spacing: 0.5px; }
    .rescan-btn {
      background: transparent;
      border: none;
      color: var(--blue);
      font-size: 0.75rem;
      font-weight: 700;
      cursor: pointer;
      padding: 0;
    }
    select, input[type="password"], input[type="text"] {
      width: 100%;
      background: #21252d;
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 12px;
      color: var(--text);
      font-size: 0.95rem;
      margin-bottom: 14px;
      outline: none;
    }
    select:focus, input:focus { border-color: var(--accent); }
    button[type="submit"], .btn-action, .btn-orange, .btn-blue{
      width: 100%;
      color: white;
      border: none;
      padding: 13px;
      border-radius: 8px;
      font-size: 0.95rem;
      font-weight: 700;
      cursor: pointer;
      text-align: center;
      text-decoration: none;
      display: block;
    }
    button:active, .btn-action:active { opacity: 0.85; }
    .btn-orange { background: var(--accent); }
    .btn-blue { background: var(--blue); }
    .toggle-link {
      color: var(--accent);
      font-size: 0.8rem;
      text-decoration: underline;
      cursor: pointer;
      display: inline-block;
      margin-bottom: 14px;
    }
    .ip-box {
      margin-top: 10px;
      padding: 12px;
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 12px;
      font-size: 0.8rem;
      text-align: center;
      color: var(--subtext);
    }

    /* Custom Dialog */
    .dialog-backdrop {
      position: fixed; top: 0; left: 0; right: 0; bottom: 0;
      background: rgba(0, 0, 0, 0.75); display: none; align-items: center; justify-content: center; z-index: 1000;
    }
    .dialog-backdrop.open { display: flex; }
    .dialog-box {
      background: var(--card); border: 1px solid var(--border); border-radius: 12px;
      width: 90%; max-width: 320px; padding: 22px; text-align: center; box-shadow: 0 8px 24px rgba(0,0,0,0.6);
    }
    .dialog-msg { margin-bottom: 20px; font-size: 0.95rem; line-height: 1.4; color: var(--text); }
    .dialog-btns { display: flex; gap: 8px; justify-content: center; }
    .btn-dialog { flex: 1; padding: 12px; border-radius: 8px; border: none; font-weight: 700; cursor: pointer; font-size: 0.9rem; }
    .btn-cancel { background: var(--card-inner); color: var(--subtext); border: 1px solid var(--border); }
    .btn-confirm { background: var(--accent); color: white; }
  </style>
</head>
<body>
  <div class="container">
    <div class="top-bar">
      <a href="/" class="back-link">← DASHBOARD</a>
      <span class="subtext" style="margin:0;">CONFIG</span>
    </div>

    <!-- Form 1: Push Notifications -->
    <div class="card">
      <h2 style="color: var(--accent);">Push Notifications</h2>
      <div class="subtext">Update your ntfy.sh alert channel. Applies immediately without rebooting.</div>
      <form onsubmit="saveNtfy(event)">
        <div class="label-row">
          <label for="ntfy">NTFY.SH ALERT TOPIC</label>
        </div>
        <input type="text" id="ntfy" name="ntfy" placeholder="mak-grill-alerts-test" required>
        
        <div style="display: flex; gap: 8px;">
          <button type="submit" class="btn-orange" style="flex: 2;">Save Topic</button>
          <button type="button" class="btn-blue" style="flex: 1;" onclick="testNtfy()">Send Test</button>
        </div>
      </form>
    </div>

    <!-- Form 2: Wi-Fi Setup -->
    <div class="card">
      <h2 style="color: var(--blue);">Wi-Fi Connection</h2>
      <div class="subtext">Connect your controller to a local 2.4 GHz home network.</div>

      <form onsubmit="submitWifi(event)">
        <div id="dropdownGroup">
          <div class="label-row">
            <label for="ssidSelect">SELECT 2.4 GHz NETWORK</label>
            <button type="button" class="rescan-btn" onclick="loadNetworks(true)">↻ Rescan</button>
          </div>
          <select id="ssidSelect" name="ssid_select" onchange="handleSelectChange(this)">
            <option value="">Scanning nearby networks...</option>
          </select>
          <span class="toggle-link" onclick="showManualEntry()">Or enter hidden SSID manually</span>
        </div>

        <div id="manualGroup" style="display:none;">
          <div class="label-row">
            <label for="ssidManual">NETWORK NAME (SSID)</label>
          </div>
          <input type="text" id="ssidManual" name="ssid_manual" placeholder="Home Network SSID">
          <span class="toggle-link" onclick="showDropdownEntry()">Back to network list</span>
        </div>

        <input type="hidden" id="finalSSID" name="ssid">

        <div class="label-row">
          <label for="pass">WI-FI PASSWORD</label>
        </div>
        <input type="password" id="pass" name="pass" placeholder="Network Password">

        <button type="submit" class="btn-blue">Save Wi-Fi & Connect</button>
      </form>
    </div>

    <div style="margin-bottom: 16px;">
      <a href="/" class="btn-action" style="background:#21252d; border:1px solid var(--border);">Continue to Dashboard (Standalone AP)</a>
    </div>

    <div class="ip-box">
      Access Point IP: <strong>192.168.4.1</strong>
    </div>
    <!-- Custom Dialog Modal -->
    <div id="dialogBackdrop" class="dialog-backdrop">
      <div class="dialog-box">
        <div id="dialogMsg" class="dialog-msg"></div>
        <div class="dialog-btns">
          <button id="dialogCancel" class="btn-dialog btn-cancel">Cancel</button>
          <button id="dialogConfirm" class="btn-dialog btn-confirm">OK</button>
        </div>
      </div>
    </div>
  </div>

  <script>
    let isManual = false;
    let scanRetries = 0;

    function showManualEntry() {
      isManual = true;
      document.getElementById('dropdownGroup').style.display = 'none';
      document.getElementById('manualGroup').style.display = 'block';
      document.getElementById('ssidManual').focus();
    }

    function showDropdownEntry() {
      isManual = false;
      document.getElementById('manualGroup').style.display = 'none';
      document.getElementById('dropdownGroup').style.display = 'block';
    }

    function handleSelectChange(elem) {
      if (elem.value === '__manual__') {
        showManualEntry();
      }
    }

    async function submitWifi(e) {
      e.preventDefault();
      const finalSSID = document.getElementById('finalSSID');
      
      if (isManual) {
        finalSSID.value = document.getElementById('ssidManual').value.trim();
      } else {
        finalSSID.value = document.getElementById('ssidSelect').value;
      }

      if (!finalSSID.value || finalSSID.value === '__manual__') {
        await showDialog('Please select or enter a valid Wi-Fi SSID.');
        return;
      }

      const btn = e.target.querySelector('button[type="submit"]');
      const originalText = btn.textContent;
      btn.textContent = 'Saving...';
      btn.style.opacity = '0.7';

      const formData = new URLSearchParams();
      formData.append('ssid', finalSSID.value);
      formData.append('pass', document.getElementById('pass').value);

      try {
        await fetch('/api/wifi/save', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: formData.toString()
        });
        await showDialog('Wi-Fi credentials saved. The controller is rebooting. Please connect your device back to your home network.');
      } catch (err) {
        await showDialog('Network error while saving Wi-Fi credentials.');
      }

      btn.textContent = originalText;
      btn.style.opacity = '1';
    }

    async function saveNtfy(e) {
      e.preventDefault();
      const btn = e.target.querySelector('button[type="submit"]');
      const originalText = btn.textContent;
      btn.textContent = 'Saving...';
      btn.style.opacity = '0.7';

      const formData = new URLSearchParams();
      formData.append('ntfy', document.getElementById('ntfy').value);

      try {
        const res = await fetch('/api/config/ntfy', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: formData.toString()
        });
        if (res.ok) {
          await showDialog('Topic saved successfully!');
        } else {
          await showDialog('Failed to save topic.');
        }
      } catch (err) {
        await showDialog('Network error while saving topic.');
      }

      btn.textContent = originalText;
      btn.style.opacity = '1';
    }

    async function testNtfy() {
      const btn = document.querySelector('button[onclick="testNtfy()"]');
      const originalText = btn.textContent;
      btn.textContent = 'Sending...';
      btn.style.opacity = '0.7';
      btn.style.pointerEvents = 'none';

      try {
        const res = await fetch('/api/config/ntfy/test', { method: 'POST' });
        if (res.ok) {
          await showDialog('Test notification sent! Check your NTFY app.');
        } else {
          await showDialog('Failed to send test. Ensure you have saved a topic first.');
        }
      } catch (err) {
        await showDialog('Network error while attempting to send test.');
      }

      btn.textContent = originalText;
      btn.style.opacity = '1';
      btn.style.pointerEvents = 'auto';
    }

    async function loadCurrentConfig() {
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        if (data.ntfy) {
          document.getElementById('ntfy').value = data.ntfy;
        }
      } catch (err) {}
    }

    async function loadNetworks(forceRescan = false) {
      const sel = document.getElementById('ssidSelect');
      if (!sel) return;

      if (forceRescan) {
        scanRetries = 0;
        sel.innerHTML = '<option value="">Starting new scan...</option>';
      }

      try {
        const url = forceRescan ? '/api/wifi/scan?rescan=1' : '/api/wifi/scan';
        const res = await fetch(url);
        const data = await res.json();

        if (data && data.scanning === true) {
          scanRetries++;
          if (scanRetries < 25) {
            const dots = '.'.repeat((scanRetries % 3) + 1);
            sel.innerHTML = `<option value="">Scanning nearby networks${dots}</option>`;
            setTimeout(() => loadNetworks(false), 1500);
            return;
          }
        }

        sel.innerHTML = '';
        const networks = (data && Array.isArray(data.networks)) ? data.networks : [];

        if (networks.length === 0) {
          sel.innerHTML = '<option value="">No 2.4GHz networks found</option>';
          const manualOpt = document.createElement('option');
          manualOpt.value = '__manual__';
          manualOpt.textContent = 'Enter SSID manually...';
          sel.appendChild(manualOpt);
          return;
        }

        const seen = new Set();
        networks.forEach(net => {
          if (net.ssid && !seen.has(net.ssid)) {
            seen.add(net.ssid);
            const opt = document.createElement('option');
            opt.value = net.ssid;
            opt.textContent = `${net.ssid} (${net.rssi} dBm)`;
            sel.appendChild(opt);
          }
        });

        const manualOpt = document.createElement('option');
        manualOpt.value = '__manual__';
        manualOpt.textContent = 'Enter hidden SSID manually...';
        sel.appendChild(manualOpt);

      } catch (err) {
        scanRetries++;
        if (scanRetries < 25) {
          setTimeout(() => loadNetworks(false), 1500);
        } else {
          sel.innerHTML = '<option value="">Scan timed out</option>';
          showManualEntry();
        }
      }
    }

    window.addEventListener('DOMContentLoaded', () => {
      loadCurrentConfig();
      loadNetworks(false);
    });

    function showDialog(message, isConfirm = false) {
      return new Promise(resolve => {
        document.getElementById('dialogMsg').textContent = message;
        const cancelBtn = document.getElementById('dialogCancel');
        const confirmBtn = document.getElementById('dialogConfirm');
        const backdrop = document.getElementById('dialogBackdrop');

        cancelBtn.style.display = isConfirm ? 'block' : 'none';

        const cleanup = () => {
          backdrop.classList.remove('open');
          confirmBtn.onclick = null;
          cancelBtn.onclick = null;
        };

        confirmBtn.onclick = () => { cleanup(); resolve(true); };
        cancelBtn.onclick = () => { cleanup(); resolve(false); };

        backdrop.classList.add('open');
      });
    }
  </script>
</body>
</html>
)rawliteral";

// -----------------------------------------------------------------------------
// Main Dashboard
// -----------------------------------------------------------------------------
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>MAK Grill Controller</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js" defer></script>
  <style>
    :root {
      --bg: #0f1115;
      --card: #181b21;
      --card-inner: #21252d;
      --accent: #ff6d00;
      --accent-dim: rgba(255, 109, 0, 0.15);
      --text: #f0f2f5;
      --subtext: #8a93a0;
      --border: #2b313c;
      --green: #00c853;
      --red: #d50000;
      --p1-color: #2979ff;
      --p2-color: #00e676;
      --p3-color: #d500f9;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
      background-color: var(--bg);
      color: var(--text);
      margin: 0;
      padding: 12px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    .container { width: 100%; max-width: 520px; }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 12px;
      padding: 0 4px;
    }
    .title-group h1 { font-size: 1.15rem; margin: 0; font-weight: 700; letter-spacing: 0.5px; }
    .meta-line { display: flex; align-items: center; gap: 8px; margin-top: 2px; }
    .meta-label { font-size: 0.72rem; color: var(--subtext); font-family: monospace; }
    .header-actions { display: flex; align-items: center; gap: 8px; }
    .btn-icon {
      background: var(--card-inner);
      border: 1px solid var(--border);
      color: var(--subtext);
      border-radius: 50%;
      width: 28px;
      height: 28px;
      display: flex;
      align-items: center;
      justify-content: center;
      text-decoration: none;
      font-size: 0.9rem;
      cursor: pointer;
    }
    .badge {
      padding: 4px 10px;
      border-radius: 20px;
      font-size: 0.75rem;
      font-weight: 700;
      background: var(--red);
      color: white;
      text-transform: uppercase;
    }
    .badge.online { background: var(--green); }
    .card {
      background: var(--card);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 16px;
      margin-bottom: 12px;
    }
    .main-temp { text-align: center; padding: 24px 16px; }
    .temp-val { font-size: 4.25rem; font-weight: 800; line-height: 1; color: var(--accent); }
    .unit { font-size: 1.5rem; color: var(--subtext); font-weight: 400; margin-left: 2px; }
    .subtext { color: var(--subtext); font-size: 0.8rem; text-transform: uppercase; letter-spacing: 0.5px; font-weight: 600; }
    
    .target-pill {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      margin-top: 14px;
      background: var(--card-inner);
      border: 1px solid var(--border);
      padding: 10px 20px;
      border-radius: 24px;
      font-size: 1.05rem;
      font-weight: 700;
      color: var(--text);
      cursor: pointer;
      transition: background 0.15s, border-color 0.15s;
    }
    .target-pill:active {
      background: var(--border);
      border-color: var(--accent);
    }
    .target-pill strong {
      color: var(--accent);
      margin: 0 4px 0 6px;
    }

    .btn-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; margin-top: 14px; }
    button {
      background: var(--card-inner);
      color: var(--text);
      border: 1px solid var(--border);
      padding: 12px 4px;
      border-radius: 8px;
      font-size: 0.95rem;
      font-weight: 700;
      cursor: pointer;
      display: flex;
      justify-content: center;
      align-items: center;
      transition: background 0.1s;
    }
    button:active { background: var(--border); }
    
    .probes-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin-top: 8px; }
    .probe-box {
      background: var(--card-inner);
      border: 1px solid var(--border);
      border-radius: 10px;
      padding: 12px 6px;
      text-align: center;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      min-height: 120px;
    }
    .probe-temp { font-size: 1.5rem; font-weight: 800; margin: 4px 0 8px 0; }
    
    .alarm-pill {
      width: 100%;
      min-height: 38px;
      padding: 6px 2px;
      font-size: 0.8rem;
      font-weight: 700;
      border-radius: 6px;
      background: var(--bg);
      border: 1px solid var(--border);
      color: var(--subtext);
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .alarm-pill.active {
      color: var(--accent);
      border-color: var(--accent);
      background: var(--accent-dim);
    }
    .alarm-pill.ready {
      color: white;
      background: var(--red);
      border-color: var(--red);
      animation: pulse 1s infinite alternate;
    }

    .chart-container { position: relative; height: 220px; width: 100%; margin-top: 8px; }
    canvas { width: 100%; height: 100%; }

    .chart-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .btn-reset {
      background: transparent;
      border: 1px solid var(--border);
      color: var(--subtext);
      font-size: 0.72rem;
      font-weight: 700;
      padding: 4px 10px;
      border-radius: 6px;
      cursor: pointer;
    }
    .btn-reset:active { color: var(--accent); border-color: var(--accent); }

    /* Bottom Sheet Modals */
    .modal-backdrop {
      position: fixed;
      top: 0; left: 0; right: 0; bottom: 0;
      background: rgba(0, 0, 0, 0.75);
      display: none;
      align-items: flex-end;
      justify-content: center;
      z-index: 999;
    }
    .modal-backdrop.open { display: flex; }
    .modal-sheet {
      background: var(--card);
      border-top: 2px solid var(--border);
      border-radius: 16px 16px 0 0;
      width: 100%;
      max-width: 520px;
      padding: 20px 16px 28px 16px;
      box-shadow: 0 -4px 16px rgba(0, 0, 0, 0.5);
    }
    .modal-head {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 12px;
    }
    .modal-title { font-size: 1.1rem; font-weight: 700; margin: 0; }
    .modal-display {
      text-align: center;
      padding: 10px 0;
      background: var(--bg);
      border-radius: 8px;
      border: 1px solid var(--border);
      margin-bottom: 14px;
    }
    
    .modal-temp-input {
      width: 100%;
      background: transparent;
      border: none;
      outline: none;
      font-size: 3rem;
      font-weight: 800;
      color: var(--accent);
      text-align: center;
      font-family: inherit;
      padding: 0;
      margin: 0;
      line-height: 1.1;
    }
    .modal-temp-input::placeholder {
      color: var(--subtext);
      font-weight: 700;
      font-size: 2.25rem;
    }
    .modal-temp-input::-webkit-outer-spin-button,
    .modal-temp-input::-webkit-inner-spin-button {
      -webkit-appearance: none;
      margin: 0;
    }
    .modal-temp-input[type=number] {
      -moz-appearance: textfield;
    }

    .modal-actions {
      display: flex;
      gap: 8px;
      margin-top: 16px;
    }
    .btn-clear {
      flex: 1;
      background: var(--card-inner);
      color: var(--subtext);
      border: 1px solid var(--border);
      padding: 14px;
      font-size: 1rem;
      font-weight: 700;
      border-radius: 8px;
    }
    .btn-done {
      flex: 2;
      background: var(--accent);
      color: white;
      border: none;
      padding: 14px;
      font-size: 1rem;
      font-weight: 700;
      border-radius: 8px;
    }

    @keyframes pulse {
      from { opacity: 0.7; }
      to { opacity: 1; }
    }
    
    /* Custom Dialog */
    .dialog-backdrop {
      position: fixed; top: 0; left: 0; right: 0; bottom: 0;
      background: rgba(0, 0, 0, 0.75); display: none; align-items: center; justify-content: center; z-index: 1000;
    }
    .dialog-backdrop.open { display: flex; }
    .dialog-box {
      background: var(--card); border: 1px solid var(--border); border-radius: 12px;
      width: 90%; max-width: 320px; padding: 22px; text-align: center; box-shadow: 0 8px 24px rgba(0,0,0,0.6);
    }
    .dialog-msg { margin-bottom: 20px; font-size: 0.95rem; line-height: 1.4; color: var(--text); }
    .dialog-btns { display: flex; gap: 8px; justify-content: center; }
    .btn-dialog { flex: 1; padding: 12px; border-radius: 8px; border: none; font-weight: 700; cursor: pointer; font-size: 0.9rem; }
    .btn-cancel { background: var(--card-inner); color: var(--subtext); border: 1px solid var(--border); }
    .btn-confirm { background: var(--accent); color: white; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="title-group">
        <h1>MAK Pellet Boss</h1>
        <div class="meta-line">
          <span id="ipLabel" class="meta-label">IP: --</span>
          <span id="timerLabel" class="meta-label">| COOK: --</span>
        </div>
      </div>
      <div class="header-actions">
        <a href="/setup" class="btn-icon" title="Settings">⚙</a>
        <span id="statusBadge" class="badge">OFFLINE</span>
      </div>
    </header>

    <!-- Pit Temp Card -->
    <div class="card main-temp">
      <div class="subtext">PIT TEMPERATURE</div>
      <div style="margin: 6px 0;"><span id="pitTemp" class="temp-val">--</span><span class="unit">°F</span></div>
      
      <div>
        <div style="display: flex; justify-content: center; gap: 12px; margin-top: 14px;">
          <div class="target-pill" style="margin-top: 0;" onclick="openSetpointSheet()">
            Target: <strong id="setpointActual">--</strong>°F
          </div>
          <div id="powerBtn" class="target-pill" style="margin-top: 0; background: var(--card-inner); color: var(--subtext);" onclick="togglePower()">
            --
          </div>
        </div>
      </div>
    </div>

    <!-- Food Probes 1, 2, 3 -->
    <div class="card">
      <div class="subtext">FOOD PROBES</div>
      <div class="probes-grid">
        <!-- Probe 1 -->
        <div class="probe-box" style="border-color: rgba(41, 121, 255, 0.35);">
          <div class="subtext" style="color: var(--p1-color);">Probe 1</div>
          <div id="p1" class="probe-temp">--</div>
          <button id="p1Pill" class="alarm-pill" onclick="openAlarmSheet(1)">Set Alarm</button>
        </div>

        <!-- Probe 2 -->
        <div class="probe-box" style="border-color: rgba(0, 230, 118, 0.35);">
          <div class="subtext" style="color: var(--p2-color);">Probe 2</div>
          <div id="p2" class="probe-temp">--</div>
          <button id="p2Pill" class="alarm-pill" onclick="openAlarmSheet(2)">Set Alarm</button>
        </div>

        <!-- Probe 3 -->
        <div class="probe-box" style="border-color: rgba(213, 0, 249, 0.35);">
          <div class="subtext" style="color: var(--p3-color);">Probe 3</div>
          <div id="p3" class="probe-temp">--</div>
          <button id="p3Pill" class="alarm-pill" onclick="openAlarmSheet(3)">Set Alarm</button>
        </div>
      </div>
    </div>

    <!-- Cook History Chart Card -->
    <div class="card">
      <div class="chart-header">
        <div class="subtext">COOK HISTORY</div>
        <button class="btn-reset" onclick="clearHistory()">RESET GRAPH</button>
      </div>
      <div class="chart-container">
        <canvas id="cookChart"></canvas>
      </div>
    </div>
    <!-- Custom Dialog Modal -->
    <div id="dialogBackdrop" class="dialog-backdrop">
      <div class="dialog-box">
        <div id="dialogMsg" class="dialog-msg"></div>
        <div class="dialog-btns">
          <button id="dialogCancel" class="btn-dialog btn-cancel">Cancel</button>
          <button id="dialogConfirm" class="btn-dialog btn-confirm">OK</button>
        </div>
      </div>
    </div>
  </div>

  <!-- Mobile Pit Setpoint Modal -->
  <div id="setpointBackdrop" class="modal-backdrop" onclick="closeOnBackdrop(event, 'setpointBackdrop')">
    <div class="modal-sheet">
      <div class="modal-head">
        <div class="modal-title">Set Pit Temperature</div>
        <div class="subtext" style="cursor:pointer;" onclick="closeSetpointSheet()">✕ CLOSE</div>
      </div>

      <div class="modal-display">
        <div class="subtext" style="margin-bottom:2px;">TARGET TEMPERATURE (°F)</div>
        <input 
          type="number" 
          inputmode="numeric" 
          pattern="[0-9]*" 
          id="modalSetpointInput" 
          class="modal-temp-input" 
          placeholder="150" 
          min="150" 
          max="500"
          onchange="commitSetpointModalInput()"
        >
      </div>

      <div class="btn-grid">
        <button onclick="stepSetpointModal(-25)">-25°</button>
        <button onclick="stepSetpointModal(-5)">-5°</button>
        <button onclick="stepSetpointModal(5)">+5°</button>
        <button onclick="stepSetpointModal(25)">+25°</button>
      </div>

      <div class="modal-actions">
        <button class="btn-done" style="flex:1;" onclick="closeSetpointSheet()">Done</button>
      </div>
    </div>
  </div>

  <!-- Mobile Alarm Sheet Modal -->
  <div id="alarmBackdrop" class="modal-backdrop" onclick="closeOnBackdrop(event, 'alarmBackdrop')">
    <div class="modal-sheet">
      <div class="modal-head">
        <div id="modalTitle" class="modal-title">Probe Alarm</div>
        <div class="subtext" style="cursor:pointer;" onclick="closeAlarmSheet()">✕ CLOSE</div>
      </div>

      <div class="modal-display">
        <div class="subtext" style="margin-bottom:2px;">TARGET ALERT (°F)</div>
        <input 
          type="number" 
          inputmode="numeric" 
          pattern="[0-9]*" 
          id="modalTargetInput" 
          class="modal-temp-input" 
          placeholder="Off" 
          min="0" 
          max="300"
          onchange="commitInputAlarm()"
        >
      </div>

      <div class="btn-grid">
        <button onclick="stepModalAlarm(-25)">-25°</button>
        <button onclick="stepModalAlarm(-5)">-5°</button>
        <button onclick="stepModalAlarm(5)">+5°</button>
        <button onclick="stepModalAlarm(25)">+25°</button>
      </div>

      <div class="modal-actions">
        <button class="btn-clear" onclick="clearModalAlarm()">Turn Off</button>
        <button class="btn-done" onclick="closeAlarmSheet()">Done</button>
      </div>
    </div>
  </div>

  <script>
    let currentTarget = 150;
    let currentPower = 1;
    let powerActual = 0;
    let isCooldown = false;
    let probeTargets = [0, 0, 0];
    let probeTemps = [0, 0, 0];
    let activeModalProbe = 1;
    let chartInstance = null;

    let audioCtx = null;
    let lastAlarmBeep = 0;

    function initAudio() {
      if (!audioCtx) {
        audioCtx = new (window.AudioContext || window.webkitAudioContext)();
      }
      if (audioCtx.state === 'suspended') {
        audioCtx.resume();
      }
    }

    async function togglePower() {
      if (isCooldown || powerActual === 0) return;

      if (powerActual === 1 && currentPower === 1) {
        if (!(await showDialog('Turn off the grill and start the cooldown cycle?', true))) return;
        
        currentPower = 0;
        updatePowerBtnUI(true);
        await fetch('/api/power?state=0', { method: 'POST' });
        fetchStatus();
      }
    }

    function updatePowerBtnUI(isConnected) {
      const btn = document.getElementById('powerBtn');
      
      if (!isConnected) {
        btn.textContent = '--';
        btn.style.background = 'var(--card-inner)';
        btn.style.color = 'var(--subtext)';
        btn.style.pointerEvents = 'none';
        return;
      }

      if (isCooldown) {
        btn.textContent = 'Cooling Down';
        btn.style.background = 'var(--accent)';
        btn.style.borderColor = 'var(--accent)';
        btn.style.color = 'white';
        btn.style.pointerEvents = 'none';
      } else if (currentPower === 0 && powerActual === 1) {
        btn.textContent = 'Stopping...';
        btn.style.background = 'var(--border)';
        btn.style.borderColor = 'var(--border)';
        btn.style.color = 'white';
        btn.style.pointerEvents = 'none';
      } else if (powerActual === 1) {
        btn.textContent = 'Turn Off';
        btn.style.background = 'var(--red)';
        btn.style.borderColor = 'var(--red)';
        btn.style.color = 'white';
        btn.style.pointerEvents = 'auto'; // The only time it can be clicked
      } else {
        // Grill is off or starting up. Remote start is disabled.
        btn.textContent = 'Start at Grill';
        btn.style.background = 'var(--card-inner)';
        btn.style.borderColor = 'var(--border)';
        btn.style.color = 'var(--subtext)';
        btn.style.pointerEvents = 'none'; 
      }
    }

    function playAlarmChime() {
      initAudio();
      if (!audioCtx) return;
      const now = audioCtx.currentTime;

      const freqs = [880, 1174.66, 1318.51];
      freqs.forEach((freq, idx) => {
        const osc = audioCtx.createOscillator();
        const gain = audioCtx.createGain();
        osc.type = 'sine';
        osc.frequency.setValueAtTime(freq, now + idx * 0.14);
        gain.gain.setValueAtTime(0.3, now + idx * 0.14);
        gain.gain.exponentialRampToValueAtTime(0.001, now + idx * 0.14 + 0.12);
        osc.connect(gain);
        gain.connect(audioCtx.destination);
        osc.start(now + idx * 0.14);
        osc.stop(now + idx * 0.14 + 0.12);
      });
    }

    function formatElapsed(sec) {
      if (!sec || sec <= 0) return 'Standby';
      const h = Math.floor(sec / 3600);
      const m = Math.floor((sec % 3600) / 60);
      if (h > 0) return `${h}h ${m}m`;
      return `${m}m ${sec % 60}s`;
    }

    function initChart() {
      if (typeof Chart === 'undefined') return;
      const ctx = document.getElementById('cookChart').getContext('2d');
      chartInstance = new Chart(ctx, {
        type: 'line',
        data: {
          labels: [],
          datasets: [
            { label: 'Pit', borderColor: '#ff6d00', backgroundColor: 'transparent', borderWidth: 2, pointRadius: 0, data: [] },
            { label: 'Set', borderColor: '#8a93a0', borderDash: [5, 5], backgroundColor: 'transparent', borderWidth: 1.5, pointRadius: 0, data: [] },
            { label: 'P1', borderColor: '#2979ff', backgroundColor: 'transparent', borderWidth: 2, pointRadius: 0, data: [] },
            { label: 'P2', borderColor: '#00e676', backgroundColor: 'transparent', borderWidth: 2, pointRadius: 0, data: [] },
            { label: 'P3', borderColor: '#d500f9', backgroundColor: 'transparent', borderWidth: 2, pointRadius: 0, data: [] }
          ]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          animation: false,
          scales: {
            x: { display: true, grid: { color: '#21252d' }, ticks: { color: '#8a93a0', font: { size: 10 }, maxRotation: 0, autoSkip: true, maxTicksLimit: 6 } },
            y: { grid: { color: '#2b313c' }, ticks: { color: '#8a93a0' } }
          },
          plugins: { legend: { labels: { color: '#f0f2f5', boxWidth: 10, font: { size: 10 } } } }
        }
      });
    }

    function renderOfflineCanvas(data) {
      const canvas = document.getElementById('cookChart');
      if (!canvas || !data || data.length === 0) return;
      const ctx = canvas.getContext('2d');
      const w = canvas.width = canvas.parentElement.clientWidth;
      const h = canvas.height = canvas.parentElement.clientHeight;

      ctx.clearRect(0, 0, w, h);

      ctx.strokeStyle = '#21252d';
      ctx.lineWidth = 1;
      for (let y = 10; y < h; y += 35) {
        ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
      }

      let min = 100, max = 250;
      data.forEach(d => {
        if (d.pit > max) max = d.pit;
        if (d.sp > max) max = d.sp;
        if (d.p1 > max && d.p1 > 32) max = d.p1;
      });
      max += 25;

      function plot(key, color, dash = []) {
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.setLineDash(dash);
        ctx.beginPath();
        let started = false;
        data.forEach((d, i) => {
          const val = d[key];
          if (val <= 32 && (key === 'p1' || key === 'p2' || key === 'p3')) return;
          const x = (i / (data.length - 1 || 1)) * (w - 20) + 10;
          const y = h - ((val - min) / (max - min)) * (h - 25) - 10;
          if (!started) { ctx.moveTo(x, y); started = true; } else { ctx.lineTo(x, y); }
        });
        ctx.stroke();
        ctx.setLineDash([]);
      }

      plot('pit', '#ff6d00');
      plot('sp', '#8a93a0', [4, 4]);
      plot('p1', '#2979ff');
      plot('p2', '#00e676');
      plot('p3', '#d500f9');
    }

    function updateProbeBox(num, temp, target, isConnected) {
      const tempElem = document.getElementById('p' + num);
      const pillElem = document.getElementById('p' + num + 'Pill');

      if (!isConnected || temp <= 32) {
        tempElem.textContent = !isConnected ? '--' : 'Off';
        pillElem.style.display = 'none'; 
        return false;
      }

      pillElem.style.display = 'block'; 
      tempElem.textContent = temp + '°';

      if (target > 0) {
        if (temp >= target) {
          pillElem.textContent = target + '° READY';
          pillElem.className = 'alarm-pill ready';
          return true;
        } else {
          pillElem.textContent = 'ALARM ' + target + '°';
          pillElem.className = 'alarm-pill active';
        }
      } else {
        pillElem.textContent = 'Set Alarm';
        pillElem.className = 'alarm-pill';
      }
      return false;
    }

    async function fetchStatus() {
      try {
        const res = await fetch('/api/status');
        const data = await res.json();
        
        // 1. Variable Extraction
        currentTarget = data.setpoint_target || currentTarget;
        probeTargets = [data.probe1_target || 0, data.probe2_target || 0, data.probe3_target || 0];
        probeTemps = [data.probe1 || 0, data.probe2 || 0, data.probe3 || 0];
        
        isCooldown = data.cooldown === true;
        powerActual = data.power_actual !== undefined ? data.power_actual : 0;
        currentPower = data.power_cmd !== undefined ? data.power_cmd : currentPower;

        // 2. Global UI Updates
        updatePowerBtnUI(data.connected);
        if (data.ip && data.ip.indexOf('(AP)') !== -1) {
          document.getElementById('ipLabel').textContent = 'IP: ' + data.ip;
        } else {
          document.getElementById('ipLabel').textContent = 'Status: Connected';
        }
        document.getElementById('timerLabel').textContent = '| COOK: ' + formatElapsed(data.elapsed);

        // 3. Consolidated Connection State Block
        const badge = document.getElementById('statusBadge');
        if (data.connected) {
          badge.textContent = 'ONLINE';
          badge.className = 'badge online';
          document.getElementById('pitTemp').textContent = data.pit_temp || '--';
          document.getElementById('setpointActual').textContent = currentTarget;
        } else {
          badge.textContent = 'DISCONNECTED';
          badge.className = 'badge';
          document.getElementById('pitTemp').textContent = '--';
          document.getElementById('setpointActual').textContent = '--';
        }

        // 4. Probe & Alarm Checks (data.connected is passed in to handle clearing)
        const p1Ready = updateProbeBox(1, probeTemps[0], probeTargets[0], data.connected);
        const p2Ready = updateProbeBox(2, probeTemps[1], probeTargets[1], data.connected);
        const p3Ready = updateProbeBox(3, probeTemps[2], probeTargets[2], data.connected);

        if ((p1Ready || p2Ready || p3Ready) && (Date.now() - lastAlarmBeep > 25000)) {
          lastAlarmBeep = Date.now();
          playAlarmChime();
        }
      } catch (err) {
        document.getElementById('statusBadge').textContent = 'UNREACHABLE';
        document.getElementById('statusBadge').className = 'badge';
      }
    }

    async function fetchHistory() {
      try {
        const res = await fetch('/api/history');
        const data = await res.json();
        if (!Array.isArray(data)) return;

        if (typeof Chart !== 'undefined' && chartInstance) {
          const now = Date.now();
          const lastT = data.length > 0 ? data[data.length - 1].t : 0;

          chartInstance.data.labels = data.map(d => {
            const ptDate = new Date(now - (lastT - d.t) * 1000);
            return ptDate.toLocaleTimeString([], { hour: 'numeric', minute: '2-digit' });
          });

          chartInstance.data.datasets[0].data = data.map(d => d.pit);
          chartInstance.data.datasets[1].data = data.map(d => d.sp);
          chartInstance.data.datasets[2].data = data.map(d => d.p1 > 32 ? d.p1 : null);
          chartInstance.data.datasets[3].data = data.map(d => d.p2 > 32 ? d.p2 : null);
          chartInstance.data.datasets[4].data = data.map(d => d.p3 > 32 ? d.p3 : null);
          chartInstance.update();
        } else {
          renderOfflineCanvas(data);
        }
      } catch (err) {
        console.error('History failed', err);
      }
    }

    async function clearHistory() {
      if (await showDialog('Clear telemetry history graph?', true)) {
        await fetch('/api/history/clear', { method: 'POST' });
        fetchHistory();
      }
    }

    async function sendSetpoint(temp) {
      currentTarget = temp;
      document.getElementById('setpointActual').textContent = temp;
      await fetch('/api/setpoint?temp=' + temp, { method: 'POST' });
      fetchStatus();
    }

    function openSetpointSheet() {
      initAudio();
      const input = document.getElementById('modalSetpointInput');
      input.value = currentTarget;
      document.getElementById('setpointBackdrop').classList.add('open');
    }

    function closeSetpointSheet() {
      commitSetpointModalInput();
      document.getElementById('setpointBackdrop').classList.remove('open');
    }

    function commitSetpointModalInput() {
      const input = document.getElementById('modalSetpointInput');
      let val = parseInt(input.value);
      if (isNaN(val) || val < 150) val = 150;
      if (val > 500) val = 500;
      input.value = val;
      sendSetpoint(val);
    }

    function stepSetpointModal(delta) {
      const input = document.getElementById('modalSetpointInput');
      let cur = parseInt(input.value) || currentTarget;
      let next = cur + delta;
      if (next < 150) next = 150;
      if (next > 500) next = 500;
      input.value = next;
      sendSetpoint(next);
    }

    function openAlarmSheet(probeNum) {
      const target = probeTargets[probeNum - 1];
      const temp = probeTemps[probeNum - 1];

      // Intercept tap to dismiss an actively ringing alarm
      if (target > 0 && temp >= target) {
        sendAlarm(probeNum, 0);
        return;
      }

      initAudio();
      activeModalProbe = probeNum;
      document.getElementById('modalTitle').textContent = 'Probe ' + probeNum + ' Alarm';
      
      const input = document.getElementById('modalTargetInput');
      input.value = target > 0 ? target : '';
      
      document.getElementById('alarmBackdrop').classList.add('open');
    }

    function closeAlarmSheet() {
      commitInputAlarm();
      document.getElementById('alarmBackdrop').classList.remove('open');
    }

    function closeOnBackdrop(e, backdropId) {
      if (e.target.id === backdropId) {
        if (backdropId === 'setpointBackdrop') closeSetpointSheet();
        if (backdropId === 'alarmBackdrop') closeAlarmSheet();
      }
    }

    async function sendAlarm(probeNum, temp) {
      await fetch('/api/alarm?probe=' + probeNum + '&temp=' + temp, { method: 'POST' });
      probeTargets[probeNum - 1] = temp;
      fetchStatus();
    }

    function commitInputAlarm() {
      const input = document.getElementById('modalTargetInput');
      let val = parseInt(input.value);
      if (isNaN(val) || val <= 0) {
        val = 0;
        input.value = '';
      } else {
        if (val > 300) val = 300;
        input.value = val;
      }
      sendAlarm(activeModalProbe, val);
    }

    function clearModalAlarm() {
      document.getElementById('modalTargetInput').value = '';
      sendAlarm(activeModalProbe, 0);
    }

    function stepModalAlarm(delta) {
      const input = document.getElementById('modalTargetInput');
      let cur = parseInt(input.value) || 0;
      let next;

      if (cur === 0) {
        next = delta > 0 ? 165 : 0;
      } else {
        next = cur + delta;
        if (next < 50) next = 0;
        if (next > 300) next = 300;
      }

      input.value = next > 0 ? next : '';
      sendAlarm(activeModalProbe, next);
    }

    document.addEventListener('touchstart', initAudio, { once: true });
    document.addEventListener('click', initAudio, { once: true });

    window.addEventListener('DOMContentLoaded', () => {
      fetchStatus();
      initChart();
      fetchHistory();
      setInterval(fetchStatus, 3000);
      setInterval(fetchHistory, 10000);
    });

    function showDialog(message, isConfirm = false) {
      return new Promise(resolve => {
        document.getElementById('dialogMsg').textContent = message;
        const cancelBtn = document.getElementById('dialogCancel');
        const confirmBtn = document.getElementById('dialogConfirm');
        const backdrop = document.getElementById('dialogBackdrop');

        cancelBtn.style.display = isConfirm ? 'block' : 'none';

        const cleanup = () => {
          backdrop.classList.remove('open');
          confirmBtn.onclick = null;
          cancelBtn.onclick = null;
        };

        confirmBtn.onclick = () => { cleanup(); resolve(true); };
        cancelBtn.onclick = () => { cleanup(); resolve(false); };

        backdrop.classList.add('open');
      });
    }
  </script>
</body>
</html>
)rawliteral";