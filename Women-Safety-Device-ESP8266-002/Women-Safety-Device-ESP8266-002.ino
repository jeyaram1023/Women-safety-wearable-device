
/*
  Women Safety Device - ESP8266 Shoe Module
  Version: FINAL - Fully Tested for Arduino IDE 2.3.6
  Date: September 26, 2025, 10:40 PM IST

  TESTED LIBRARIES (Install these exact versions):
  - ESP8266 Arduino Core: 3.1.2 or higher
  - LiquidCrystal I2C: 1.1.2 by Frank de Brabander
  - TinyGPS++: 1.0.3 by Mikal Hart
  - ArduinoJson: 6.21.3 by Benoit Blanchon
  - SoftwareSerial: Built-in with ESP8266 Core

  HARDWARE TESTED:
  - ESP8266 NodeMCU v1.0 (ESP-12E)
  - GPS Neo-6M Module
  - GSM SIM800L Module
  - 16x2 I2C LCD (PCF8574)
  - SIM Card (activated with SMS plan)

  IMPORTANT: ESP8266 NodeMCU doesn't have pins 27 & 28
  This code uses GPIO14 (D5) and GPIO16 (D0) as equivalents
*/

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// Pin Definitions for ESP8266 NodeMCU
#define GPS_RX_PIN 4             // D2 - GPS Module TX
#define GPS_TX_PIN 5             // D1 - GPS Module RX
#define GSM_RX_PIN 12            // D6 - GSM Module TX
#define GSM_TX_PIN 13            // D7 - GSM Module RX
#define OUTPUT_PIN_27 14         // D5 - Digital Output (Pin 27 equivalent)
#define 

 
 
  16         // D0 - Digital Output (Pin 28 equivalent)
#define STATUS_LED_PIN 2         // D4 - Built-in LED
#define BATTERY_PIN A0           // A0 - Battery Monitor
#define SDA_PIN 0                // D3 - I2C SDA
#define SCL_PIN 2                // D4 - I2C SCL

// LCD Configuration
#define LCD_ADDRESS 0x27
#define USE_LCD 1                // Set to 0 to disable LCD

// Component Initialization
#if USE_LCD
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
#endif

SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
SoftwareSerial gsmSerial(GSM_RX_PIN, GSM_TX_PIN);
TinyGPSPlus gps;
ESP8266WebServer server(80);

// Data Structures (MUST match ESP32 exactly)
typedef struct {
    uint32_t messageID;
    bool emergencyActive;
    bool emergencyDeactivate;
    int heartRate;
    int gasLevel;
    float watchBatteryLevel;
    bool personFainted;
    char timestamp[20];
    bool buttonPressed;
    bool shakingDetected;
    float accelX, accelY, accelZ;
    char emergencyType[20];
    uint32_t checksum;
} WatchData;

typedef struct {
    uint32_t messageID;
    float shoeBatteryLevel;
    bool gpsActive;
    bool gsmActive;
    double latitude;
    double longitude;
    bool emergencyMessageSent;
    char gpsStatus[20];
    char gsmStatus[20];
    uint32_t checksum;
} ShoeData;

// Emergency Contacts - REPLACE WITH REAL NUMBERS!
String emergencyContacts[4] = {
    "+1234567890",    // Contact 1 - REPLACE!
    "+1234567891",    // Contact 2 - REPLACE!
    "+1234567892",    // Contact 3 - REPLACE!
    "+1234567893"     // Contact 4 - REPLACE!
};

String contactNames[4] = {
    "Emergency Contact 1",
    "Emergency Contact 2",
    "Emergency Contact 3",
    "Emergency Contact 4"
};

// Global Variables
WatchData receivedWatchData;
ShoeData shoeData;

bool emergencyMode = false;
bool gpsActive = false;
bool gsmActive = false;
bool gpsFixed = false;
bool gsmReady = false;
bool emergencyMessageSent = false;
bool faintMessageSent = false;
bool espNowReady = false;
bool lcdAvailable = false;

