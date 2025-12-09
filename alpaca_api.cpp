// alpaca_api.cpp

#include "flatcat.h"

// ================================================================
// --- ALPACA DISCOVERY IMPLEMENTATION ---
// ================================================================

void startAlpacaDiscovery() {
  if (udp.begin(ALPACA_DISCOVERY_PORT)) {
    Serial.printf("Alpaca Discovery listening on UDP port %d\n",
                  ALPACA_DISCOVERY_PORT);
  } else {
    Serial.println("Failed to start Alpaca Discovery UDP listener.");
  }
}

void handleAlpacaDiscovery() {
  int packetSize = udp.parsePacket();

  if (packetSize) {
    char incomingPacket[255];
    int len = udp.read(incomingPacket, 255);
    incomingPacket[len] = 0;

    // Check for 'alpacadiscovery1'
    if (strstr(incomingPacket, "alpacadiscovery1") != NULL) {
      Serial.println("Alpaca Discovery Request received. Sending response.");

      IPAddress remoteIP = udp.remoteIP();
      int remotePort = udp.remotePort();

      udp.beginPacket(remoteIP, remotePort);
      udp.write((uint8_t *)ALPACA_DISCOVERY_RESPONSE,
                strlen(ALPACA_DISCOVERY_RESPONSE));
      udp.endPacket();

      Serial.println("Response sent.");
    }
  }
}

// ================================================================
// --- ALPACA JSON UTILITY ---
// ================================================================

// Generates a standard Alpaca JSON response
String createAlpacaJSON(long clientTransactionIDToEcho, int errorNum,
                        String errorMsg, const char *valueType, String value) {
  StaticJsonDocument<512> doc;

  doc["ClientTransactionID"] = clientTransactionIDToEcho;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = errorNum;
  doc["ErrorMessage"] = errorMsg;

  if (errorNum == 0) {
    if (valueType == "string") {
      doc["Value"] = value;
    } else if (valueType == "bool") {
      doc["Value"] =
          (value.equalsIgnoreCase("true") || value.equalsIgnoreCase("1"));
    } else if (valueType == "int") {
      doc["Value"] = value.toInt();
    } else if (valueType == "double") {
      doc["Value"] = value.toDouble();
    } else {
      doc["Value"] = value; // Default to string if type is unknown
    }
  }

  String json;
  serializeJson(doc, json);
  return json;
}

// ================================================================
// --- ALPACA MANAGEMENT API HANDLERS ---
// ================================================================

// Implements /management/v1/apiversions
void handleAlpacaAPIVersions(long clientID) {
  StaticJsonDocument<128> doc;
  doc["ClientTransactionID"] = clientID;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = 0;
  doc["ErrorMessage"] = "";

  JsonArray versions = doc.createNestedArray("Value");
  versions.add(1);

  String json;
  serializeJson(doc, json);

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "application/json", json.c_str());
}

// Implements /management/v1/description
void handleAlpacaDescription(long clientID) {
  StaticJsonDocument<256> doc;
  doc["ClientTransactionID"] = clientID;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = 0;
  doc["ErrorMessage"] = "";

  // Value: A single JSON Object {}
  JsonObject value = doc.createNestedObject("Value");

  value["ServerName"] = currentSettings.hostname;
  value["Manufacturer"] = "orangemaze";
  value["ManufacturerVersion"] = "1.0.0"; // Corrected for compliance
  value["Version"] = "1.0.0";
  value["Location"] = "The Observatory";

  String json;
  serializeJson(doc, json);

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "application/json", json.c_str());
}

// Implements /management/v1/configureddevices
void handleAlpacaConfiguredDevices(long clientID) {
  StaticJsonDocument<512> doc;
  doc["ClientTransactionID"] = clientID;
  doc["ServerTransactionID"] = serverTransactionID++;
  doc["ErrorNumber"] = 0;
  doc["ErrorMessage"] = "";

  // Value: An Array of JSON Objects []
  JsonArray confarray = doc.createNestedArray("Value");

  // 'dev' is defined here
  JsonObject dev = confarray.createNestedObject();

  dev["DeviceName"] = currentSettings.title1;
  dev["DeviceType"] = "CoverCalibrator";
  dev["DeviceNumber"] = 0;

  // FIX: Using the global UniqueID (which you were setting to "1000" before)
  dev["UniqueID"] = deviceUniqueID;

  String json;
  serializeJson(doc, json);

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "application/json", json.c_str());
}

