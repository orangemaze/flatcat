// flatcat.ino (Main File)

#include "flatcat.h"

// ================================================================
// --- GLOBAL OBJECT DEFINITIONS ---
// ================================================================
WebServer server(80);
Servo myServo_1;
// Servo myServo_2;
Preferences preferences;
DNSServer dnsServer;
WiFiUDP udp;

// --- HARDWARE PINS ---
int elPin_1 = D8;
// int elPin_2 = D6;
int servoPin_1 = D9;
// int servoPin_2 = D7;
int factoryResetPin = D0;
int closedStopPin_1 = D1; // <-- DEFINITION for Closed Sensor
int openStopPin_1 = D2;   // <-- DEFINITION for Open Sensor

// --- SERVO CONFIG / CONSTANTS ---
int openAngle = 90;
int closeAngle = 0;

const int coverReady =
    4; // Changed from 0 (NotPresent) to 4 (Unknown) to keep controls enabled
const int coverClosed = 1; // ASCOM standard for Closed
const int coverMoving = 2; // ASCOM standard for Moving
const int coverOpen = 3;   // ASCOM standard for Open

const int calibratorReady = 0;
const int calibratorNotReady = 1;
const int maxBrightness = 64;

// --- STATE VARIABLES ---
long serverTransactionID = 1;
int currentServoAngle_1 = closeAngle;
// int currentServoAngle_2 = closeAngle;
int coverState_1 = coverClosed;
int currentDimmerValue_1 = 0;
// int currentDimmerValue_2 = 0;
bool isDimmerActive = false;
int calibratorState_1 = calibratorNotReady;
bool isClosedStopActive_1 = false; // <-- DEFINITION: Initial state
bool isOpenStopActive_1 = false;   // <-- DEFINITION: Initial state
bool isMovingToClose_1 = false;    // <-- DEFINITION
bool isMovingToOpen_1 = false;     // <-- DEFINITION

// --- CONFIG & TIME ---
DeviceSettings currentSettings;
const char *ntpServer = "pool.ntp.org";
String currentTimeString = "Syncing...";
unsigned long lastTimeUpdate = 0;
const char *ap_ssid = "flatcat-setup";
String deviceUniqueID = "FLATCAT";

// --- ALPACA DISCOVERY CONSTANTS ---
const int ALPACA_DISCOVERY_PORT = 32227;
const char *ALPACA_DISCOVERY_RESPONSE = "{\"AlpacaPort\": 80}";

// ================================================================
// --- C++ SETUP ---
// ================================================================
void setup() {
  Serial.begin(115200);

  // Wait for Serial to connect (Native USB fix)
  unsigned long startWait = millis();
  while (!Serial && (millis() - startWait < 3000)) {
    delay(10);
  }
  delay(1000); // Extra safety

  Serial.println("tada");

  // --- 1. Factory Reset Check ---
  pinMode(factoryResetPin, INPUT_PULLUP);
  delay(50);
  if (digitalRead(factoryResetPin) == LOW) {
    // ... (Factory reset logic retained)
    Serial.println("Reset button detected! Hold for 3s to erase...");
    delay(3000);
    if (digitalRead(factoryResetPin) == LOW) {
      Serial.println("ERASING ALL SAVED SETTINGS!");
      // These preferences.begin/end calls are correct for the reset logic
      preferences.begin("flatcat-wifi", false);
      preferences.clear();
      preferences.end();
      preferences.begin("flatcat", false);
      preferences.clear();
      preferences.end();
      Serial.println("All settings erased. Rebooting now.");
      delay(2000);
      ESP.restart();
    } else {
      Serial.println("Reset canceled. Continuing normal boot.");
    }
  }

  // --- 2. Initial Data and Boot Message ---
  delay(2000);
  Serial.println("Flatcat booting up...");
  // initializeUniqueID(); // Initialize the persistent ASCOM ID

  // --- 3. Initialize Hardware and Sensors ---
  pinMode(elPin_1, OUTPUT);
  //  pinMode(elPin_2, OUTPUT);
  analogWrite(elPin_1, 0);
  //  analogWrite(elPin_2, 0);

  // PWM and Servo setup
  // ESP32PWM::allocateTimer(0); // Often used by WiFi/System
  ESP32PWM::allocateTimer(1); // Try Timer 1 only
  // ESP32PWM::allocateTimer(2);
  // ESP32PWM::allocateTimer(3);
  myServo_1.attach(servoPin_1);
  // myServo_2.attach(servoPin_2);

  // Initial servo positions and sensor pin setup
  myServo_1.write(currentServoAngle_1);
  // myServo_2.write(currentServoAngle_2);
  myServo_1.detach(); // Stop sending signals
  // Initialize Sensor Pins with Internal Pull-Up (A3213 requirement)

  pinMode(closedStopPin_1, INPUT_PULLUP);
  pinMode(openStopPin_1, INPUT_PULLUP);
  // TIMSK0=0;
  // --- 4. Load Credentials and Connect WiFi ---
  // Read the saved credentials (read-only)
  preferences.begin("flatcat-wifi", true); // TRUE is correct for reading
  String saved_ssid = preferences.getString("wifi-ssid", "");
  String saved_pass = preferences.getString("wifi-pass", "");
  preferences.end(); // Must close the read session

  if (saved_ssid == "") {
    startApMode();
  } else {
    Serial.println("Found credentials for: " + saved_ssid);
    WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 30000)) {
      Serial.print(".");
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      startMainServer();
    } else {
      Serial.println(
          "\nFailed to connect. Wiping credentials and starting AP mode.");
      // Wiping credentials (must be read/write mode)
      preferences.begin("flatcat-wifi", false); // FALSE is correct for writing
      preferences.clear();
      preferences.end();
      startApMode();
    }
  }
}

// ================================================================
// --- C++ LOOP ---
// ================================================================
void loop() {
  // 1. Process standard web clients (CRITICAL for TCP connections)
  server.handleClient();

  // 2. Process background discovery (Non-blocking UDP)
  handleAlpacaDiscovery();

  // If in AP mode, process DNS requests
  if (WiFi.status() != WL_CONNECTED) {
    dnsServer.processNextRequest();
  }

  // 3. NEW: Check and update the physical cover status
  updateCoverStatus(); // <-- NEW CALL
  checkAndStopServo();
  // --- NON-BLOCKING TIME UPDATE ---
  if (millis() - lastTimeUpdate > 1000) {
    lastTimeUpdate = millis();

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo) || timeinfo.tm_year < 100) {
      currentTimeString = "Syncing...";
    } else {
      char timeString[20];
      strftime(timeString, sizeof(timeString), "%I:%M:%S %p", &timeinfo);
      currentTimeString = timeString;
    }
  }
  delay(1);
}