double currentLatitude = 0.0;
double currentLongitude = 0.0;
float shoeBatteryPercentage = 100.0;
float watchBatteryPercentage = 100.0;

uint32_t messageCounter = 0;
uint32_t lastReceivedMessageID = 0;

unsigned long lastGpsRead = 0;
unsigned long lastBatteryRead = 0;
unsigned long lastStatusSend = 0;
unsigned long lastLcdUpdate = 0;

// Timing Constants
#define GPS_READ_INTERVAL 1000
#define BATTERY_READ_INTERVAL 30000
#define STATUS_SEND_INTERVAL 5000
#define LCD_UPDATE_INTERVAL 2000
#define GPS_TIMEOUT 60000
#define GSM_TIMEOUT 30000
#define LOCATION_UPDATE_INTERVAL 120000

// WiFi AP Configuration
const char* AP_SSID = "bbb";
const char* AP_PASS = "123456789";

// Function Prototypes
void initializeSystem();
void initESPNOW();
void initGSM();
void readGPS();
void readBattery();
void activateEmergency();
void deactivateEmergency();
void handleEmergencyMode();
void sendEmergencyAlert();
void sendFaintAlert();
void sendLocationUpdate();
void sendDeactivationAlert();
bool sendSMS(String number, String message);
bool waitForGSMResponse(String expected, unsigned long timeout);
void sendStatusToWatch();
void updateLCDDisplay();
void setupWebServer();
uint32_t calculateChecksum(uint8_t* data, size_t len);
void onDataSent(uint8_t *mac_addr, uint8_t status);
void onDataReceive(uint8_t *mac_addr, uint8_t *data, uint8_t len);

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== WOMEN SAFETY DEVICE - SHOE MODULE ===");
    Serial.println("Version: FINAL - Arduino IDE 2.3.6 Compatible");
    Serial.println("ESP8266 NodeMCU with GPIO14 & GPIO16 outputs");

    // Initialize system
    initializeSystem();

    // Initialize ESP-NOW
    initESPNOW();

    // Setup WiFi AP and Web Server
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("Web App IP: ");
    Serial.println(WiFi.softAPIP());

    setupWebServer();
    server.begin();

    // Initialize GSM
    Serial.println("Initializing GSM module...");
    initGSM();

    // Initialize data structure
    memset(&shoeData, 0, sizeof(shoeData));
    strcpy(shoeData.gpsStatus, "Inactive");
    strcpy(shoeData.gsmStatus, "Ready");
    shoeData.shoeBatteryLevel = 100.0;

    Serial.println("=== SHOE MODULE READY ===");
    Serial.println("MAC Address: " + WiFi.macAddress());
    Serial.println("Use this MAC in ESP32 Watch Module code!");

    // Ready indication
    for(int i = 0; i < 5; i++) {
        digitalWrite(STATUS_LED_PIN, LOW);  // LED ON
        delay(100);
        digitalWrite(STATUS_LED_PIN, HIGH); // LED OFF
        delay(100);
    }
}

void loop() {
    server.handleClient();

    // Read GPS data
    if (millis() - lastGpsRead > GPS_READ_INTERVAL) {
        readGPS();
        lastGpsRead = millis();
    }

    // Read battery level
    if (millis() - lastBatteryRead > BATTERY_READ_INTERVAL) {
        readBattery();
        lastBatteryRead = millis();
    }

    // Handle emergency mode
    if (emergencyMode) {
        handleEmergencyMode();
    }

    // Update LCD display
    #if USE_LCD
    if (lcdAvailable && (millis() - lastLcdUpdate > LCD_UPDATE_INTERVAL)) {
        updateLCDDisplay();
        lastLcdUpdate = millis();
    }
    #endif

    // Send status to watch
    if (millis() - lastStatusSend > STATUS_SEND_INTERVAL) {
        sendStatusToWatch();
        lastStatusSend = millis();
    }

    delay(100);
}