// Implements /management/v1/supporteddevices (Uses configureddevices logic)
void handleAlpacaSupportedDevices() { handleAlpacaConfiguredDevices(0); }

// ================================================================
// --- ALPACA DEVICE API HANDLERS ---
// ================================================================

// Implements the specific methods for the CoverCalibrator device
void handleAlpacaCoverCalibrator(long clientID, long transactionID,
                                 int deviceNum) {
  String uri = server.uri();
  String method = server.method() == HTTP_GET ? "GET" : "PUT";
  int start = uri.indexOf("covercalibrator/");
  if (start != -1) {
    start += strlen("covercalibrator/");
    int end = uri.indexOf('/', start);
    if (end == -1)
      end = uri.length();

    String devNumStr = uri.substring(start, end);

    // --- NEW VALIDATION FIX ---
    // 1. Check if the string contains non-numeric characters (like 'A')
    if (!isNumeric(devNumStr)) {
      String errorMsg = "Invalid device number format. Must be numeric.";
      String responseJson =
          createAlpacaJSON(transactionID, 0x100, errorMsg, "", "");
      server.send(400, "application/json", responseJson.c_str());
      return; // EXIT immediately for non-numeric input
    }

    // 2. Now safely convert to int
    int requestedDevNum = devNumStr.toInt();

    // 3. Check if the number is the supported device (Device 0)
    if (requestedDevNum != 0) {
      String errorMsg = "Invalid device number: " + devNumStr +
                        ". Only device 0 is configured.";
      String responseJson =
          createAlpacaJSON(transactionID, 0x100, errorMsg, "", "");
      server.send(400, "application/json", responseJson.c_str());
      return; // EXIT immediately for non-zero numbers
    }
  }

  // --- 1. Handle 'connected' ---
  if (uri.indexOf("connected") != -1) {
    String responseJson =
        createAlpacaJSON(transactionID, 0, "", "bool", "true");
    if (method == "GET") {
      server.send(200, "application/json", responseJson.c_str());
    } else { // PUT
      server.send(200, "application/json", responseJson.c_str());
    }
    return;
  }

  // --- 2. Handle 'coverstate' Property (GET only) ---
  if (uri.indexOf("coverstate") != -1) {
    if (method == "GET") {
      String stateValue = String(coverState_1);

      // CRITICAL FIX: Ensure transactionID is used to echo the CTID
      String responseJson =
          createAlpacaJSON(transactionID, 0, "", "int", stateValue);

      server.send(200, "application/json", responseJson.c_str());
    } else {
      // Reject PUT request
      server.send(400, "text/plain",
                  "Error: CoverState is a GET-only property.");
    }
    return;
  }
  // --- 3. Handle 'calibratorstate' Property (GET only) ---
  if (uri.indexOf("calibratorstate") != -1) {
    if (method == "GET") {
      // Logic for GET (correctly returning the integer value)
      String stateValue = String(calibratorState_1);
      String responseJson =
          createAlpacaJSON(transactionID, 0, "", "int", stateValue);
      server.send(200, "application/json", responseJson.c_str());
    } else {
      // FIX: Explicitly handle and reject the unsupported PUT method.
      String errorMsg = "CalibratorState is a GET-only property.";

      // Use 0x403 (1027) InvalidOperationException and 400 Bad Request status.
      String responseJson =
          createAlpacaJSON(transactionID, 0x403, errorMsg, "", "");
      server.send(400, "application/json", responseJson.c_str());
    }
    return;
  }
  if (uri.indexOf("brightness") != -1) {
    if (method == "GET") {
      // currentDimmerValue_1 is the global variable (0 to MaxBrightness)
      String value = String(currentDimmerValue_1);
      String responseJson =
          createAlpacaJSON(transactionID, 0, "", "int", value);
      server.send(200, "application/json", responseJson.c_str());
    } else {
      // Brightness is GET-only; the PUT command is handled by CalibratorOn
      String errorMsg = "Brightness is a GET-only property.";
      String responseJson =
          createAlpacaJSON(transactionID, 0x403, errorMsg, "", "");
      server.send(400, "application/json", responseJson.c_str());
    }
    return;
  }
  // --- 4. Handle 'maxbrightness' Property (GET only) ---
  if (uri.indexOf("maxbrightness") != -1) {
    if (method == "GET") {
      String responseJson =
          createAlpacaJSON(clientID, 0, "", "int", String(maxBrightness));
      server.send(200, "application/json", responseJson.c_str());
    } else {
      server.send(400, "text/plain",
                  "Error: MaxBrightness is a GET-only property.");
    }
    return;
  }

  // --- 5. Handle 'interfaceversion' Property (GET only) ---
  if (uri.indexOf("interfaceversion") != -1) {
    if (method == "GET") {
      String responseJson = createAlpacaJSON(transactionID, 0, "", "int", "2");
      server.send(200, "application/json", responseJson.c_str());
    } else {
      server.send(400, "text/plain",
                  "Error: InterfaceVersion is a GET-only property.");
    }
    return;
  }

  // --- 6. Handle 'description' Property (GET only) ---
  if (uri.indexOf("description") != -1) {
    if (method == "GET") {
      String responseJson = createAlpacaJSON(transactionID, 0, "", "string",
                                             currentSettings.title1);
      server.send(200, "application/json", responseJson.c_str());
    } else {
      server.send(400, "text/plain",
                  "Error: Description is a GET-only property.");
    }
    return;
  }

  // --- NEW: SupportedStrings / Capabilities to enable NINA Buttons ---

  // supportedactions
  if (uri.indexOf("supportedactions") != -1) {
    if (method == "GET") {
      StaticJsonDocument<200> doc;
      doc["ClientTransactionID"] = transactionID;
      doc["ServerTransactionID"] = serverTransactionID++;
      doc["ErrorNumber"] = 0;
      doc["ErrorMessage"] = "";
      JsonArray val = doc.createNestedArray("Value");
      String output;
      serializeJson(doc, output);
      server.send(200, "application/json", output);
    } else {
      server.send(400, "text/plain", "GET only");
    }
    return;
  }
  // canopen
  if (uri.indexOf("canopen") != -1) {
    if (method == "GET") {
      server.send(
          200, "application/json",
          createAlpacaJSON(transactionID, 0, "", "bool", "true").c_str());
    } else {
      server.send(400, "text/plain", "GET only");
    }
    return;
  }
  // canclose
  if (uri.indexOf("canclose") != -1) {
    if (method == "GET") {
      server.send(
          200, "application/json",
          createAlpacaJSON(transactionID, 0, "", "bool", "true").c_str());
    } else {
      server.send(400, "text/plain", "GET only");
    }
    return;
  }
  // canhalt
  if (uri.indexOf("canhalt") != -1) {
    if (method == "GET") {
      server.send(
          200, "application/json",
          createAlpacaJSON(transactionID, 0, "", "bool", "true").c_str());
    } else {
      server.send(400, "text/plain", "GET only");
    }
    return;
  }
  // covermoving
  if (uri.indexOf("covermoving") != -1) {
    if (method == "GET") {
      String isMoving =
          (coverState_1 == 3) ? "true" : "false"; // 3 = coverMoving
      server.send(
          200, "application/json",
          createAlpacaJSON(transactionID, 0, "", "bool", isMoving).c_str());
    } else {
      server.send(400, "text/plain", "GET only");
    }
    return;
  }
  // calibratorchanging
  if (uri.indexOf("calibratorchanging") != -1) {
    if (method == "GET") {
      server.send(
          200, "application/json",
          createAlpacaJSON(transactionID, 0, "", "bool", "false").c_str());
    } else {
      server.send(400, "text/plain", "GET only");
    }
    return;
  }

  // --- 7. Handle 'CalibratorOn' Method (PUT) ---
  if (uri.indexOf("calibratoron") != -1) {
    if (method == "PUT") {
      if (server.hasArg("Brightness")) {
        int brightness = server.arg("Brightness").toInt();
        // Check Lock (logic retained from our previous work)
        if (calibratorState_1 == calibratorNotReady) {
          String errorMsg =
              "Calibrator is NotReady (Cover is closed or moving).";
          String responseJson =
              createAlpacaJSON(clientID, 0x401, errorMsg, "", "");
          server.send(403, "application/json", responseJson.c_str());
          return;
        }
        setDimmerValue(brightness);

        String responseJson = createAlpacaJSON(clientID, 0, "", "", "");
        server.send(200, "application/json", responseJson.c_str());
      } else {
        server.send(400, "text/plain",
                    "Missing Brightness parameter for CalibratorOn.");
      }
    } else {
      server.send(400, "text/plain", "CalibratorOn must be a PUT request.");
    }
    return;
  }

  // --- 8. Handle 'CalibratorOff' Method (PUT) ---
  if (uri.indexOf("calibratoroff") != -1) {
    if (method == "PUT") {
      analogWrite(elPin_1, 0);
      currentDimmerValue_1 = 0;
      isDimmerActive = false;

      String responseJson = createAlpacaJSON(clientID, 0, "", "", "");
      server.send(200, "application/json", responseJson.c_str());
    } else {
      server.send(400, "text/plain", "CalibratorOff must be a PUT request.");
    }
    return;
  }
  // --- 9. Handle 'devicestate' Method (PUT) ---
  if (uri.indexOf("devicestate") != -1) {
    if (method == "GET") {
      // --- 1. Allocate JSON Document ---
      // Allocate a size large enough for the response array (512 is usually
      // safe)
      StaticJsonDocument<512> doc;

      // --- 2. Build the Mandatory ASCOM Header ---
      doc["ClientTransactionID"] = transactionID;
      doc["ServerTransactionID"] = serverTransactionID++;
      doc["ErrorNumber"] = 0;
      doc["ErrorMessage"] = "";

      // --- 3. Create the JSON Array for "Value" ---
      JsonArray valueArray = doc.createNestedArray("Value");

      // --- 4. Populate the Array with Name/Value Pairs ---
      // Item 1: Connection Status
      JsonObject item1 = valueArray.createNestedObject();
      item1["Name"] = "Connected";
      item1["Value"] =
          true; // Hardcoded true as we are connected to reach this point

      // Item 2: Cover Position Status (0=Ready, 1=Open, 2=Closed, 3=Moving)
      JsonObject item2 = valueArray.createNestedObject();
      item2["Name"] = "CoverState";
      item2["Value"] = coverState_1; // Global variable updated by sensor logic

      // Item 3: Calibrator/Dimmer Status (0=Ready/ON/OFF, 1=NotReady)
      JsonObject item3 = valueArray.createNestedObject();
      item3["Name"] = "CalibratorState";
      item3["Value"] =
          calibratorState_1; // Global variable updated by dimmer lock logic

      // Item 4: Current Dimmer Brightness (0 to MaxBrightness)
      JsonObject item4 = valueArray.createNestedObject();
      item4["Name"] = "Brightness";
      item4["Value"] = currentDimmerValue_1;

      // --- 5. Serialize and Send Response ---
      String responseJson;
      serializeJson(doc, responseJson);

      server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      server.send(200, "application/json", responseJson.c_str());
    } else {
      // PUT requests are not supported for DeviceState
      server.send(400, "text/plain",
                  "Error: DeviceState is a GET-only property.");
    }
    return;
  }
  // --- 10. Handle 'DriverInfo' Method (PUT) ---
  if (uri.indexOf("driverinfo") != -1) {
    if (method == "GET") {
      // Build the informational string
      String info = "FlatCat CoverCalibrator V2.0 |";
      info += "ASCOM Alpaca Driver by Orangemaze |";
      info += "Compiled: " __DATE__ " " __TIME__
              " |"; // Use compiler macros for date/time
      info += "Hardware: Flat Panel/Lens Cap";

      // Return the string value
      String responseJson =
          createAlpacaJSON(transactionID, 0, "", "string", info);
      server.send(200, "application/json", responseJson.c_str());
    } else {
      // FIX: Reject PUT requests with the correct 400 Bad Request status.
      String errorMsg = "DriverInfo is a GET-only property.";
      String responseJson =
          createAlpacaJSON(transactionID, 0x403, errorMsg, "",
                           ""); // 0x403 InvalidOperationException
      server.send(400, "application/json", responseJson.c_str());
    }
    return;
  }
  if (uri.indexOf("driverversion") != -1) {
    if (method == "GET") {
      // Return the version string for the driver.
      String value = "2.0.0"; // Use your current driver version string
      String responseJson =
          createAlpacaJSON(transactionID, 0, "", "string", value);
      server.send(200, "application/json", responseJson.c_str());
    } else {
      // DriverVersion is GET-only
      String errorMsg = "DriverVersion is a GET-only property.";
      String responseJson =
          createAlpacaJSON(transactionID, 0x403, errorMsg, "", "");
      server.send(400, "application/json", responseJson.c_str());
    }
    return;
  }
  if (uri.indexOf("opencover") != -1) {
    if (method == "PUT") {
      // Error Check 1: Prevent operation if dimmer is ON
      if (isDimmerActive) {
        String errorMsg = "Cannot open cover: Calibrator is currently ON.";
        // 0x401 is ASCOM InvalidOperationException
        String responseJson =
            createAlpacaJSON(transactionID, 0x401, errorMsg, "", "");
        server.send(403, "application/json", responseJson.c_str());
        return;
      }

      // Error Check 2: Check if already open (using the sensor flag)
      if (isOpenStopActive_1) {
        String errorMsg = "Cover is already in the Open position.";
        String responseJson =
            createAlpacaJSON(transactionID, 0, errorMsg, "", "");
        server.send(200, "application/json", responseJson.c_str());
        return;
      }

      // --- Execute Device Operation ---
      // These actions initiate the movement: attach servo, set flags, and
      // command the angle.
      myServo_1.attach(servoPin_1);
      isMovingToClose_1 = false;
      isMovingToOpen_1 = true;
      coverState_1 = coverMoving;
      myServo_1.write(openAngle);
      currentServoAngle_1 = openAngle;

      // ASCOM Success Response (200 OK)
      String responseJson = createAlpacaJSON(transactionID, 0, "", "", "");
      server.send(200, "application/json", responseJson.c_str());
    } else {
      // Reject GET method
      String errorMsg = "OpenCover must be a PUT request.";
      String responseJson =
          createAlpacaJSON(transactionID, 0x403, errorMsg, "", "");
      server.send(400, "application/json", responseJson.c_str());
    }
    return;
  }
  if (uri.indexOf("closecover") != -1) {
    if (method == "PUT") {
      // Error Check 1: Prevent operation if dimmer is ON
      if (isDimmerActive) {
        String errorMsg = "Cannot close cover: Calibrator is currently ON.";
        // 0x401 is ASCOM InvalidOperationException
        String responseJson =
            createAlpacaJSON(transactionID, 0x401, errorMsg, "", "");
        server.send(403, "application/json", responseJson.c_str());
        return;
      }

      // Error Check 2: Check if already closed (using the sensor flag)
      if (isClosedStopActive_1) {
        String errorMsg = "Cover is already in the Closed position.";
        String responseJson =
            createAlpacaJSON(transactionID, 0, errorMsg, "", "");
        server.send(200, "application/json", responseJson.c_str());
        return;
      }

      // --- Execute Device Operation ---
      // These actions initiate the movement: attach servo, set flags, and
      // command the angle.
      myServo_1.attach(servoPin_1);
      isMovingToOpen_1 = false;
      isMovingToClose_1 = true;
      coverState_1 = coverMoving;
      myServo_1.write(closeAngle);
      currentServoAngle_1 = closeAngle;

      // ASCOM Success Response (200 OK)
      String responseJson = createAlpacaJSON(transactionID, 0, "", "", "");
      server.send(200, "application/json", responseJson.c_str());
    } else {
      // Reject GET method
      String errorMsg = "CloseCover must be a PUT request.";
      String responseJson =
          createAlpacaJSON(transactionID, 0x403, errorMsg, "", "");
      server.send(400, "application/json", responseJson.c_str());
    }
    return;
  }
  // --- Fall-through for unimplemented methods/properties (FIX for 501/descrip)
  // --- If the request reached this point, the member name was invalid (like
  // 'descrip') or a valid member was requested with an unsupported method
  // (e.g., PUT on a GET-only property).
  String errorMsg = "Unknown member or unsupported action: " + uri;

  // 0x403 is ASCOM InvalidOperationException
  String responseJson =
      createAlpacaJSON(transactionID, 0x403, errorMsg, "", "");

  // Use 400 Bad Request status code
  server.send(400, "application/json", responseJson.c_str());
}

