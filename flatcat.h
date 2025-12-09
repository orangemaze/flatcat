// flatcat.h

#ifndef FLATCAT_H
#define FLATCAT_H

// --- Standard Includes ---
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "esp_system.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>


// --- Servo Library ---
#include <ESP32Servo.h>

#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>

// --- HTML FILES ---
#include "page_main.h"
#include "page_reboot.h"
#include "page_settings.h"
#include "page_setup.h"

// ================================================================
// --- GLOBAL OBJECTS & VARIABLES (External Declarations) ---
// ================================================================

// --- HARDWARE PINS ---
extern int elPin_1;
extern int elPin_2;
extern int servoPin_1;
extern int servoPin_2;
extern int factoryResetPin;
extern int closedStopPin_1;
extern int openStopPin_1;

// --- SERVO CONFIG / CONSTANTS ---
#define MIN_MICROS 500
#define MAX_MICROS 2400

extern int openAngle;
extern int closeAngle;
extern const int coverReady;
extern const int coverOpen;
extern const int coverClosed;
extern const int coverMoving;
extern const int calibratorReady;
extern const int calibratorNotReady;
extern const int maxBrightness;

// --- GLOBAL OBJECTS & STATE ---
extern WebServer server;
extern Servo myServo_1;
extern Servo myServo_2;

extern Preferences preferences;
extern DNSServer dnsServer;
extern WiFiUDP udp;
extern const char *ntpServer;

extern String currentTimeString;
extern unsigned long lastTimeUpdate;
extern long serverTransactionID;
extern bool isDimmerActive;
extern int currentServoAngle_1;
extern int currentServoAngle_2;
extern int coverState_1;
extern int currentDimmerValue_1;
extern int currentDimmerValue_2;
extern int calibratorState_1;
extern bool isClosedStopActive_1;
extern bool isOpenStopActive_1;
extern bool isMovingToClose_1;
extern bool isMovingToOpen_1;
extern bool isConnected;
extern unsigned long moveStartTime;
#define MAX_BRIGHTNESS 64
#define SIMULATED_MOVEMENT_TIME 2000
extern const int calibratorOff;

// --- CONFIG STRUCT ---
struct DeviceSettings {
  String hostname;
  String ip;
  String gateway;
  String subnet;
  String title1;
  String title2;
  long gmtOffset;
  int daylightOffset;
};
extern DeviceSettings currentSettings;
extern const char *ap_ssid;

// --- ALPACA DISCOVERY CONSTANTS ---
extern const int ALPACA_DISCOVERY_PORT;
extern const char *ALPACA_DISCOVERY_RESPONSE;
extern String deviceUniqueID;

// ================================================================
// --- FUNCTION PROTOTYPES (Used by all files) ---
// ================================================================

// --- Alpaca Core Functions (alpaca_api.cpp) ---
void startAlpacaDiscovery();
void handleAlpacaDiscovery();
String createAlpacaJSON(long clientID, int errorNum, String errorMsg,
                        const char *valueType, String value);
void handleAlpacaAPI();
void handleAlpacaAPIVersions(long clientID);
void handleAlpacaDescription(long clientID);
void handleAlpacaConfiguredDevices(long clientID);
void handleAlpacaSupportedDevices();
void handleAlpacaCoverCalibrator(long clientID, long transactionID,
                                 int deviceNum);

// --- Custom Web Server/Hardware Functions (web_handlers.cpp) ---
void handleRoot();
void handleSettings();
void handleGetTime();
void handleSlider1();
void handleSlider2();
void handleOpen1();
void handleClose1();
void handleOpen2();
void handleClose2();
void handleGetAllStatus();
void handleGetSettings();
void handleSave();
void handleScan();
void handleSaveWifi();
void handleNotFound();

// --- Utility Functions (config_utils.cpp / flatcat.ino) ---
void updateDimmerLock();
void startApMode();
void loadSettings();
void startMainServer();
void updateCoverStatus();
void checkAndStopServo();
int validateDeviceNumber(String uri);
void initializeUniqueID();
bool isNumeric(String str);
void setDimmerValue(int brightness);

#endif // FLATCAT_H