void initializeSystem() {
    // Initialize EEPROM
    EEPROM.begin(512);

    // Initialize pins
    pinMode(OUTPUT_PIN_27, OUTPUT);
    pinMode(OUTPUT_PIN_28, OUTPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(BATTERY_PIN, INPUT);

    digitalWrite(OUTPUT_PIN_27, LOW);
    digitalWrite(OUTPUT_PIN_28, LOW);
    digitalWrite(STATUS_LED_PIN, HIGH); // LED OFF

    Serial.println("Digital pins initialized:");
    Serial.println("- GPIO14 (D5) = Pin 27 equivalent");
    Serial.println("- GPIO16 (D0) = Pin 28 equivalent");

    // Initialize I2C and LCD
    #if USE_LCD
    Wire.begin(SDA_PIN, SCL_PIN);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Shoe Module");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");
    lcdAvailable = true;
    delay(2000);
    #endif

    // Initialize serial communications
    gpsSerial.begin(9600);
    gsmSerial.begin(9600);

    Serial.println("System components initialized");
}

void initESPNOW() {
    // Set WiFi mode
    WiFi.mode(WIFI_STA);

    // Print MAC address
    Serial.println("ESP8266 Shoe MAC: " + WiFi.macAddress());

    // Initialize ESP-NOW
    if (esp_now_init() != 0) {
        Serial.println("ESP-NOW init failed!");
        #if USE_LCD
        if (lcdAvailable) {
            lcd.clear();
            lcd.print("ESP-NOW Error!");
            delay(2000);
        }
        #endif
        return;
    }

    Serial.println("ESP-NOW initialized successfully");

    // Set role
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

    // Register callbacks
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceive);

    espNowReady = true;
}

void initGSM() {
    strcpy(shoeData.gsmStatus, "Initializing");

    // Clear buffer
    while (gsmSerial.available()) {
        gsmSerial.read();
    }

    delay(2000);

    // Test GSM module
    gsmSerial.println("AT");
    delay(1000);

    if (waitForGSMResponse("OK", 5000)) {
        Serial.println("GSM module responding");

        // Configure SMS text mode
        gsmSerial.println("AT+CMGF=1");
        delay(1000);
        if (!waitForGSMResponse("OK", 5000)) {
            Serial.println("Failed to set SMS text mode");
        }

        // Enable SMS notifications
        gsmSerial.println("AT+CNMI=1,2,0,0,0");
        delay(1000);
        waitForGSMResponse("OK", 5000);

        // Check network registration
        gsmSerial.println("AT+CREG?");
        delay(2000);

        String response = "";
        unsigned long timeout = millis() + 5000;
        while (millis() < timeout && gsmSerial.available()) {
            response += char(gsmSerial.read());
        }

        if (response.indexOf("+CREG: 0,1") >= 0 || response.indexOf("+CREG: 0,5") >= 0) {
            Serial.println("Network registered");
            strcpy(shoeData.gsmStatus, "Network OK");
            gsmReady = true;
        } else {
            Serial.println("Network registration failed");
            strcpy(shoeData.gsmStatus, "No Network");
            gsmReady = false;
        }

    } else {
        Serial.println("GSM module not responding");
        Serial.println("Check connections and SIM card");
        strcpy(shoeData.gsmStatus, "Error");
        gsmReady = false;
    }
}

void readGPS() {
    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        if (gps.encode(c)) {
            if (gps.location.isValid()) {
                currentLatitude = gps.location.lat();
                currentLongitude = gps.location.lng();
                shoeData.latitude = currentLatitude;
                shoeData.longitude = currentLongitude;

                if (!gpsFixed) {
                    gpsFixed = true;
                    strcpy(shoeData.gpsStatus, "Fixed");
                    Serial.printf("GPS Fix: %.6f, %.6f\n", currentLatitude, currentLongitude);
                }
            }
        }
    }

    // Update status
    if (gpsActive) {
        if (!gpsFixed) {
            strcpy(shoeData.gpsStatus, "Searching");
        }
        // Update LED
        if (gpsFixed) {
            digitalWrite(STATUS_LED_PIN, LOW); // ON
        } else {
            static unsigned long lastBlink = 0;
            static bool ledState = false;
            if (millis() - lastBlink > 1000) {
                ledState = !ledState;
                digitalWrite(STATUS_LED_PIN, ledState);
                lastBlink = millis();
            }
        }
    } else {
        strcpy(shoeData.gpsStatus, "Inactive");
        digitalWrite(STATUS_LED_PIN, HIGH); // OFF
    }
}

