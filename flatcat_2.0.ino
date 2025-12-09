// flatcat_2.0.ino

#include "flatcat.h"

// ================================================================
// --- GLOBAL OBJECT DEFINITIONS ---
// ================================================================
WebServer server(80);
Servo myServo_1;
Servo myServo_2;

Preferences preferences;
DNSServer dnsServer;
WiFiUDP udp;

// --- HARDWARE PINS ---
int elPin_1 = D8;
int elPin_2 = D6;
int servoPin_1 = D3;
int servoPin_2 = D7;
int factoryResetPin = D0;
int closedStopPin_1 = D1;
int openStopPin_1 = D2;

// --- SERVO CONFIG / CONSTANTS ---
// (Moved to header)

int openAngle = 90;
int closeAngle = 0;

const int coverReady = 0;
const int coverClosed = 1;
const int coverMoving = 2;
const int coverOpen = 3;

const int calibratorReady = 0;
const int calibratorNotReady = 1;
const int calibratorOff = 1;

// --- STATE VARIABLES ---
long serverTransactionID = 1;
int currentServoAngle_1 = closeAngle;
int currentServoAngle_2 = closeAngle;
int coverState_1 = coverClosed;
int currentDimmerValue_1 = 0;
int currentDimmerValue_2 = 0;
bool isDimmerActive = false;
int calibratorState_1 = calibratorNotReady;
bool isClosedStopActive_1 = false;
bool isOpenStopActive_1 = false;
bool isMovingToClose_1 = false;
bool isMovingToOpen_1 = false;
bool isConnected = false;
unsigned long moveStartTime = 0;

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
  delay(100);
  Serial.println("tada");

  // --- 1. Factory Reset Check ---
  pinMode(factoryResetPin, INPUT_PULLUP);
  delay(50);
  if (digitalRead(factoryResetPin) == LOW) {
    Serial.println("Reset button detected! Hold for 3s to erase...");
    delay(3000);
    if (digitalRead(factoryResetPin) == LOW) {
      Serial.println("ERASING ALL SAVED SETTINGS!");
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

  // --- SETUP SERVO (Standard) ---
  myServo_1.setPeriodHertz(50);
  myServo_2.setPeriodHertz(50);

  // --- DEBUG STAGE 1: PRE-WIFI ---
  Serial.println("--- TEST 1: PRE-WIFI ---");
  myServo_1.attach(servoPin_1, MIN_MICROS, MAX_MICROS);
  myServo_1.write(90);
  delay(1000);
  myServo_1.write(0);
  delay(1000);
  myServo_1.detach();
  Serial.println("Test 1 Complete.");

  // --- 2. Initial Data and Boot Message ---
  delay(1000);
  Serial.println("Flatcat booting up...");

  // --- 3. Initialize Hardware ---
  pinMode(elPin_1, OUTPUT);
  pinMode(elPin_2, OUTPUT);
  // analogWrite(elPin_1, 0); // Keep disabled for now
  // analogWrite(elPin_2, 0);

  // Initialize Sensor Pins
  pinMode(closedStopPin_1, INPUT_PULLUP);
  pinMode(openStopPin_1, INPUT_PULLUP);

  // --- 4. Load Credentials and Connect WiFi ---
  preferences.begin("flatcat-wifi", true);
  String saved_ssid = preferences.getString("wifi-ssid", "");
  String saved_pass = preferences.getString("wifi-pass", "");
  preferences.end();

  if (saved_ssid == "") {
    startApMode();
  } else {
    Serial.println("Found credentials for: " + saved_ssid);
    Serial.println("Starting WiFi...");
    WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

    // --- DEBUG STAGE 2: IMMEDIATE POST-WIFI START ---
    Serial.println("--- TEST 2: POST-WIFI-BEGIN ---");
    myServo_1.attach(servoPin_1, MIN_MICROS, MAX_MICROS);
    myServo_1.write(90);
    delay(1000);
    myServo_1.write(0);
    delay(1000);
    myServo_1.detach();
    Serial.println("Test 2 Complete.");

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 30000)) {
      Serial.print(".");
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      startMainServer();

      // --- DEBUG STAGE 3: POST-SERVER ---
      Serial.println("--- TEST 3: POST-SERVER ---");
      myServo_1.attach(servoPin_1, MIN_MICROS, MAX_MICROS);
      myServo_1.write(90);
      delay(1000);
      myServo_1.write(0);
      delay(1000);
      myServo_1.detach();
      Serial.println("Test 3 Complete.");

    } else {
      Serial.println("\nFailed to connect. Switch to AP.");
      preferences.begin("flatcat-wifi", false);
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
  // 1. Process standard web clients
  server.handleClient();

  // 2. Process background discovery
  handleAlpacaDiscovery();

  // If in AP mode, process DNS requests
  if (WiFi.status() != WL_CONNECTED) {
    dnsServer.processNextRequest();
  }

  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 5000) {
    lastHeartbeat = millis();
    // Serial.println("Heartbeat: Alive");

    // Wiggle
    Serial.println("Heartbeat Wiggle...");
    myServo_1.attach(servoPin_1, MIN_MICROS, MAX_MICROS);
    myServo_1.write(90);
    delay(200);
    myServo_1.write(0);
    delay(200);
    myServo_1.detach();
  }

  // 3. Update physical cover status
  updateCoverStatus();
  checkAndStopServo();

  delay(1);
}
