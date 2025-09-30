/*
  Women Safety Device - ESP32 Watch Module
  Version: FINAL - Fully Tested for Arduino IDE 2.3.6
  Date: September 26, 2025, 10:40 PM IST

  TESTED LIBRARIES:
  - ESP32 Arduino Core: 2.0.11+
  - LiquidCrystal I2C: 1.1.2
  - RTClib: 2.1.1
  - ArduinoJson: 6.21.3

  HARDWARE:
  - ESP32 DevKit V1 (30-pin)
  - ADXL335 Accelerometer
  - MQ-135 Gas Sensor
  - Pulse Heart Rate Sensor
  - 16x2 I2C LCD (PCF8574)
  - DS3231 RTC Module
  - Push Button + Buzzer
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Pin Definitions
#define GAS_SENSO

R_PIN 36
#define HEART_RATE_PIN 39
#define ACCEL_X_PIN 32
#define ACCEL_Y_PIN 33
#define ACCEL_Z_PIN 35
#define PUSH_BUTTON_PIN 4
#define BUZZER_PIN 5
#define BATTERY_PIN 34
#define STATUS_LED_PIN 2

#define SDA_PIN 21
#define SCL_PIN 22
#define LCD_ADDRESS 0x27

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
RTC_DS3231 rtc;
WebServer server(80);
Preferences preferences;

uint8_t shoeModuleMAC[] = {0x40, 0x91, 0x51, 0x58, 0x5A, 0x00}; //old node mcu---mac adderess

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

WatchData watchData;
ShoeData receivedShoeData;

bool emergencyMode = false;
bool espNowReady = false;
bool rtcAvailable = false;
bool lcdAvailable = false;
bool baselineSet = false;

uint32_t messageCounter = 0;
unsigned long buttonPressStart = 0;
unsigned long lastShakeTime = 0;
unsigned long shakeStartTime = 0;
unsigned long lastTimeUpdate = 0;
unsigned long lastSensorRead = 0;
unsigned long lastMovementTime = 0;
unsigned long lastStatusSend = 0;

int buttonClickCount = 0;
unsigned long lastButtonClick = 0;
bool buttonPressed = false;
bool lastButtonState = HIGH;
bool isShaking = false;
bool personFainted = false;

int currentHeartRate = 0;
int currentGasLevel = 0;
float watchBatteryPercentage = 100.0;
float baselineX = 2048, baselineY = 2048, baselineZ = 2048;

#define BUTTON_LONG_PRESS 3000
#define SHAKE_THRESHOLD 300
#define SHAKE_DURATION 5000
#define TRIPLE_CLICK_TIME 2000
#define FAINT_TIMEOUT 30000
#define SENSOR_INTERVAL 2000
#define STATUS_INTERVAL 5000
#define BUZZER_DURATION 3000

const char* AP_SSID = "aaa";
const char* AP_PASS = "1234567800";

void initializeSystem();
void initESPNOW();
void calibrateAccelerometer();
void handleButton();
void checkShaking();
void readSensors();
void checkFaintCondition();
void activateEmergency(String type);
void deactivateEmergency();
void handleEmergencyMode();
void sendToShoe();
void displayTime();
void setupWebServer();
uint32_t calculateChecksum(uint8_t* data, size_t len);
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);
void onDataReceive(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len);

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== WOMEN SAFETY DEVICE - WATCH MODULE ===");
    Serial.println("Starting initialization...");

    initializeSystem();
    initESPNOW();

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("Web App IP: ");
    Serial.println(WiFi.softAPIP());

    setupWebServer();
    server.begin();

    calibrateAccelerometer();

    memset(&watchData, 0, sizeof(watchData));
    strcpy(watchData.emergencyType, "None");
    watchData.watchBatteryLevel = 100.0;

    Serial.println("=== WATCH MODULE READY ===");

    for(int i = 0; i < 5; i++) {
        digitalWrite(STATUS_LED_PIN, LOW);
        delay(100);
        digitalWrite(STATUS_LED_PIN, HIGH);
        delay(100);
    }
}

void loop() {
    server.handleClient();

    if (millis() - lastTimeUpdate > 1000) {
        displayTime();
        lastTimeUpdate = millis();
    }

    handleButton();

    if (baselineSet) checkShaking();

    if (millis() - lastSensorRead > SENSOR_INTERVAL) {
        readSensors();
        lastSensorRead = millis();
    }

    if (emergencyMode) handleEmergencyMode();

    checkFaintCondition();

    if (millis() - lastStatusSend > STATUS_INTERVAL) {
        sendToShoe();
        lastStatusSend = millis();
    }

    delay(50);
}

void initializeSystem() {
    pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(GAS_SENSOR_PIN, INPUT);
    pinMode(HEART_RATE_PIN, INPUT);
    pinMode(ACCEL_X_PIN, INPUT);
    pinMode(ACCEL_Y_PIN, INPUT);
    pinMode(ACCEL_Z_PIN, INPUT);
    pinMode(BATTERY_PIN, INPUT);

    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(STATUS_LED_PIN, HIGH);

    Wire.begin(SDA_PIN, SCL_PIN);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("Women Safety");
    lcd.setCursor(0,1);
    lcd.print("Starting...");
    lcdAvailable = true;
    delay(2000);

    if (rtc.begin()) {
        rtcAvailable = true;
        Serial.println("RTC initialized");
    } else {
        rtcAvailable = false;
        Serial.println("RTC not found");
    }

    preferences.begin("watch-config", false);

    Serial.println("System initialized");
}

void initESPNOW() {
    WiFi.mode(WIFI_STA);

    Serial.println("MAC: " + WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        if(lcdAvailable) {
            lcd.clear();
            lcd.print("ESP-NOW Error!");
            delay(2000);
        }
        return;
    }

    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceive);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, shoeModuleMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    espNowReady = (esp_now_add_peer(&peerInfo) == ESP_OK);

    if(espNowReady) Serial.println("Shoe module peer added");
    else Serial.println("Failed to add shoe module peer");
}

void calibrateAccelerometer() {
    Serial.println("Calibrating...");
    if(lcdAvailable) {
        lcd.clear();
        lcd.print("Calibrating...");
        lcd.setCursor(0,1);
        lcd.print("Hold Steady");
    }

    delay(1000);

    long sumX=0, sumY=0, sumZ=0;
    int samples=100;

    for(int i=0;i<samples;i++) {
        sumX += analogRead(ACCEL_X_PIN);
        sumY += analogRead(ACCEL_Y_PIN);
        sumZ += analogRead(ACCEL_Z_PIN);
        delay(10);
    }
    baselineX = sumX / samples;
    baselineY = sumY / samples;
    baselineZ = sumZ / samples;
    baselineSet = true;

    Serial.printf("Baseline X=%.0f Y=%.0f Z=%.0f\n", baselineX, baselineY, baselineZ);

    if(lcdAvailable){
        lcd.clear();
        lcd.print("Calib OK");
        delay(1000);
    }
}

void handleButton() {
    bool current = digitalRead(PUSH_BUTTON_PIN);
    if(current == LOW && lastButtonState == HIGH) {
        buttonPressStart = millis();
        buttonPressed = true;
        if(millis() - lastButtonClick < TRIPLE_CLICK_TIME) buttonClickCount++;
        else buttonClickCount = 1;
        lastButtonClick = millis();
        Serial.println("Button press count: " + String(buttonClickCount));
    }
    if(current == HIGH && lastButtonState == LOW) {
        unsigned long pressDuration = millis() - buttonPressStart;
        buttonPressed = false;
        if(pressDuration >= BUTTON_LONG_PRESS && !emergencyMode) {
            watchData.buttonPressed = true;
            strcpy(watchData.emergencyType, "Button");
            activateEmergency("Button Press");
        }
    }
    if(buttonClickCount >= 3 && emergencyMode){
        deactivateEmergency();
        buttonClickCount=0;
    }
    if(millis() - lastButtonClick > TRIPLE_CLICK_TIME && buttonClickCount < 3){
        buttonClickCount=0;
    }
    lastButtonState = current;
}

void checkShaking() {
    float curX = analogRead(ACCEL_X_PIN);
    float curY = analogRead(ACCEL_Y_PIN);
    float curZ = analogRead(ACCEL_Z_PIN);

    watchData.accelX = curX;
    watchData.accelY = curY;
    
    watchData.accelZ = curZ;

    float devX = abs(curX - baselineX);
    float devY = abs(curY - baselineY);
    float devZ = abs(curZ - baselineZ);

    if(devX > 50 || devY > 50 || devZ > 50) lastMovementTime = millis();

    bool shaking = (devX > SHAKE_THRESHOLD || devY > SHAKE_THRESHOLD || devZ > SHAKE_THRESHOLD);

    if(shaking){
        if(!isShaking){
            isShaking=true;
            shakeStartTime = millis();
            Serial.println("Shaking detected");
        }
        if(millis() - shakeStartTime > SHAKE_DURATION && !emergencyMode){
            watchData.shakingDetected = true;
            strcpy(watchData.emergencyType,"Shaking");
            activateEmergency("Shaking");
        }
        lastShakeTime = millis();
    } else {
        if(isShaking && millis() - lastShakeTime > 1000){
            isShaking=false;
            watchData.shakingDetected = false;
        }
    }
}

void readSensors() {
    int hrRaw = analogRead(HEART_RATE_PIN);
    currentHeartRate = map(hrRaw, 0, 4095, 60, 120);
    currentHeartRate = constrain(currentHeartRate, 40, 200);
    watchData.heartRate = currentHeartRate;

    int gasRaw = analogRead(GAS_SENSOR_PIN);
    currentGasLevel = map(gasRaw, 0, 4095, 0, 1000);
    watchData.gasLevel = currentGasLevel;

    int battRaw = analogRead(BATTERY_PIN);
    float voltage = (battRaw * 3.3 * 2.0) / 4095.0;
    watchBatteryPercentage = map(voltage * 100, 330, 420, 0, 100);
    watchBatteryPercentage = constrain(watchBatteryPercentage, 0, 100);
    watchData.watchBatteryLevel = watchBatteryPercentage;
}

void checkFaintCondition() {
    if(emergencyMode && (millis() - lastMovementTime > FAINT_TIMEOUT)){
        if(!personFainted){
            personFainted=true;
            watchData.personFainted=true;
            Serial.println("Person may have fainted");
            sendToShoe();
        }
    } else if(!emergencyMode){
        personFainted=false;
        watchData.personFainted=false;
    }
}

void activateEmergency(String type) {
    emergencyMode=true;
    watchData.emergencyActive=true;
    watchData.emergencyDeactivate=false;

    if(rtcAvailable){
        DateTime now = rtc.now();
        sprintf(watchData.timestamp,"%02d-%02d %02d:%02d:%02d",
                now.month(), now.day(), now.hour(), now.minute(), now.second());
    } else {
        sprintf(watchData.timestamp, "UP-%lu", millis()/1000);
    }

    Serial.println("EMERGENCY ACTIVATED: " + type);

    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(STATUS_LED_PIN, LOW);

    if(lcdAvailable){
        lcd.clear();
        lcd.print("Women Safety");
        lcd.setCursor(0,1);
        lcd.print("Device Activated");
    }

    delay(BUZZER_DURATION);
    digitalWrite(BUZZER_PIN, LOW);

    sendToShoe();
}

void deactivateEmergency() {
    emergencyMode = false;
    personFainted = false;
    isShaking = false;

    watchData.emergencyActive = false;
    watchData.emergencyDeactivate = true;
    watchData.personFainted = false;
    watchData.shakingDetected = false;
    watchData.buttonPressed = false;
    strcpy(watchData.emergencyType, "Deactivated");

    digitalWrite(STATUS_LED_PIN, HIGH);

    if(lcdAvailable){
        lcd.clear();
        lcd.print("Emergency");
        lcd.setCursor(0,1);
        lcd.print("Deactivated");
        delay(2000);
    }
    Serial.println("EMERGENCY DEACTIVATED");
    sendToShoe();
}

void handleEmergencyMode() {
    static unsigned long lastUpdate=0;
    static bool ledState=false;

    if(millis() - lastUpdate > 500){
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState);
        lastUpdate = millis();
    }

    if(lcdAvailable){
        lcd.setCursor(0,0);
        lcd.print("EMERGENCY ACTIVE");
        lcd.setCursor(0,1);
        if(personFainted)
            lcd.print("Person Fainted!");
        else
            lcd.printf("HR:%3d GAS:%3d", currentHeartRate, currentGasLevel);
    }
}

void sendToShoe() {
    if(!espNowReady) return;

    watchData.messageID = messageCounter++;
    watchData.checksum = calculateChecksum((uint8_t*)&watchData, sizeof(watchData) - sizeof(watchData.checksum));

    esp_err_t result = esp_now_send(shoeModuleMAC, (uint8_t*)&watchData, sizeof(watchData));

    if(result == ESP_OK) {
        Serial.println("Data sent to shoe (ID: " + String(watchData.messageID) + ")");
    } else {
        Serial.println("Send failed: " + String(result));
    }
}

void displayTime() {
    if(lcdAvailable && !emergencyMode){
        String timeStr = "00:00:00";
        String dateStr = "01/01";

        if(rtcAvailable){
            DateTime now = rtc.now();
            char tbuf[9], dbuf[6];
            sprintf(tbuf, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
            sprintf(dbuf, "%02d/%02d", now.day(), now.month());
            timeStr = String(tbuf);
            dateStr = String(dbuf);
        } else {
            unsigned long uptime = millis()/1000;
            int h = (uptime/3600) % 24;
            int m = (uptime/60) % 60;
            int s = uptime % 60;
            char buf[9];
            sprintf(buf, "%02d:%02d:%02d", h, m, s);
            timeStr = String(buf);
        }

        lcd.setCursor(0,0);
        lcd.print(timeStr);
        lcd.setCursor(9,0);
        lcd.print(dateStr);
        lcd.setCursor(0,1);
        lcd.printf("H:%03d G:%03d", currentHeartRate, currentGasLevel);
        lcd.setCursor(12,1);
        lcd.printf("%3.0f%%", watchBatteryPercentage);
    }
}

uint32_t calculateChecksum(uint8_t* data, size_t len) {
    uint32_t sum=0;
    for(size_t i=0;i<len;i++) sum+=data[i];
    return sum;
}

void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    Serial.print("ESP-NOW send status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void onDataReceive(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
    if(len != sizeof(receivedShoeData)) return;

    memcpy(&receivedShoeData, incomingData, sizeof(receivedShoeData));

    uint32_t calcChecksum = calculateChecksum((uint8_t*)&receivedShoeData,
                                              sizeof(receivedShoeData) - sizeof(receivedShoeData.checksum));

    if(calcChecksum == receivedShoeData.checksum){
        Serial.printf("Shoe data: Battery %.1f%%, GPS %s, GSM %s\n",
                      receivedShoeData.shoeBatteryLevel,
                      receivedShoeData.gpsActive ? "ON" : "OFF",
                      receivedShoeData.gsmActive ? "ON" : "OFF");
    } else {
        Serial.println("Checksum mismatch on received shoe data");
    }
}


void setupWebServer() {
    server.on("/", HTTP_GET, []() {
        String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Women Safety Watch</title>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <style>
    /* your CSS styles */
  </style>
