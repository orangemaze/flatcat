// config_utils.cpp

#include "flatcat.h"

// ================================================================
// --- UTILITY FUNCTIONS ---
// ================================================================

bool isNumeric(String str) {
  for (int i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) {
      return false;
    }
  }
  return true;
}

void setDimmerValue(int brightness) {
  brightness = constrain(brightness, 0, MAX_BRIGHTNESS);
  currentDimmerValue_1 = brightness;
  int newPwmValue = map(currentDimmerValue_1, 0, MAX_BRIGHTNESS, 0, 255);
  analogWrite(elPin_1, newPwmValue);
  isDimmerActive = (brightness > 0);
}

void updateCoverStatus() {
  // Read the status of the Hall sensors
  isClosedStopActive_1 = (digitalRead(closedStopPin_1) == LOW);
  isOpenStopActive_1 = (digitalRead(openStopPin_1) == LOW);

  // Determine the new cover state
  if (isClosedStopActive_1 && !isOpenStopActive_1) {
    coverState_1 = coverClosed;
  } else if (isOpenStopActive_1 && !isClosedStopActive_1) {
    coverState_1 = coverOpen;
  } else if (!isClosedStopActive_1 && !isOpenStopActive_1) {
    // If we are not moving, we trust the last known state
    if (coverState_1 != coverMoving && coverState_1 != coverOpen &&
        coverState_1 != coverClosed) {
      coverState_1 = coverReady;
    }
  } else {
    // Both active? Error.
    coverState_1 = coverReady;
  }

  // Debug output
  static long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    lastDebug = millis();
    Serial.printf("Sensors: Closed=%d Open=%d | State=%d\n",
                  isClosedStopActive_1, isOpenStopActive_1, coverState_1);
  }
}

void checkAndStopServo() {
  // --- Stop Logic for Cover 1 ---

  // A. Moving Close
  if (isMovingToClose_1) {
    // Check physical sensor or time
    if (isClosedStopActive_1 ||
        (millis() - moveStartTime > SIMULATED_MOVEMENT_TIME)) {
      myServo_1.detach();
      coverState_1 = coverClosed;
      isMovingToClose_1 = false;
    }
  }
  // B. Moving Open
  else if (isMovingToOpen_1) {
    if (isOpenStopActive_1 ||
        (millis() - moveStartTime > SIMULATED_MOVEMENT_TIME)) {
      myServo_1.detach();
      coverState_1 = coverOpen;
      isMovingToOpen_1 = false;
    }
  }
}

void initializeUniqueID() {
  preferences.begin("flatcat", true);
  if (preferences.isKey("device-uuid")) {
    deviceUniqueID = preferences.getString("device-uuid");
  } else {
    preferences.end();
    preferences.begin("flatcat", false);

    uint8_t baseMac[6];
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    deviceUniqueID = String(baseMac[3], HEX);
    deviceUniqueID.toUpperCase();
    deviceUniqueID += String(baseMac[4], HEX);
    deviceUniqueID.toUpperCase();
    deviceUniqueID += String(baseMac[5], HEX);
    deviceUniqueID.toUpperCase();

    while (deviceUniqueID.length() < 10) {
      deviceUniqueID = "0" + deviceUniqueID;
    }

    preferences.putString("device-uuid", deviceUniqueID);
    preferences.end();
  }
}

void updateDimmerLock() {
  if (coverState_1 == coverOpen || coverState_1 == coverMoving) {
    isDimmerActive = false;
    analogWrite(elPin_1, 0);
    analogWrite(elPin_2, 0);
    calibratorState_1 = calibratorNotReady;
  } else {
    isDimmerActive = (currentDimmerValue_1 > 0 || currentDimmerValue_2 > 0);
    if (isDimmerActive) {
      calibratorState_1 = calibratorReady;
    } else {
      calibratorState_1 = calibratorOff;
    }
  }
}

void loadSettings() {
  preferences.begin("flatcat", false);
  currentSettings.hostname = preferences.getString("hostname", "flatcat");
  currentSettings.ip = preferences.getString("ip", "192.168.1.150");
  currentSettings.gateway = preferences.getString("gateway", "192.168.1.1");
  currentSettings.subnet = preferences.getString("subnet", "255.255.255.0");
  currentSettings.title1 = preferences.getString("title1", "FlatCat");
  currentSettings.title2 = preferences.getString("title2", "Scope 2");
  currentSettings.gmtOffset = preferences.getLong("gmtOffset", -18000);
  currentSettings.daylightOffset = preferences.getInt("daylightOffset", 3600);
  preferences.end();
}

void startApMode() {
  WiFi.softAP(ap_ssid);
  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(53, "*", apIP);
  server.onNotFound(handleNotFound);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/savewifi", HTTP_POST, handleSaveWifi);
  server.begin();
}

void startMainServer() {
  loadSettings();

  IPAddress local_IP, gateway_IP, subnet_IP;
  local_IP.fromString(currentSettings.ip);
  gateway_IP.fromString(currentSettings.gateway);
  subnet_IP.fromString(currentSettings.subnet);

  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);

  WiFi.setHostname(currentSettings.hostname.c_str());

  if (!WiFi.config(local_IP, gateway_IP, subnet_IP, primaryDNS, secondaryDNS)) {
    // Fail silently or log
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(currentSettings.hostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
  }

  configTime(currentSettings.gmtOffset, currentSettings.daylightOffset,
             ntpServer);
  startAlpacaDiscovery();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/gettime", HTTP_GET, handleGetTime);
  server.on("/slider1", HTTP_GET, handleSlider1);
  server.on("/slider2", HTTP_GET, handleSlider2);
  server.on("/open1", HTTP_GET, handleOpen1);
  server.on("/close1", HTTP_GET, handleClose1);
  server.on("/open2", HTTP_GET, handleOpen2);
  server.on("/close2", HTTP_GET, handleClose2);
  server.on("/getstatus", HTTP_GET, handleGetAllStatus);
  server.on("/getsettings", HTTP_GET, handleGetSettings);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Main web server started.");
}