// Main Alpaca API Router
void handleAlpacaAPI() {
  String uri = server.uri();

  // Logic for ClientID and ClientTransactionID retrieval (Focus on correct
  // casing)
  long clientTransactionID = 0;
  long clientID = 0;

  // 1. Get the ClientTransactionID (Case Sensitive - this is the one we care
  // about)
  // 1. Get the ClientTransactionID
  if (server.hasArg("ClientTransactionID")) {
    String ctidStr = server.arg("ClientTransactionID");
    // Use strtol() for universal compiler compatibility
    long tempID = strtol(ctidStr.c_str(), NULL, 10);
    clientTransactionID = strtol(ctidStr.c_str(), NULL, 10);
    if (tempID > 0) {
      clientTransactionID = tempID;
    }
  }

  // 2. Get the ClientID
  if (server.hasArg("ClientID")) {
    String cidStr = server.arg("ClientID");
    // Use strtol() for universal compiler compatibility
    clientID = strtol(cidStr.c_str(), NULL, 10);
  }
  bool ctidValid = false;
  String ctidStr;

  if (server.hasArg("ClientTransactionID")) {
    ctidStr = server.arg("ClientTransactionID");

    // Check 1: Check for valid content (non-empty, non-zero number)
    if (ctidStr.length() > 0 && ctidStr.toInt() != 0) {
      clientTransactionID = ctidStr.toInt();
      ctidValid = true;
    }
  }

  // --- CRITICAL ERROR CHECK ---
  // The CTID is mandatory for all device calls. If it's not valid, return 400.

  // NOTE: We only require the CTID for Device API calls (CoverCalibrator),
  // but often fail if it's invalid anywhere it's provided.
  if (server.uri().indexOf("/api/v1/") != -1 && !ctidValid) {
    String errorMsg = "Invalid or missing ClientTransactionID.";
    // Pass 0 as the ID since the sent ID is invalid
    String responseJson = createAlpacaJSON(0, 0x100, errorMsg, "", "");
    server.send(400, "application/json", responseJson.c_str());
    return;
  }
  String method = server.method() == HTTP_GET || server.method() == HTTP_HEAD
                      ? "GET"
                      : (server.method() == HTTP_PUT ? "PUT" : "UNSUPPORTED");

  if (method == "UNSUPPORTED") {
    server.send(405, "text/plain",
                "Method not supported. Only GET, HEAD, and PUT are valid.");
    return;
  }

  // --- Alpaca Routing Logic ---
  // Management calls should use the CTID as the clientID
  if (uri.indexOf("/management/v1/apiversions") != -1 ||
      uri.indexOf("/management/apiversions") != -1) {
    handleAlpacaAPIVersions(clientTransactionID);
  } else if (uri.indexOf("/management/v1/description") != -1) {
    handleAlpacaDescription(clientTransactionID);
  } else if (uri.indexOf("/management/v1/configureddevices") != -1) {
    handleAlpacaConfiguredDevices(clientTransactionID);
  } else if (uri.indexOf("/management/v1/supporteddevices") != -1) {
    handleAlpacaSupportedDevices();
  }
  // Device API call
  else if (uri.indexOf("/api/v1/covercalibrator") != -1) {
    // Pass clientID and clientTransactionID correctly
    handleAlpacaCoverCalibrator(clientID, clientTransactionID, 0);
  } else {
    server.send(404, "text/plain", "Invalid ASCOM API endpoint.");
  }
}
