#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_wifi.h>

#include "types.h"
#include "web_page.h"
#include "notifications.h"

// -----------------------------------------------------------------------------
// Configurations
// -----------------------------------------------------------------------------
const char* AP_SSID = "MAK";
const char* AP_PASS = "ABCDEFGH";

const byte DNS_PORT = 53;
// DNSServer dnsServer;


WiFiUDP rawDNS;
WebServer server(80);


Preferences preferences;

String homeSSID  = "";
String homePass  = "";
String ntfyTopic = "mak-grill-alerts-test";

bool shouldReboot = false;
unsigned long rebootTime = 0;
bool isCoolingDown = false;

GrillState currentState;
GrillCommand currentCommand;
TelemetryHistory telemetry;
AlarmConfig alarms;

void recordTelemetry() {
  if (millis() - telemetry.lastRecordTime < 10000) return;
  telemetry.lastRecordTime = millis();

  telemetry.buffer[telemetry.head] = {
    (uint32_t)(millis() / 1000),
    (int16_t)currentState.pitTemp,
    (int16_t)currentCommand.targetSetPoint,
    (int16_t)currentState.probe1,
    (int16_t)currentState.probe2,
    (int16_t)currentState.probe3
  };

  telemetry.head = (telemetry.head + 1) % TELEMETRY_CAPACITY;
  if (telemetry.count < TELEMETRY_CAPACITY) telemetry.count++;
}

void checkAlarms() {
  int currentTemps[3] = {currentState.probe1, currentState.probe2, currentState.probe3};

  for (int i = 0; i < 3; i++) {
    if (alarms.targets[i] > 0 && !alarms.triggered[i]) {
      if (currentTemps[i] >= alarms.targets[i] && currentTemps[i] > 32) {
        alarms.triggered[i] = true;
        char title[32];
        char msg[64];
        snprintf(title, sizeof(title), "Probe %d Ready", i + 1);
        snprintf(msg, sizeof(msg), "Probe %d reached target: %d°F!", i + 1, currentTemps[i]);
        notifications.sendAlert(title, msg, 4, "tada,meat_on_bone");
      }
    }
  }

  if (currentState.power == 1 && currentState.pitTemp > 0) {
    if (currentState.pitTemp < (currentState.currentSetPoint - 40)) {
      if (alarms.flameoutStartTime == 0) {
        alarms.flameoutStartTime = millis();
      } else if (!alarms.flameoutTriggered && (millis() - alarms.flameoutStartTime > 480000)) {
        alarms.flameoutTriggered = true;
        char msg[80];
        snprintf(msg, sizeof(msg), "Flameout alert! Pit is %d°F (Setpoint %d°F) for > 8 mins.",
                 currentState.pitTemp, currentState.currentSetPoint);
        notifications.sendAlert("Flameout Detected", msg, 5, "rotating_light,fire");
      }
    } else {
      alarms.flameoutStartTime = 0;
      alarms.flameoutTriggered = false;
    }
  }
}

// -----------------------------------------------------------------------------
// Promiscuous Sniffer
// -----------------------------------------------------------------------------
void wifiSniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_DATA) return;
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* p = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;

  uint16_t fc = p[0] | (p[1] << 8);
  uint8_t fType = (fc >> 2) & 0x03;
  uint8_t fSubtype = (fc >> 4) & 0x0F;

  if (fType != 2) return; // Data frames only

  // Determine MAC header length (Standard 24, QoS 26)
  int macLen = 24;
  if (fSubtype == 8 || fSubtype == 9 || fSubtype == 10 || fSubtype == 11) macLen = 26;

  if (len < macLen + 8 + 20) return; // Need LLC + IPv4 minimum

  uint8_t* llc = p + macLen;
  // Check for IPv4 LLC/SNAP header (AA AA 03 00 00 00 08 00)
  if (llc[0] == 0xAA && llc[1] == 0xAA && llc[6] == 0x08 && llc[7] == 0x00) {
    uint8_t* ip = llc + 8;
    uint8_t proto = ip[9];

    char dstIP[16];
    snprintf(dstIP, sizeof(dstIP), "%d.%d.%d.%d", ip[16], ip[17], ip[18], ip[19]);

    // Filter out broadcast and traffic correctly headed to the ESP32
    if (strcmp(dstIP, "255.255.255.255") == 0 || strcmp(dstIP, "192.168.4.1") == 0) return;

    uint16_t dstPort = 0;
    int ipHeaderLen = (ip[0] & 0x0F) * 4;

    if (proto == 17 && (len >= macLen + 8 + ipHeaderLen + 4)) { // UDP
      dstPort = (ip[ipHeaderLen + 2] << 8) | ip[ipHeaderLen + 3];
      Serial.printf("[%lu ms] [SNIFFER] UDP blocked: Dest %s:%d\n", millis(), dstIP, dstPort);
    } 
    else if (proto == 6 && (len >= macLen + 8 + ipHeaderLen + 4)) { // TCP
      dstPort = (ip[ipHeaderLen + 2] << 8) | ip[ipHeaderLen + 3];
      Serial.printf("[%lu ms] [SNIFFER] TCP blocked: Dest %s:%d\n", millis(), dstIP, dstPort);
    }
    else if (proto == 1) { // ICMP
      Serial.printf("[%lu ms] [SNIFFER] ICMP Ping blocked: Dest %s\n", millis(), dstIP);
    }
  }
}

