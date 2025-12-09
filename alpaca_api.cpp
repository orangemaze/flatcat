// alpaca_api.cpp

#include "flatcat.h"

void startAlpacaDiscovery() {
  if (udp.begin(ALPACA_DISCOVERY_PORT)) {
    Serial.print("Alpaca Discovery Service listening on UDP port ");
    Serial.println(ALPACA_DISCOVERY_PORT);
  }
}

// Check for and handle incoming UDP packets
void handleAlpacaDiscovery() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    // We received a packet
    char packetBuffer[255];
    int len = udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0; // Null terminate
    }

    String message = String(packetBuffer);

    // Check for "alpaca discovery"
    if (message.indexOf("alpaca discovery") >= 0) {
      // Send response
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.print(ALPACA_DISCOVERY_RESPONSE);
      udp.endPacket();
    }
  }
}

// --- Helper to Create Alpaca JSON Response ---
String createAlpacaJSON(long clientID, int errorNum, String errorMsg,
                        const char *valueType, String value) {
  StaticJsonDocument<1024> doc; // Increased size just in case

  doc["ClientTransactionID"] = clientID;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = errorNum;
  doc["ErrorMessage"] = errorMsg;

  // Only add "Value" if it's not empty, or if we explicitly want to send it
  if (valueType != "") {
    if (String(valueType) == "bool") {
      doc["Value"] = (value == "true");
    } else if (String(valueType) == "int") {
      doc["Value"] = value.toInt();
    } else {
      doc["Value"] = value;
    }
  }

  String output;
  serializeJson(doc, output);
  return output;
}