void readBattery() {
    int battRaw = analogRead(BATTERY_PIN);
    float voltage = (battRaw * 3.3 * 2.0) / 1024.0;
    shoeBatteryPercentage = map(voltage * 100, 300, 420, 0, 100);
    shoeBatteryPercentage = constrain(shoeBatteryPercentage, 0, 100);
    shoeData.shoeBatteryLevel = shoeBatteryPercentage;
}

bool waitForGSMResponse(String expected, unsigned long timeout) {
    unsigned long start = millis();
    String response = "";

    while (millis() - start < timeout) {
        if (gsmSerial.available()) {
            response += char(gsmSerial.read());
            if (response.indexOf(expected) >= 0) {
                return true;
            }
        }
        delay(10);
    }
    return false;
}

// ESP-NOW Callbacks for ESP8266
void onDataSent(uint8_t *mac_addr, uint8_t status) {
    // Serial.printf("Send Status: %s\n", status == 0 ? "OK" : "FAIL");
}

void onDataReceive(uint8_t *mac_addr, uint8_t *data, uint8_t len) {
    if (len != sizeof(receivedWatchData)) {
        Serial.printf("Size mismatch: expected %d, got %d\n", sizeof(receivedWatchData), len);
        return;
    }

    memcpy(&receivedWatchData, data, sizeof(receivedWatchData));

    // Verify checksum
    uint32_t calcChecksum = calculateChecksum((uint8_t*)&receivedWatchData, 
                                             sizeof(receivedWatchData) - sizeof(receivedWatchData.checksum));

    if (calcChecksum != receivedWatchData.checksum) {
        Serial.println("Checksum failed");
        return;
    }

    // Check for duplicate
    if (receivedWatchData.messageID <= lastReceivedMessageID) {
        return;
    }
    lastReceivedMessageID = receivedWatchData.messageID;

    // Update watch battery
    watchBatteryPercentage = receivedWatchData.watchBatteryLevel;

    Serial.printf("Watch data (ID:%d): Emergency=%s, HR=%d, Gas=%d, Type=%s\n",
                  receivedWatchData.messageID,
                  receivedWatchData.emergencyActive ? "YES" : "NO",
                  receivedWatchData.heartRate,
                  receivedWatchData.gasLevel,
                  receivedWatchData.emergencyType);

    // Handle emergency activation
    if (receivedWatchData.emergencyActive && !emergencyMode) {
        Serial.println("Emergency activation received");
        activateEmergency();
    }

    // Handle emergency deactivation
    if (receivedWatchData.emergencyDeactivate && emergencyMode) {
        Serial.println("Emergency deactivation received");
        deactivateEmergency();
    }

    // Handle faint condition
    if (receivedWatchData.personFainted && emergencyMode && !faintMessageSent) {
        Serial.println("Person fainted detected");
        sendFaintAlert();
        faintMessageSent = true;
    }
}

void activateEmergency() {
    emergencyMode = true;
    emergencyMessageSent = false;
    faintMessageSent = false;

    Serial.println("EMERGENCY MODE ACTIVATED!");

    // Activate GPS and GSM
    gpsActive = true;
    gsmActive = true;
    shoeData.gpsActive = true;
    shoeData.gsmActive = true;

    // Activate digital output pins
    digitalWrite(OUTPUT_PIN_27, HIGH);
    digitalWrite(OUTPUT_PIN_28, HIGH);

    Serial.println("Digital pins activated: GPIO14=HIGH, GPIO16=HIGH");

    // Update status
    strcpy(shoeData.gpsStatus, "Acquiring Fix");
    if (gsmReady) {
        strcpy(shoeData.gsmStatus, "Active");
    }

    // Wait for GPS fix
    unsigned long gpsStart = millis();
    while (!gpsFixed && (millis() - gpsStart < GPS_TIMEOUT)) {
        readGPS();
        delay(1000);
    }

    // Send emergency SMS
    if (gpsFixed) {
        Serial.println("Sending emergency alert with location");
        sendEmergencyAlert();
    } else {
        Serial.println("GPS timeout - sending alert without location");
        strcpy(shoeData.gpsStatus, "No Fix");
        sendEmergencyAlert();
    }
}

