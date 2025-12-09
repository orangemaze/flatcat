// web_handlers.cpp

#include "flatcat.h"

// ================================================================
// --- CUSTOM WEB SERVER IMPLEMENTATIONS ---
// ================================================================

// --- AP Mode Handlers ---
void handleNotFound() {
  String uri = server.uri();
  uri.toLowerCase();

  // 1. Check if it's an Alpaca API call
  if (uri.startsWith("/api/v1/") || uri.startsWith("/management/")) {
    handleAlpacaAPI();
    return;
  }

  // If we are in AP mode, redirect everything to the setup page (Captive
  // Portal)
  if (WiFi.status() != WL_CONNECTED) {
    server.send(200, "text/html", setup_html);
  } else {
    // In normal mode, just send 404
    server.send(404, "text/plain", "Not Found");
  }
}

void handleScan() {
  // Serial.println("Scanning for networks...");
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
  // Serial.println("Saving Wi-Fi credentials...");
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  // ... (logic to determine ssid) ...

  // CRITICAL: Must be 'false' for WRITE access
  preferences.begin("flatcat-wifi", false);

  // FIX: Clear the namespace entirely to remove any corruption.
  preferences.clear();

  preferences.putString("wifi-ssid", ssid);
  preferences.putString("wifi-pass", pass);

  // Commit and close the session.
  preferences.end();

  // Serial.println("Credentials saved. Rebooting...");
  server.send(
      200, "text/plain",
      "Wi-Fi credentials saved. The device will now reboot in 5 seconds.");
  preferences.begin("flatcat-wifi", true); // TRUE is correct for reading
  String saved_ssid = preferences.getString("wifi-ssid", "");
  String saved_pass = preferences.getString("wifi-pass", "");
  // Serial.println(saved_ssid);
  // Serial.println(saved_pass);
  preferences.end();

  delay(5000);
  ESP.restart();
}

// --- Main Server Handlers ---
void handleRoot() { server.send(200, "text/html", index_html); }

void handleSettings() { server.send(200, "text/html", settings_html); }

void handleGetTime() { server.send(200, "text/plain", currentTimeString); }

