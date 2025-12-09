#line 1 "C:\\Users\\orangemaze\\Antigravity\\flatcat_2.0\\config_utilities.cpp"
// config_utils.cpp

#include "flatcat.h" // Includes all necessary declarations

// ================================================================
// --- UTILITY FUNCTIONS ---
// ================================================================
/**
 * @brief Checks the Hall sensors to determine the cover's absolute state.
 * * Updates the global coverState_1 based on digital sensor readings.
 * Uses the INPUT_PULLUP setup, meaning the sensor reads LOW when the magnet is
 * present.
 */
/**
 * @brief Checks if a string contains only digits.
 */
bool isNumeric(String str) {
  for (int i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) {
      return false;
    }
  }
  return true;
}

void setDimmerValue(int brightness) {
  // 1. Clamp the brightness value to the valid range (1 to MaxBrightness).
  brightness = constrain(brightness, 0, maxBrightness);

  // 2. Update the global state variable
  currentDimmerValue_1 = brightness;
  Serial.println(currentDimmerValue_1);
  // 3. Map the Alpaca brightness (0-64) to the hardware PWM duty cycle (0-255).
  int newPwmValue = map(currentDimmerValue_1, 0, maxBrightness, 0, 255);
  Serial.println(newPwmValue);
  analogWrite(elPin_1, newPwmValue);

  // 4. Update the dimmer active flag
  isDimmerActive = (brightness > 0);
}

int validateDeviceNumber(String uri) {
  // Find the position of the device number (which is always after the interface
  // name)
  int start = uri.indexOf("covercalibrator/");
  if (start == -1)
    return -1; // Should not happen if routed correctly

  start += strlen("covercalibrator/");

  int end = uri.indexOf('/', start);
  if (end == -1)
    end =
        uri.length(); // Handle cases where device number is the end of the URI

  String devNumStr = uri.substring(start, end);

  // Check if the number is valid (e.g., "0")
  if (devNumStr.equals("0")) {
    return 0; // Device 0 is valid (CoverCalibrator 1)
  }

  // Reject non-numeric or out-of-range device numbers
  if (!devNumStr.toInt())
    return -1; // Catches "A" and other garbage
  if (devNumStr.toInt() != 0)
    return -1; // Catches -1, 99999, etc.

  return -1; // Invalid device number
}

void updateCoverStatus() {
  // SENSOR BYPASS MODE:
  // We rely entirely on the command handlers (Web/Alpaca) to set the state.
  // This function is disabled to prevent it from resetting the state to "Ready"
  // just because no physical sensors are triggering.

  /*
  isClosedStopActive_1 = false;
  isOpenStopActive_1 = false;

  if (isClosedStopActive_1 && !isOpenStopActive_1) {
    coverState_1 = coverClosed;
  } else if (isOpenStopActive_1 && !isClosedStopActive_1) {
    coverState_1 = coverOpen;
  } else if (!isClosedStopActive_1 && !isOpenStopActive_1) {
    if (coverState_1 != coverMoving) {
      coverState_1 = coverReady;
    }
  } else {
    coverState_1 = coverReady;
  }
  */
}

void checkAndStopServo() {
  // --- Stop Logic for Cover 1 ---
  // FIX: Bypassed sensor stop logic until sensors are attached

  // A. Check if the cover was commanded to CLOSE
  if (isMovingToClose_1) {
    /*
    if (isClosedStopActive_1) {
      myServo_1.detach(); // Power off the servo immediately to stop movement
      coverState_1 = coverClosed; // Confirm final state
      isMovingToClose_1 = false;  // Clear the command flag
      // Serial.println("Cover 1: CLOSED stop sensor hit. Servo detached.");
    }
    */
  }
  // B. Check if the cover was commanded to OPEN
  else if (isMovingToOpen_1) {
    /*
    if (isOpenStopActive_1) {
      myServo_1.detach(); // Power off the servo immediately to stop movement
      coverState_1 = coverOpen;   // Confirm final state
      isMovingToOpen_1 = false;   // Clear the command flag
      // Serial.println("Cover 1: OPEN stop sensor hit. Servo detached.");
    }
    */
  }
}