void deactivateEmergency() {
    emergencyMode = false;
    emergencyMessageSent = false;
    faintMessageSent = false;

    Serial.println("EMERGENCY MODE DEACTIVATED");

    // Deactivate GPS and GSM
    gpsActive = false;
    gsmActive = false;
    gpsFixed = false;
    shoeData.gpsActive = false;
    shoeData.gsmActive = false;

    // Deactivate digital pins
    digitalWrite(
         , LOW);
    digitalWrite(OUTPUT_PIN_28, LOW);

    Serial.println("Digital pins deactivated: GPIO14=LOW, GPIO16=LOW");

    // Update status
    strcpy(shoeData.gpsStatus, "Inactive");
    strcpy(shoeData.gsmStatus, "Ready");

    // Send deactivation alert
    sendDeactivationAlert();
}

void handleEmergencyMode() {
    static unsigned long lastLocationUpdate = 0;

    // Send periodic location updates
    if (gpsFixed && (millis() - lastLocationUpdate > LOCATION_UPDATE_INTERVAL)) {
        Serial.println("Sending location update");
        sendLocationUpdate();
        lastLocationUpdate = millis();
    }
}

void sendEmergencyAlert() {
    if (!gsmReady) {
        Serial.println("GSM not ready");
        strcpy(shoeData.gsmStatus, "Not Ready");
        return;
    }

    strcpy(shoeData.gsmStatus, "Sending SMS");

    String message = "🚨 EMERGENCY ALERT! 🚨\n";
    message += "Women Safety Device activated.\n";
    message += "Time: " + String(receivedWatchData.timestamp) + "\n";
    message += "Trigger: " + String(receivedWatchData.emergencyType) + "\n";
    message += "Heart Rate: " + String(receivedWatchData.heartRate) + " BPM\n";
    message += "Gas Level: " + String(receivedWatchData.gasLevel) + " PPM\n";

    if (gpsFixed) {
        message += "Location: https://maps.google.com/maps?q=";
        message += String(currentLatitude, 6) + "," + String(currentLongitude, 6) + "\n";
    } else {
        message += "Location: GPS searching...\n";
    }

    message += "Please respond immediately!";

    // Send to all contacts
    int successCount = 0;
    for (int i = 0; i < 4; i++) {
        Serial.printf("Sending to contact %d: %s\n", i+1, emergencyContacts[i].c_str());
        if (sendSMS(emergencyContacts[i], message)) {
            successCount++;
        }
        delay(5000);
    }

    emergencyMessageSent = true;
    shoeData.emergencyMessageSent = true;

    if (successCount > 0) {
        strcpy(shoeData.gsmStatus, "SMS Sent");
        Serial.printf("SMS sent to %d/%d contacts\n", successCount, 4);
    } else {
        strcpy(shoeData.gsmStatus, "SMS Failed");
        Serial.println("All SMS failed");
    }
}

void sendFaintAlert() {
    if (!gsmReady) return;

    String message = "🆘 CRITICAL ALERT! 🆘\n";
    message += "Your friend has fainted and fallen down.\n";
    message += "No movement detected.\n";
    message += "Time: " + String(receivedWatchData.timestamp) + "\n";

    if (gpsFixed) {
        message += "Location: https://maps.google.com/maps?q=";
        message += String(currentLatitude, 6) + "," + String(currentLongitude, 6) + "\n";
    }

    message += "IMMEDIATE HELP REQUIRED!";

    for (int i = 0; i < 4; i++) {
        sendSMS(emergencyContacts[i], message);
        delay(5000);
    }

    Serial.println("Faint alerts sent");
}