// UI Control Handlers
void handleSlider1() {
  Serial.println("hi");
  if (server.hasArg("value")) {
    int brightness = server.arg("value").toInt();

    // Call the new centralized function
    setDimmerValue(brightness);

    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

/* void handleSlider2() {
  if (server.hasArg("value")) {
    currentDimmerValue_2 = server.arg("value").toInt();
    updateDimmerLock();
    int newBrightness = map(currentDimmerValue_2, 0, 64, 0, 255);
    analogWrite(elPin_2, newBrightness);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
} */

// --- OPEN HANDLER ---
void handleOpen1() {
  Serial.println("DEBUG: handleOpen1 called."); // <--- ADDED
  if (isDimmerActive) {
    Serial.println("DEBUG: Blocked by DimmerActive."); // <--- ADDED
    server.send(409, "text/plain", "Error: Turn off dimmer first.");
    return;
  }

  if (isOpenStopActive_1) {
    Serial.println("DEBUG: Blocked by Sensor (Already Open)."); // <--- ADDED
    server.send(200, "text/plain", "Cover is already Open.");
    return;
  }

  // NEW: Attach, set flags, and command move
  Serial.println("DEBUG: Attaching servo..."); // <--- ADDED
  myServo_1.attach(servoPin_1);
  isMovingToClose_1 = false; // Ensure the opposite flag is clear
  isMovingToOpen_1 = true;   // Set the target flag
  coverState_1 = coverMoving;

  Serial.print("DEBUG: Writing Angle: ");
  Serial.println(openAngle);  // <--- ADDED
  myServo_1.write(openAngle); // Command the full travel angle

  // DEBUG: Force wait to ensure signal generates
  Serial.println("DEBUG: Blocking wait 1000ms...");
  delay(1000);

  currentServoAngle_1 = openAngle;

  server.send(200, "text/plain", "Opened");
  Serial.println("Cover 1 commanded to open.");
}

// --- CLOSE HANDLER ---
void handleClose1() {
  Serial.println("DEBUG: handleClose1 called."); // <--- ADDED
  if (isDimmerActive) {
    Serial.println("DEBUG: Blocked by DimmerActive."); // <--- ADDED
    server.send(409, "text/plain", "Error: Turn off dimmer first.");
    return;
  }

  if (isClosedStopActive_1) {
    Serial.println("DEBUG: Blocked by Sensor (Already Closed)."); // <--- ADDED
    server.send(200, "text/plain", "Cover is already Closed.");
    return;
  }

  // NEW: Attach, set flags, and command move
  Serial.println("DEBUG: Attaching servo..."); // <--- ADDED
  myServo_1.attach(servoPin_1);
  isMovingToOpen_1 = false; // Ensure the opposite flag is clear
  isMovingToClose_1 = true; // Set the target flag
  coverState_1 = coverMoving;

  Serial.print("DEBUG: Writing Angle: ");
  Serial.println(closeAngle);  // <--- ADDED
  myServo_1.write(closeAngle); // Command the full travel angle

  // DEBUG: Force wait to ensure signal generates
  Serial.println("DEBUG: Blocking wait 1000ms...");
  delay(1000);

  currentServoAngle_1 = closeAngle;

  server.send(200, "text/plain", "Closed");
  Serial.println("Cover 1 commanded to close.");
}

void handleGetAllStatus() {
  StaticJsonDocument<200> doc;
  doc["servo1"] = currentServoAngle_1;
  // doc["servo2"] = currentServoAngle_2;
  doc["dimmer1"] = currentDimmerValue_1;
  // doc["dimmer2"] = currentDimmerValue_2;
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
  // doc["title2"] = currentSettings.title2;
  doc["gmtOffset"] = currentSettings.gmtOffset;
  doc["daylightOffset"] = currentSettings.daylightOffset;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void clearSavedSettings() {
  // Clear the Wi-Fi settings namespace
  preferences.begin("flatcat-wifi", false);
  preferences.clear();
  preferences.end();

  // Clear the main device settings namespace (where title1 is saved)
  preferences.begin("flatcat", false);
  preferences.clear();
  preferences.end();

  // Serial.println("All saved configurations have been erased.");
  ESP.restart();
}

void handleSave() {
  // Serial.println("Saving new settings...");
  preferences.begin("flatcat", false);

  if (server.hasArg("title1"))
    preferences.putString("title1", server.arg("title1"));
  // if (server.hasArg("title2"))
  //   preferences.putString("title2", server.arg("title2"));
  if (server.hasArg("gmtOffset"))
    preferences.putLong("gmtOffset", server.arg("gmtOffset").toInt());
  if (server.hasArg("daylightOffset"))
    preferences.putInt("daylightOffset", server.arg("daylightOffset").toInt());
  if (server.hasArg("hostname"))
    preferences.putString("hostname", server.arg("hostname"));
  if (server.hasArg("ip"))
    preferences.putString("ip", server.arg("ip"));
  if (server.hasArg("gateway"))
    preferences.putString("gateway", server.arg("gateway"));
  if (server.hasArg("subnet"))
    preferences.putString("subnet", server.arg("subnet"));

  preferences.end();
  // Serial.println("Settings saved. Rebooting.");
  server.send(200, "text/html", reboot_html);
  delay(1000);
  ESP.restart();
}

// --- DEBUG HANDLER FOR WIFI TOGGLE TEST ---
void handleTestMove() {
  server.send(200, "text/plain",
              "Starting Wireless-Free Move Test in 2 seconds...");
  delay(2000); // Give time for response to send

  Serial.println("DEBUG: Disabling WiFi for Move...");
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(500);

  Serial.println("DEBUG: Moving Servo...");
  myServo_1.attach(servoPin_1);
  // Just wiggle it 0-90-0 to test power
  myServo_1.write(openAngle);
  delay(1000);
  myServo_1.write(closeAngle);
  delay(1000);
  myServo_1.detach();
  Serial.println("DEBUG: Move Complete. Reconnecting WiFi...");

  // Re-enable WiFi (Logic borrowed from setup)
  WiFi.begin(currentSettings.ip.c_str(), currentSettings.gateway.c_str());
  // Actually we need to reload credentials or just restart?
  // Easier to just restart to ensure clean state
  ESP.restart();
}