void setup() {
  Serial.begin(115200);

  unsigned long start = millis();
  while (!Serial && (millis() - start < 3000)) {
    delay(10);
  }

  Serial.println("\n========================================");
  Serial.println("       MAK Grills ESP32 Controller      ");
  Serial.println("========================================");

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[%lu ms] [AP EVENT] Device Associated! MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  millis(),
                  info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
                  info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
                  info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
  }, ARDUINO_EVENT_WIFI_AP_STACONNECTED);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[%lu ms] [AP EVENT] Device Disconnected! MAC: %02X:%02X:%02X:%02X:%02X:%02X | AID: %d\n",
                  millis(),
                  info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1],
                  info.wifi_ap_stadisconnected.mac[2], info.wifi_ap_stadisconnected.mac[3],
                  info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5],
                  info.wifi_ap_stadisconnected.aid);
  }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[%lu ms] [AP EVENT] DHCP Lease Assigned: %s\n",
                  millis(), IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
  }, ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED);

  preferences.begin("mak-config", false);
  homeSSID  = preferences.getString("ssid", "");
  homePass  = preferences.getString("pass", "");
  ntfyTopic = preferences.getString("ntfy", "mak-grill-alerts-test");
  preferences.end();

  homeSSID.trim();
  homePass.trim();

  WiFi.setSleep(false);
  WiFi.setHostname("makgrill");

  int targetChannel = 1;

  // 1. Enter Station mode to perform a clean channel scan
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  if (homeSSID.length() > 0) {
    Serial.printf("[%lu ms] [SCAN] Locating '%s' to synchronize radio channel...\n", millis(), homeSSID.c_str());
    int n = WiFi.scanNetworks(false, true, false, 300, 0, homeSSID.c_str());
    if (n > 0) {
      targetChannel = WiFi.channel(0);
      Serial.printf("[%lu ms] [SCAN] Found '%s' on Channel %d.\n", millis(), homeSSID.c_str(), targetChannel);
    }
    WiFi.scanDelete();
  }

  // 2. Switch to Dual Mode and start the SoftAP exactly on Omada's channel
  WiFi.mode(WIFI_AP_STA);

  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  
  WiFi.softAP(AP_SSID, AP_PASS, targetChannel);
  Serial.printf("[%lu ms] [AP] SSID: %s | PASS: %s | IP: %s | Channel: %d\n",
                millis(), AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str(), targetChannel);

  // 3. Connect to Omada NATURALLY
  if (homeSSID.length() > 0) {
    Serial.printf("[%lu ms] [STA] Connecting to '%s' (Omada Roaming Mode)...\n", millis(), homeSSID.c_str());
    WiFi.begin(homeSSID.c_str(), homePass.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) { // 20-second timeout
      delay(500);
      Serial.print(".");
      attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[%lu ms] [STA] Connected! Assigned IP: %s\n", millis(), WiFi.localIP().toString().c_str());
      if (MDNS.begin("makgrill")) {
        Serial.println("[mDNS] Active: http://makgrill.local");
      }
    } else {
      Serial.printf("[%lu ms] [STA] Failed to connect (Status Code: %d).\n", millis(), WiFi.status());
      WiFi.disconnect();
    }
  }

  // dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  // dnsServer.setTTL(10);
  // dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  rawDNS.begin(53);


  notifications.begin(ntfyTopic.c_str());

  // esp_wifi_set_promiscuous(true);
  // esp_wifi_set_promiscuous_rx_cb(wifiSniffer);

  // ---------------------------------------------------------------------------
  // Web Server Routes
  // ---------------------------------------------------------------------------