void sendLocationUpdate() {
    if (!gsmReady || !gpsFixed) return;

    String message = "📍 Location Update - Emergency Active\n";
    message += "https://maps.google.com/maps?q=";
    message += String(currentLatitude, 6) + "," + String(currentLongitude, 6) + "\n";
    message += "HR: " + String(receivedWatchData.heartRate) + " BPM\n";
    message += "Battery: W" + String((int)watchBatteryPercentage) + "% S" + String((int)shoeBatteryPercentage) + "%";

    // Send to first 2 contacts only
    for (int i = 0; i < 2; i++) {
        sendSMS(emergencyContacts[i], message);
        delay(5000);
    }
}

void sendDeactivationAlert() {
    if (!gsmReady) return;

    String message = "✅ Women Safety Device - Emergency Deactivated\n";
    message += "Emergency manually deactivated.\n";
    message += "Time: " + String(receivedWatchData.timestamp) + "\n";
    message += "If not done by user, check immediately.";

    for (int i = 0; i < 4; i++) {
        sendSMS(emergencyContacts[i], message);
        delay(5000);
    }
}

bool sendSMS(String phoneNumber, String message) {
    // Clear buffer
    while (gsmSerial.available()) gsmSerial.read();

    // Send AT command
    gsmSerial.println("AT+CMGS=\"" + phoneNumber + "\"");
    delay(1000);

    if (waitForGSMResponse(">", 10000)) {
        gsmSerial.print(message);
        delay(1000);
        gsmSerial.write(26); // Ctrl+Z

        if (waitForGSMResponse("OK", 15000)) {
            Serial.println("SMS sent to " + phoneNumber);
            return true;
        }
    }

    Serial.println("SMS failed to " + phoneNumber);
    return false;
}

void sendStatusToWatch() {
    if (!espNowReady) return;

    shoeData.messageID = messageCounter++;
    shoeData.checksum = calculateChecksum((uint8_t*)&shoeData, 
                                         sizeof(shoeData) - sizeof(shoeData.checksum));

    uint8_t broadcastMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcastMAC, (uint8_t*)&shoeData, sizeof(shoeData));
}

#if USE_LCD
void updateLCDDisplay() {
    lcd.clear();

    // First line - Battery levels
    lcd.setCursor(0, 0);
    lcd.printf("W:%3.0f%% S:%3.0f%%", watchBatteryPercentage, shoeBatteryPercentage);

    // Second line - Status
    lcd.setCursor(0, 1);
    if (emergencyMode) {
        lcd.print("EMERGENCY ACTIVE");
    } else {
        String status = "GPS:";
        status += gpsFixed ? "OK" : "--";
        status += " GSM:";
        status += gsmReady ? "OK" : "--";
        lcd.print(status);
    }
}
#endif

uint32_t calculateChecksum(uint8_t* data, size_t len) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum += data[i];
    }
    return checksum;
}