void initializeUniqueID() {
  preferences.begin("flatcat", true); // Read-only access initially

  if (preferences.isKey("device-uuid")) {
    // Load existing ID
    deviceUniqueID = preferences.getString("device-uuid");
    // Serial.printf("Loaded Unique ID: %s\n", deviceUniqueID.c_str());
  } else {
    // ID does not exist, must generate and save it
    preferences.end();                   // Close read-only access
    preferences.begin("flatcat", false); // Open read/write access

    // Generate a simple unique ID from the MAC address (safer than pure random)
    uint8_t baseMac[6];
    // esp_efuse_read_mac(baseMac);
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    // Format as a hex string (e.g., 40:0C:0D:F1:C0:B5) and remove colons
    deviceUniqueID = String(baseMac[3], HEX);
    deviceUniqueID.toUpperCase();
    deviceUniqueID += String(baseMac[4], HEX);
    deviceUniqueID.toUpperCase();
    deviceUniqueID += String(baseMac[5], HEX);
    deviceUniqueID.toUpperCase();

    // Ensure the ID is a large integer string for better compliance
    while (deviceUniqueID.length() < 10) {
      deviceUniqueID = "0" + deviceUniqueID;
    }

    preferences.putString("device-uuid", deviceUniqueID);
    preferences.end();
    // Serial.printf("Generated and Saved Unique ID: %s\n",
    // deviceUniqueID.c_str());
  }
}
void updateDimmerLock() {

  if (coverState_1 == coverClosed || coverState_1 == coverMoving) {
    // If the cover is closed or moving, the dimmer MUST be off.
    isDimmerActive = false;
    analogWrite(elPin_1, 0);
    // analogWrite(elPin_2, 0);

    // Set the Alpaca Calibrator State
    calibratorState_1 = calibratorNotReady; // Cannot be used right now
  } else {
    // Allow dimmers to operate if the cover is open or ready.
    isDimmerActive =
        (currentDimmerValue_1 > 0 /* || currentDimmerValue_2 > 0 */);

    // Set the Alpaca Calibrator State
    if (isDimmerActive) {
      calibratorState_1 = calibratorReady; // It's currently running (ON)
    } else {
      calibratorState_1 =
          calibratorReady; // It's currently OFF, but ready to be turned ON
    }
  }
}

void loadSettings() { // keep
  preferences.begin("flatcat", false);
  currentSettings.hostname =
      preferences.getString("hostname", "flatcat"); // Set default explicitly
  currentSettings.ip = preferences.getString("ip", "192.168.1.150");
  currentSettings.gateway = preferences.getString("gateway", "192.168.1.1");
  currentSettings.subnet = preferences.getString("subnet", "255.255.255.0");
  currentSettings.title1 = preferences.getString("title1", "FlatCat");
  currentSettings.title2 = preferences.getString("title2", "Scope 2");
  currentSettings.gmtOffset = preferences.getLong("gmtOffset", -18000);
  currentSettings.daylightOffset = preferences.getInt("daylightOffset", 3600);
  preferences.end();
  // Serial.println("Loaded all settings.");
}

void startApMode() { // keep
  // Serial.println("No saved credentials. Starting Access Point mode...");
  WiFi.softAP(ap_ssid);
  IPAddress apIP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  // Serial.println(apIP);

  dnsServer.start(53, "*", apIP);

  server.onNotFound(handleNotFound);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/savewifi", HTTP_POST, handleSaveWifi);

  server.begin();
  // Serial.println("AP Mode web server started.");
}

void startMainServer() { // keep
  // Serial.println("Wi-Fi connected. Starting main server...");
  loadSettings();

  IPAddress local_IP, gateway_IP, subnet_IP;
  local_IP.fromString(currentSettings.ip);
  gateway_IP.fromString(currentSettings.gateway);
  subnet_IP.fromString(currentSettings.subnet);

  IPAddress primaryDNS(8, 8, 8, 8);
  IPAddress secondaryDNS(8, 8, 4, 4);

  WiFi.setHostname(currentSettings.hostname.c_str());

  if (!WiFi.config(local_IP, gateway_IP, subnet_IP, primaryDNS, secondaryDNS)) {
    // Serial.println("Failed to configure static IP!");
  }

  // Serial.print("IP Address: ");
  // Serial.println(WiFi.localIP());

  if (MDNS.begin(currentSettings.hostname.c_str())) {
    // Serial.println("mDNS responder started. Hostname: " +
    // currentSettings.hostname);
    MDNS.addService("http", "tcp", 80);
  } else {
    // Serial.println("Error starting mDNS!");
  }

  configTime(currentSettings.gmtOffset, currentSettings.daylightOffset,
             ntpServer);
  // Serial.println("Started NTP time client.");

  // --- Start Alpaca Discovery Service (UDP) ---
  startAlpacaDiscovery();

  // --- Main Server Routes (Only essential routes remain for this module) ---
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/gettime", HTTP_GET, handleGetTime);
  server.on("/slider1", HTTP_GET, handleSlider1);    // Example control route
  server.on("/test/move", HTTP_GET, handleTestMove); // <-- NEW DEBUG ROUTE

  // --- Essential Web UI Routes ---
  server.on("/open1", HTTP_GET, handleOpen1);
  server.on("/close1", HTTP_GET, handleClose1);
  server.on("/getallstatus", HTTP_GET, handleGetAllStatus); // Fixed to match JS
  server.on("/getsettings", HTTP_GET, handleGetSettings); // Added missing route
  server.on("/save", HTTP_POST, handleSave);

  server.onNotFound(handleNotFound);

  server.begin();
  // Serial.println("Main web server started.");
}