server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", INDEX_HTML);
  });

  server.on("/setup", HTTP_GET, []() {
    server.send(200, "text/html", SETUP_HTML);
  });

  server.on("/favicon.ico", HTTP_GET, []() {
    server.send(204); 
  });

  server.on("/api/status", HTTP_GET, []() {
    JsonDocument doc;
    doc["connected"]       = currentState.connected;
    doc["pit_temp"]        = currentState.pitTemp;
    doc["setpoint_actual"] = currentState.currentSetPoint;
    doc["setpoint_target"] = currentCommand.targetSetPoint;
    doc["power_cmd"]       = currentCommand.power;
    doc["power_actual"]    = currentState.power;
    doc["cooldown"]        = isCoolingDown;
    doc["ip"]              = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "192.168.4.1 (AP)";
    doc["ntfy"]            = ntfyTopic;
    

    uint32_t elapsedSec = 0;
    if (currentState.cookStartTime > 0) {
      elapsedSec = (millis() - currentState.cookStartTime) / 1000;
    }
    doc["elapsed"] = elapsedSec;

    doc["probe1"]          = currentState.probe1;
    doc["probe1_target"]   = alarms.targets[0];
    doc["probe2"]          = currentState.probe2;
    doc["probe2_target"]   = alarms.targets[1];
    doc["probe3"]          = currentState.probe3;
    doc["probe3_target"]   = alarms.targets[2];

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    server.send(200, "application/json", jsonResponse);
  });

  server.on("/api/history", HTTP_GET, []() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    int startIdx = (telemetry.count == TELEMETRY_CAPACITY) ? telemetry.head : 0;
    for (int i = 0; i < telemetry.count; i++) {
      int idx = (startIdx + i) % TELEMETRY_CAPACITY;
      JsonObject pt = arr.add<JsonObject>();
      pt["t"]   = telemetry.buffer[idx].timestamp;
      pt["pit"] = telemetry.buffer[idx].pitTemp;
      pt["sp"]  = telemetry.buffer[idx].setPoint;
      pt["p1"]  = telemetry.buffer[idx].probe1;
      pt["p2"]  = telemetry.buffer[idx].probe2;
      pt["p3"]  = telemetry.buffer[idx].probe3;
    }

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    server.send(200, "application/json", jsonResponse);
  });

  server.on("/api/history/clear", HTTP_POST, []() {
    telemetry.head = 0;
    telemetry.count = 0;
    telemetry.lastRecordTime = 0;
    if (currentState.pitTemp >= 150 && currentState.power == 1) {
      currentState.cookStartTime = millis();
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/api/setpoint", HTTP_POST, []() {
    int temp = server.hasArg("temp") ? server.arg("temp").toInt() : 0;
    if (temp >= 150 && temp <= 500) {
      currentCommand.targetSetPoint = temp;
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid setpoint (150 - 500)");
    }
  });

  server.on("/api/alarm", HTTP_POST, []() {
    int probe = server.hasArg("probe") ? server.arg("probe").toInt() : 0;
    int temp  = server.hasArg("temp")  ? server.arg("temp").toInt()  : 0;

    if (probe >= 1 && probe <= 3) {
      alarms.targets[probe - 1] = temp;
      alarms.triggered[probe - 1] = false;
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid probe index (1 - 3)");
    }
  });

  server.on("/api/config/ntfy", HTTP_POST, []() {
    String newNtfy = server.hasArg("ntfy") ? server.arg("ntfy") : "";
    newNtfy.trim();
    if (newNtfy.length() > 0) {
      ntfyTopic = newNtfy;
      preferences.begin("mak-config", false);
      preferences.putString("ntfy", ntfyTopic);
      preferences.end();
      notifications.begin(ntfyTopic.c_str());
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Topic cannot be empty");
    }
  });

  server.on("/api/config/ntfy/test", HTTP_POST, []() {
    if (ntfyTopic.length() > 0) {
      notifications.sendAlert("Test Alert", "This is a test notification from the ESP32 MAK Grill Controller.", 4, "bell,white_check_mark");
      server.send(200, "text/plain", "Test sent");
    } else {
      server.send(400, "text/plain", "No NTFY topic configured");
    }
  });

  server.on("/api/power", HTTP_POST, []() {
    int state = server.hasArg("state") ? server.arg("state").toInt() : -1;
    if (state == 0 || state == 1) {
      currentCommand.power = state;
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid power state");
    }
  });

  server.on("/api/wifi/save", HTTP_POST, []() {
    String newSSID = server.hasArg("ssid") ? server.arg("ssid") : "";
    String newPass = server.hasArg("pass") ? server.arg("pass") : "";
    if (newSSID.length() > 0) {
      newSSID.trim();
      newPass.trim();
      preferences.begin("mak-config", false);
      preferences.putString("ssid", newSSID);
      preferences.putString("pass", newPass);
      preferences.end();
      
      String html = "<html><body><h2>Saved</h2><p>Rebooting...</p></body></html>";
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", html);
      shouldReboot = true;
      rebootTime = millis() + 2500;
    } else {
      server.send(400, "text/plain", "SSID cannot be empty");
    }
  });

  server.on("/api/wifi/scan", HTTP_GET, []() {
    bool forceRescan = server.hasArg("rescan");
    int16_t n = WiFi.scanComplete();

    if (forceRescan && n != WIFI_SCAN_RUNNING) {
      WiFi.scanDelete();
      // Start async scan: async=true, show_hidden=true, passive=false, 120ms per channel
      WiFi.scanNetworks(true, true, false, 120); 
      server.send(200, "application/json", "{\"scanning\":true,\"networks\":[]}");
      return;
    }

    if (n == WIFI_SCAN_RUNNING) {
      server.send(200, "application/json", "{\"scanning\":true,\"networks\":[]}");
      return;
    }

    if (n > 0) {
      JsonDocument doc;
      doc["scanning"] = false;
      JsonArray arr = doc["networks"].to<JsonArray>();

      for (int i = 0; i < n; ++i) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() > 0) {
          JsonObject obj = arr.add<JsonObject>();
          obj["ssid"] = ssid;
          obj["rssi"] = WiFi.RSSI(i);
        }
      }

      String res;
      serializeJson(doc, res);
      server.send(200, "application/json", res);
      return;
    }

    if (n == 0) {
      WiFi.scanDelete();
      server.send(200, "application/json", "{\"scanning\":false,\"networks\":[]}");
      return;
    }

    // If n == WIFI_SCAN_FAILED (-2), initiate a fresh scan
    WiFi.scanDelete();
    WiFi.scanNetworks(true, true, false, 120);
    server.send(200, "application/json", "{\"scanning\":true,\"networks\":[]}");
  });
  

  // --- THE GRILL TELEMETRY INTERCEPTOR ---
  server.on("/GrillService/Service", HTTP_ANY, []() {
    if (server.hasArg("Temp")) currentState.pitTemp = server.arg("Temp").toInt();
    if (server.hasArg("SetPoint")) currentState.currentSetPoint = server.arg("SetPoint").toInt();
    if (server.hasArg("Probe1")) currentState.probe1 = server.arg("Probe1").toInt();
    if (server.hasArg("Probe2")) currentState.probe2 = server.arg("Probe2").toInt();
    if (server.hasArg("Probe3")) currentState.probe3 = server.arg("Probe3").toInt();
    
    if (server.hasArg("Power")) {
      String pwr = server.arg("Power");
      pwr.toUpperCase();
      if (pwr.indexOf("COOL") != -1 || pwr == "CD") {
        currentState.power = 0; 
        currentCommand.power = 1; // FIX: Release the '0' command so it doesn't kill future sessions
        isCoolingDown = true;
      } else if (pwr == "ON") {
        currentState.power = 1;
        isCoolingDown = false;
      } else {
        currentState.power = 0;
        currentCommand.power = 1; // FIX: Release the '0' command
        isCoolingDown = false;
      }
    }

    currentState.lastSeen = millis();
    currentState.connected = true;

    recordTelemetry();
    checkAlarms();

    // --- AUTOMATIC COOK TIMER LOGIC ---
    if (currentState.power == 1 && currentState.pitTemp >= 150) {
      // Start the timer once the grill passes the 150°F ignition threshold
      if (currentState.cookStartTime == 0) {
        currentState.cookStartTime = millis();
      }
    } else if (currentState.power == 0) {
      // Reset the timer back to Standby when the grill is turned off
      currentState.cookStartTime = 0;
    }

    char payload[128];
    snprintf(payload, sizeof(payload),
             "\"setPoint=%d&potStatus=&cookMode=%d&zoneProbe=%d&power=%d\"",
             currentCommand.targetSetPoint, currentCommand.cookMode,
             currentCommand.zoneProbe, currentCommand.power);

    // This gracefully drops the socket, ignoring the Microchip's trailing newline garbage
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", payload);
  });

  server.onNotFound([]() {
    server.send(204); 
  });

  server.begin();
  Serial.printf("[%lu ms] [HTTP] Server ready on port 80\n", millis());
}