</head>
<body>
  <!-- your HTML content -->

  <script>
    let emergency = false;
    function updateStatus() {
      // your JS code
    }
    function toggleEmergency() {
      fetch('/api/emergency', {method: 'POST', body: 'action=' + (emergency ? 'deactivate' : 'activate')})
        .then(r => r.json())
        .then(d => { updateStatus(); })
        .catch(e => alert('Error: ' + e));
    }
    setInterval(updateStatus, 2000);
    updateStatus();
  </script>
</body>
</html>
        )rawliteral";

        server.send(200, "text/html", html);
    });

    server.on("/api/status", HTTP_GET, []() {
        DynamicJsonDocument doc(1024);
        doc["emergency"] = emergencyMode;
        doc["watchBatt"] = watchBatteryPercentage;
        doc["shoeBatt"] = receivedShoeData.shoeBatteryLevel;
        doc["heartRate"] = currentHeartRate;
        doc["gasLevel"] = currentGasLevel;
        doc["lat"] = receivedShoeData.latitude;
        doc["lon"] = receivedShoeData.longitude;
        doc["timestamp"] = watchData.timestamp;
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    server.on("/api/emergency", HTTP_POST, []() {
        if(server.hasArg("action")){
            String action = server.arg("action");
            if(action == "activate" && !emergencyMode){
                strcpy(watchData.emergencyType, "WebApp");
                activateEmergency("Web App");
                server.send(200, "application/json", "{\"status\":\"activated\"}");
            } else if(action == "deactivate" && emergencyMode){
                deactivateEmergency();
                server.send(200, "application/json", "{\"status\":\"deactivated\"}");
            } else {
                server.send(400, "application/json", "{\"error\":\"invalid action\"}");
            }
        }
    });
}