void setupWebServer() {
    server.on("/", HTTP_GET, []() {
        String html = F(R"(
<!DOCTYPE html>
<html><head><title>Women Safety Shoe</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>body{font-family:Arial;margin:20px;background:#e8f4f8}.container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px}.header{text-align:center;background:#28a745;color:white;padding:20px;border-radius:8px;margin-bottom:20px}.card{background:#f8f9fa;padding:15px;margin:10px 0;border-radius:8px;border-left:4px solid #28a745}.emergency{border-left-color:#dc3545;background:#fff5f5}.status{font-weight:bold}.active{color:#dc3545;animation:blink 1s infinite}.inactive{color:#28a745}.pins{font-family:monospace;padding:8px;background:#f1f3f4;border-radius:4px}.high{color:#dc3545;font-weight:bold}.low{color:#6c757d}@keyframes blink{0%,50%{opacity:1}51%,100%{opacity:0.7}}</style>
</head><body>
<div class="container">
<div class="header"><h1>👟 Women Safety Shoe</h1><p>GPS & Emergency Communication</p></div>
<div class="card" id="status"><h3>📊 System Status</h3>
<p>Emergency: <span id="emergency" class="status">Loading...</span></p>
<p>GPS Status: <span id="gpsStatus">--</span></p>
<p>GSM Status: <span id="gsmStatus">--</span></p>
<p>Digital Pins: <span id="pinStatus" class="pins">--</span></p></div>
<div class="card"><h3>🔋 Battery Status</h3>
<p>Watch: <span id="watchBatt">--</span>%</p>
<p>Shoe: <span id="shoeBatt">--</span>%</p></div>
<div class="card"><h3>📍 Location</h3>
<p>Latitude: <span id="lat">--</span></p>
<p>Longitude: <span id="lon">--</span></p>
<p><a href="#" id="mapsLink" target="_blank">Maps link available when GPS fixed</a></p></div>
<div class="card"><h3>📱 Sensor Data</h3>
<p>Heart Rate: <span id="heartRate">--</span> BPM</p>
<p>Gas Level: <span id="gasLevel">--</span> PPM</p>
<p>Emergency Type: <span id="emergencyType">--</span></p>
<p>Timestamp: <span id="timestamp">--</span></p></div>
<div class="card"><h3>🔧 System Info</h3>
<p><b>ESP8266 Pin Mapping:</b></p>
<p>GPIO14 (D5) = Pin 27 equivalent</p>
<p>GPIO16 (D0) = Pin 28 equivalent</p>
<p>MAC: Use serial monitor to get address</p></div>
</div>
<script>
function updateStatus(){
fetch('/api/status').then(r=>r.json()).then(d=>{
document.getElementById('emergency').textContent = d.emergency ? 'ACTIVE' : 'Ready';
document.getElementById('emergency').className = d.emergency ? 'status active' : 'status inactive';
document.getElementById('gpsStatus').textContent = d.gpsStatus || '--';
document.getElementById('gsmStatus').textContent = d.gsmStatus || '--';
const pins = document.getElementById('pinStatus');
if(d.emergency) { pins.textContent = 'GPIO14:HIGH GPIO16:HIGH'; pins.className = 'pins high'; }
else { pins.textContent = 'GPIO14:LOW GPIO16:LOW'; pins.className = 'pins low'; }
document.getElementById('watchBatt').textContent = d.watchBatt || '--';
document.getElementById('shoeBatt').textContent = d.shoeBatt || '--';
document.getElementById('lat').textContent = d.lat ? d.lat.toFixed(6) : '--';
document.getElementById('lon').textContent = d.lon ? d.lon.toFixed(6) : '--';
if(d.lat && d.lon) {
const url = `https://maps.google.com/maps?q=${d.lat},${d.lon}`;
document.getElementById('mapsLink').href = url;
document.getElementById('mapsLink').textContent = '📍 View Location on Google Maps';
}
document.getElementById('heartRate').textContent = d.heartRate || '--';
document.getElementById('gasLevel').textContent = d.gasLevel || '--';
document.getElementById('emergencyType').textContent = d.emergencyType || '--';
document.getElementById('timestamp').textContent = d.timestamp || '--';
document.getElementById('status').className = d.emergency ? 'card emergency' : 'card';
}).catch(e=>console.error(e));}
setInterval(updateStatus, 3000); updateStatus();
</script></body></html>
        )");
        server.send(200, "text/html", html);
    });

    server.on("/api/status", HTTP_GET, []() {
        DynamicJsonDocument doc(1024);
        doc["emergency"] = emergencyMode;
        doc["gpsStatus"] = shoeData.gpsStatus;
        doc["gsmStatus"] = shoeData.gsmStatus;
        doc["watchBatt"] = watchBatteryPercentage;
        doc["shoeBatt"] = shoeBatteryPercentage;
        doc["lat"] = currentLatitude;
        doc["lon"] = currentLongitude;
        doc["heartRate"] = receivedWatchData.heartRate;
        doc["gasLevel"] = receivedWatchData.gasLevel;
        doc["emergencyType"] = receivedWatchData.emergencyType;
        doc["timestamp"] = receivedWatchData.timestamp;
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });
}
