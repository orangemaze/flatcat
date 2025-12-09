// web_handlers.cpp

#include "flatcat.h"

// ================================================================
// --- CUSTOM WEB SERVER IMPLEMENTATIONS ---
// ================================================================

// --- AP Mode Handlers ---
void handleNotFound() {
  String uri = server.uri();
  uri.toLowerCase();

  if (uri.startsWith("/api/v1/") || uri.startsWith("/management/")) {
    handleAlpacaAPI();
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    // Ensure setup_html is available. If not defined in header, we might have
    // issue. Assuming page_setup.h defines 'setup_html' char array.
    server.send(200, "text/html", setup_html);
  } else {
    server.send(404, "text/plain", "Not Found");
  }
}

void handleScan() {
  int n = WiFi.scanNetworks();
  StaticJsonDocument<1024> doc;
  JsonArray networks = doc.to<JsonArray>();

  for (int i = 0; i < n; ++i) {
    JsonObject net = networks.createNestedObject();
    net["ssid"] = WiFi.SSID(i);
    net["rssi"] = WiFi.RSSI(i);
    net["enc"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secure";
  }
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSaveWifi() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  preferences.begin("flatcat-wifi", false);
  preferences.clear();
  preferences.putString("wifi-ssid", ssid);
  preferences.putString("wifi-pass", pass);
  preferences.end();

  server.send(200, "text/plain", "Wi-Fi credentials saved. Rebooting...");
  delay(1000);
  ESP.restart();
}

// --- Main Server Handlers ---
void handleRoot() { server.send(200, "text/html", index_html); }
void handleSettings() { server.send(200, "text/html", settings_html); }
void handleGetTime() { server.send(200, "text/plain", currentTimeString); }

// UI Control Handlers
void handleSlider1() {
  if (server.hasArg("value")) {
    int brightness = server.arg("value").toInt();
    setDimmerValue(brightness);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleSlider2() {
  if (server.hasArg("value")) {
    currentDimmerValue_2 = server.arg("value").toInt();
    updateDimmerLock();
    int newBrightness = map(currentDimmerValue_2, 0, 64, 0, 255);
    analogWrite(elPin_2, newBrightness);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleOpen1() {
  if (isDimmerActive) {
    server.send(409, "text/plain", "Error: Turn off dimmer first.");
    return;
  }
  if (isOpenStopActive_1) {
    server.send(200, "text/plain", "Cover is already Open.");
    return;
  }

  // Command Open
  myServo_1.attach(servoPin_1, MIN_MICROS, MAX_MICROS);
  isMovingToClose_1 = false;
  isMovingToOpen_1 = true;
  coverState_1 = coverMoving;

  myServo_1.write(openAngle);
  currentServoAngle_1 = openAngle;
  moveStartTime = millis();

  server.send(200, "text/plain", "Opened");
}

void handleClose1() {
  if (isDimmerActive) {
    server.send(409, "text/plain", "Error: Turn off dimmer first.");
    return;
  }
  if (isClosedStopActive_1) {
    server.send(200, "text/plain", "Cover is already Closed.");
    return;
  }

  // Command Close
  myServo_1.attach(servoPin_1, MIN_MICROS, MAX_MICROS);
  isMovingToOpen_1 = false;
  isMovingToClose_1 = true;
  coverState_1 = coverMoving;

  myServo_1.write(closeAngle);
  currentServoAngle_1 = closeAngle;
  moveStartTime = millis();

  server.send(200, "text/plain", "Closed");
}

// Placeholder for Servo 2
void handleOpen2() {
  myServo_2.attach(servoPin_2, MIN_MICROS, MAX_MICROS);
  myServo_2.write(openAngle);
  server.send(200, "text/plain", "Opened");
}

void handleClose2() {
  myServo_2.attach(servoPin_2, MIN_MICROS, MAX_MICROS);
  myServo_2.write(closeAngle);
  server.send(200, "text/plain", "Closed");
}

void handleGetAllStatus() {
  StaticJsonDocument<200> doc;
  doc["servo1"] = currentServoAngle_1;
  doc["servo2"] = currentServoAngle_2;
  doc["dimmer1"] = currentDimmerValue_1;
  doc["dimmer2"] = currentDimmerValue_2;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleGetSettings() {
  StaticJsonDocument<400> doc;
  doc["hostname"] = currentSettings.hostname;
  doc["ip"] = currentSettings.ip;
  doc["gateway"] = currentSettings.gateway;
  doc["subnet"] = currentSettings.subnet;
  doc["title1"] = currentSettings.title1;
  doc["title2"] = currentSettings.title2;
  doc["gmtOffset"] = currentSettings.gmtOffset;
  doc["daylightOffset"] = currentSettings.daylightOffset;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSave() {
  preferences.begin("flatcat", false);
  if (server.hasArg("title1"))
    preferences.putString("title1", server.arg("title1"));
  if (server.hasArg("title2"))
    preferences.putString("title2", server.arg("title2"));
  if (server.hasArg("gmtOffset"))
    preferences.putLong("gmtOffset", server.arg("gmtOffset").toInt());
  if (server.hasArg("daylightOffset"))
    preferences.putInt("daylightOffset", server.arg("daylightOffset").toInt());

  // Network settings might require reboot logic or reconfig, but saving is
  // fine.
  if (server.hasArg("hostname"))
    preferences.putString("hostname", server.arg("hostname"));
  if (server.hasArg("ip"))
    preferences.putString("ip", server.arg("ip"));
  if (server.hasArg("gateway"))
    preferences.putString("gateway", server.arg("gateway"));
  if (server.hasArg("subnet"))
    preferences.putString("subnet", server.arg("subnet"));

  preferences.end();
  server.send(200, "text/html", reboot_html);
  delay(1000);
  ESP.restart();
}