// --- 1. API Versions ---
void handleAlpacaAPIVersions(long clientID) {
  StaticJsonDocument<200> doc;
  doc["ClientTransactionID"] = clientID;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = 0;
  doc["ErrorMessage"] = "";

  JsonArray values = doc.createNestedArray("Value");
  values.add(1);

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// --- 2. Description ---
void handleAlpacaDescription(long clientID) {
  StaticJsonDocument<512> doc;
  doc["ClientTransactionID"] = clientID;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = 0;
  doc["ErrorMessage"] = "";

  JsonObject val = doc.createNestedObject("Value");
  val["ServerName"] = "FlatCat Server";
  val["Manufacturer"] = "Orangemaze";
  val["ServerVersion"] = "2.0";
  val["Location"] = "Unknown";

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// --- 3. Configured Devices ---
void handleAlpacaConfiguredDevices(long clientID) {
  StaticJsonDocument<512> doc;
  doc["ClientTransactionID"] = clientID;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = 0;
  doc["ErrorMessage"] = "";

  JsonArray val = doc.createNestedArray("Value");

  JsonObject device = val.createNestedObject();
  device["DeviceName"] = currentSettings.title1;
  device["DeviceType"] = "CoverCalibrator";
  device["DeviceNumber"] = 0;
  device["UniqueID"] = deviceUniqueID;

  String output;
  serializeJson(doc, output);
  server.send(200, "application/json", output);
}

// --- 4. Supported Devices ---
void handleAlpacaSupportedDevices() {
  server.send(200, "text/plain", "CoverCalibrator");
}

// ================================================================
// --- COVER CALIBRATOR API HANDLER ---
// ================================================================

void handleAlpacaCoverCalibrator(long clientID, long transactionID,
                                 int deviceNum) {
  String uri = server.uri();
  uri.toLowerCase();

  String method = (server.method() == HTTP_GET)   ? "GET"
                  : (server.method() == HTTP_PUT) ? "PUT"
                                                  : "OTHER";

  // --- 1. Handle 'name' Property ---
  if (uri.indexOf("name") != -1 && uri.indexOf("name/") == -1) {
    if (method == "GET") {
      String responseJson = createAlpacaJSON(transactionID, 0, "", "string",
                                             currentSettings.title1);
      server.send(200, "application/json", responseJson.c_str());
    } else {
      server.send(400, "text/plain", "Values are GET only");
    }
    return;
  }

  // --- 2. Handle 'connected' Property ---
  if (uri.indexOf("connected") != -1) {
    if (method == "GET") {
      String responseJson = createAlpacaJSON(transactionID, 0, "", "bool",
                                             isConnected ? "true" : "false");
      server.send(200, "application/json", responseJson.c_str());
    } else if (method == "PUT") {
      if (server.hasArg("Connected")) {
        String val = server.arg("Connected");
        if (val == "true" || val == "True")
          isConnected = true;
        else
          isConnected = false;

        String responseJson = createAlpacaJSON(transactionID, 0, "", "", "");
        server.send(200, "application/json", responseJson.c_str());
      } else {
        server.send(400, "text/plain", "Missing Connected arguments");
      }
    }
    return;
  }

  // --- 3. Handle 'coverstate' Property (GET only) ---
  if (uri.indexOf("coverstate") != -1) {
    if (method == "GET") {
      String responseJson =
          createAlpacaJSON(transactionID, 0, "", "int", String(coverState_1));
      server.send(200, "application/json", responseJson.c_str());
    } else {
      server.send(400, "text/plain", "CoverState is GET only");
    }
    return;
  }

  // --- 4. Handle 'calibratorstate' Property (GET only) ---
  if (uri.indexOf("calibratorstate") != -1) {
    if (method == "GET") {
      String responseJson = createAlpacaJSON(transactionID, 0, "", "int",
                                             String(calibratorState_1));
      server.send(200, "application/json", responseJson.c_str());
    } else {
      server.send(400, "text/plain", "CalibratorState is GET only");
    }
    return;
  }

  // --- 5. Handle 'brightness' Property (GET only) ---
  if (uri.indexOf("brightness") != -1) {
    if (method == "GET") {
      String responseJson = createAlpacaJSON(transactionID, 0, "", "int",
                                             String(currentDimmerValue_1));
      server.send(200, "application/json", responseJson.c_str());
    } else {
      server.send(400, "text/plain", "Brightness is GET only");
    }
    return;
  }

  // --- 6. Handle 'maxbrightness' Property (GET only) ---
  if (uri.indexOf("maxbrightness") != -1) {
    if (method == "GET") {
      String responseJson =
          createAlpacaJSON(transactionID, 0, "", "int", String(MAX_BRIGHTNESS));
      server.send(200, "application/json", responseJson.c_str());
    } else {
      server.send(400, "text/plain", "MaxBrightness is GET only");
    }
    return;
  }

  // --- ACTIONS ---

  // --- 7. CalibratorOn (PUT) ---
  if (uri.indexOf("calibratoron") != -1) {
    if (method == "PUT") {
      if (server.hasArg("Brightness")) {
        int b = server.arg("Brightness").toInt();
        if (coverState_1 != coverClosed) {
          String responseJson = createAlpacaJSON(transactionID, 0x401,
                                                 "Cover not closed", "", "");
          server.send(200, "application/json", responseJson.c_str());
          return;
        }
        setDimmerValue(b);
        String responseJson = createAlpacaJSON(transactionID, 0, "", "", "");
        server.send(200, "application/json", responseJson.c_str());
      } else {
        server.send(400, "text/plain", "Missing Brightness");
      }
    }
    return;
  }

  // --- 8. CalibratorOff (PUT) ---
  if (uri.indexOf("calibratoroff") != -1) {
    if (method == "PUT") {
      setDimmerValue(0);
      String responseJson = createAlpacaJSON(transactionID, 0, "", "", "");
      server.send(200, "application/json", responseJson.c_str());
    }
    return;
  }

  // --- 9. OpenCover (PUT) ---
  if (uri.indexOf("opencover") != -1) {
    if (method == "PUT") {
      if (isDimmerActive) {
        String responseJson =
            createAlpacaJSON(transactionID, 0x401, "Calibrator is On", "", "");
        server.send(200, "application/json", responseJson.c_str());
        return;
      }
      if (isOpenStopActive_1) {
        String responseJson =
            createAlpacaJSON(transactionID, 0, "Already open", "", "");
        server.send(200, "application/json", responseJson.c_str());
        return;
      }

      Serial.println("Excuting OpenCover...");
      myServo_1.attach(servoPin_1, MIN_MICROS, MAX_MICROS);
      isMovingToClose_1 = false;
      isMovingToOpen_1 = true;
      coverState_1 = coverMoving;
      myServo_1.write(openAngle);
      currentServoAngle_1 = openAngle;
      moveStartTime = millis();

      String responseJson = createAlpacaJSON(transactionID, 0, "", "", "");
      server.send(200, "application/json", responseJson.c_str());
    }
    return;
  }

  // --- 10. CloseCover (PUT) ---
  if (uri.indexOf("closecover") != -1) {
    if (method == "PUT") {
      if (isDimmerActive) {
        String responseJson =
            createAlpacaJSON(transactionID, 0x401, "Calibrator is On", "", "");
        server.send(200, "application/json", responseJson.c_str());
        return;
      }
      if (isClosedStopActive_1) {
        String responseJson =
            createAlpacaJSON(transactionID, 0, "Already closed", "", "");
        server.send(200, "application/json", responseJson.c_str());
        return;
      }

      Serial.println("Excuting CloseCover...");
      myServo_1.attach(servoPin_1, MIN_MICROS, MAX_MICROS);
      isMovingToOpen_1 = false;
      isMovingToClose_1 = true;
      coverState_1 = coverMoving;
      myServo_1.write(closeAngle);
      currentServoAngle_1 = closeAngle;
      moveStartTime = millis();

      String responseJson = createAlpacaJSON(transactionID, 0, "", "", "");
      server.send(200, "application/json", responseJson.c_str());
    }
    return;
  }

  // --- 11. HaltCover (PUT) ---
  if (uri.indexOf("haltcover") != -1) {
    if (method == "PUT") {
      Serial.println("Halting Cover...");
      myServo_1.detach();
      isMovingToOpen_1 = false;
      isMovingToClose_1 = false;
      if (coverState_1 == coverMoving)
        coverState_1 = coverReady;

      String responseJson = createAlpacaJSON(transactionID, 0, "", "", "");
      server.send(200, "application/json", responseJson.c_str());
    }
    return;
  }

  server.send(400, "text/plain", "Method not found or supported");
}

void handleAlpacaAPI() {
  long clientID = 0;
  long clientTransactionID = 0;
  bool ctidFound = false;

  if (server.hasArg("ClientID"))
    clientID = server.arg("ClientID").toInt();
  if (server.hasArg("ClientTransactionID")) {
    clientTransactionID = server.arg("ClientTransactionID").toInt();
    ctidFound = true;
  }

  String uri = server.uri();
  uri.toLowerCase();

  if (uri.indexOf("/management/v1/apiversions") != -1 ||
      uri.indexOf("/management/apiversions") != -1) {
    handleAlpacaAPIVersions(clientTransactionID);
    return;
  }
  if (uri.indexOf("/management/v1/description") != -1) {
    handleAlpacaDescription(clientTransactionID);
    return;
  }
  if (uri.indexOf("/management/v1/configureddevices") != -1) {
    handleAlpacaConfiguredDevices(clientTransactionID);
    return;
  }

  // Device Routing
  if (uri.indexOf("/api/v1/covercalibrator") != -1) {
    if (!ctidFound) {
      String response =
          createAlpacaJSON(0, 0x400, "Missing ClientTransactionID", "", "");
      server.send(400, "application/json", response);
      return;
    }
    handleAlpacaCoverCalibrator(clientID, clientTransactionID, 0);
    return;
  }

  server.send(404, "text/plain", "Not Found");
}