void loop() {
  // CRITICAL FIX: The DNS Server must process requests here or the Grill gets no IP
  // dnsServer.processNextRequest();
  // --- RAW DNS SPOOFER ---
  int dnsSize = rawDNS.parsePacket();
  if (dnsSize) {
    byte buf[128] = {0};
    rawDNS.read(buf, sizeof(buf));
    
    // Extract domain for logging
    char domain[64] = {0};
    int dIdx = 0;
    int i = 12; // Skip DNS header
    while (i < dnsSize && buf[i] != 0 && dIdx < 62) {
      int labelLen = buf[i];
      for (int j = 0; j < labelLen; j++) {
        domain[dIdx++] = buf[i + 1 + j];
      }
      domain[dIdx++] = '.';
      i += labelLen + 1;
    }
    domain[dIdx] = '\0';
    
    Serial.printf("[%lu ms] [DNS] Spoofing %s -> 192.168.4.1\n", millis(), domain);

    // Build raw DNS Response
    byte response[128];
    int respLen = 0;

    // 1. Mirror the exact Transaction ID the grill sent
    response[0] = buf[0];
    response[1] = buf[1];
    
    // 2. Flags: Standard Query Response, No Error (0x8180)
    response[2] = 0x81;
    response[3] = 0x80;
    
    // 4. Questions (1)
    response[4] = 0x00;
    response[5] = 0x01;
    
    // 5. Answer RRs (1)
    response[6] = 0x00;
    response[7] = 0x01;
    
    // 6. Authority RRs (0), Additional RRs (0)
    response[8] = 0x00; response[9] = 0x00;
    response[10] = 0x00; response[11] = 0x00;
    
    // 7. Copy the original query section payload
    int queryLen = dnsSize - 12;
    memcpy(&response[12], &buf[12], queryLen);
    respLen = 12 + queryLen;
    
    // 8. Append the Answer Record (Pointer, Type A, Class IN, TTL 60, IP 192.168.4.1)
    byte answer[] = {
      0xC0, 0x0C,             // Name pointer to offset 12
      0x00, 0x01,             // Type A
      0x00, 0x01,             // Class IN
      0x00, 0x00, 0x00, 0x3C, // TTL (60 seconds)
      0x00, 0x04,             // Data length (4 bytes)
      192, 168, 4, 1          // Payload IP
    };
    
    memcpy(&response[respLen], answer, sizeof(answer));
    respLen += sizeof(answer);
    
    // 9. Blast it back to the grill
    rawDNS.beginPacket(rawDNS.remoteIP(), rawDNS.remotePort());
    rawDNS.write(response, respLen);
    rawDNS.endPacket();
  }

  server.handleClient();

  if (currentState.connected && (millis() - currentState.lastSeen > 30000)) {
    currentState.pitTemp = 0;
    currentState.probe1 = 0;
    currentState.probe2 = 0;
    currentState.probe3 = 0;
    currentState.connected = false;
  }

  // --- Home Network Watchdog (Soft Reset) ---
  static unsigned long lastWifiCheck = 0;
  static int offlineCount = 0;
  
  if (millis() - lastWifiCheck > 10000) { 
    lastWifiCheck = millis();
    if (homeSSID.length() > 0 && WiFi.status() != WL_CONNECTED) {
      offlineCount++;
      if (offlineCount >= 12) {
        Serial.println("[WATCHDOG] Connection lost. Re-syncing Omada channel...");
        WiFi.softAPdisconnect(true);
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(500);

        int targetChannel = 1;
        WiFi.mode(WIFI_STA);
        delay(100);
        int n = WiFi.scanNetworks(false, true, false, 300, 0, homeSSID.c_str());
        if (n > 0) targetChannel = WiFi.channel(0);
        WiFi.scanDelete();

        WiFi.mode(WIFI_AP_STA);
        IPAddress local_ip(192, 168, 4, 1);
        IPAddress gateway(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);
        WiFi.softAPConfig(local_ip, gateway, subnet);
        WiFi.softAP(AP_SSID, AP_PASS, targetChannel);

        WiFi.begin(homeSSID.c_str(), homePass.c_str());
        offlineCount = 0; 
      }
    } else {
      offlineCount = 0; 
    }
  }

  if (shouldReboot && (millis() > rebootTime)) {
    Serial.println("[System] Executing scheduled reboot now...");
    ESP.restart();
  }
}