# Women Safety Device: ESP32 Watch and ESP8266 Smart Shoe

![Project Banner](https://i.ibb.co/cK65J10p/Picsart-25-10-09-17-54-19-962.png)

[![GitHub stars](https://img.shields.io/github/stars/jeyaram1023/Women-safety-wearable-device?style=social)](https://github.com/jeyaram1023/Women-safety-wearable-device/stargazers) 
[![GitHub forks](https://img.shields.io/github/forks/jeyaram1023/Women-safety-wearable-device?style=social)](https://github.com/jeyaram1023/Women-safety-wearable-device/network/members) 
[![GitHub issues](https://img.shields.io/github/issues/jeyaram1023/Women-safety-wearable-device)](https://github.com/jeyaram1023/Women-safety-wearable-device/issues) 
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-ESP32--ESP8266-blue)](https://www.espressif.com)
[![Language](https://img.shields.io/badge/Language-C++-brightgreen)](https://isocpp.org/)
[![Arduino IDE](https://img.shields.io/badge/Arduino%20IDE-2.3.6-orange?logo=arduino)](https://www.arduino.cc/en/software)

This project is a **complete IoT solution for women’s personal safety**. It combines a wearable **ESP32 Watch module** and a connected **ESP8266-based Smart Shoe**, linked with the ESP-NOW wireless protocol for fast emergency communication. The system enables instant distress alerts, vital health data transmission, and automated location sharing to trusted contacts.

## 💡 Problem Statement

Despite modern advances, women's safety remains a critical issue. This device provides immediate, hands-free emergency notifications with GPS tracking and health monitoring to enhance personal security.

## 🚀 Solution Overview

- Wearable watch module monitors movement, pulse, gas levels, and manual emergency button.
- Smart shoe module receives emergency alerts, activates GPS & GSM, and sends SMS with location.
- Each module displays battery status and health info on an LCD.
- Local web applications hosted on both devices allow real-time monitoring and control.

## 📺 Demo Video

[![Watch the Demo](https://img.youtube.com/vi/YOUR_YOUTUBE_VIDEO_ID/0.jpg)](https://www.youtube.com/watch?v=YOUR_YOUTUBE_VIDEO_ID)

Click the image to watch the demo video on YouTube.

## 📷 Project Image

![Women Safety Device](https://i.ibb.co/LDQQ8244/shoe-circuit.png)
![Women Safety Device](https://i.ibb.co/39bMVzYP/watch-circuit.png)

*Photo: The Women Safety Device with Watch and Smart Shoe modules.*

## 🛠️ Features

- Emergency activation by 3-second button press or 5-second shaking detection.
- Triple-click the button to deactivate emergency mode.
- Instant wireless signal transmission to shoe module.
- GPS location and health stats sent via SMS to 4 emergency contacts.
- Faint/unconscious user detection triggers special alert.
- Real-time system status on local web app.
- Battery monitoring on both modules.

## 🔌 Hardware Overview

| Module        | Components                                  |
|---------------|---------------------------------------------|
| Watch Module  | ESP32, ADXL335 accelerometer, MQ-135 gas sensor, heart rate sensor, 16x2 I2C LCD, push button, buzzer, DS3231 RTC, status LED, 18650 battery |
| Shoe Module   | ESP8266 NodeMCU, GPS Neo-6M, SIM800L GSM module, 16x2 I2C LCD, digital pins GPIO14 & GPIO16, status LED, 18650 battery (dual recommended), SIM card |

## ⚙️ Installation & Setup

1. Clone the repository:

git clone https://github.com/jeyaram1023/Women-safety-wearable-device.git


2. Install Arduino IDE 2.3.6 with ESP32 and ESP8266 board support.

3. Install required libraries:

- LiquidCrystal_I2C
- RTClib
- TinyGPS++
- ArduinoJson

4. Upload `Women-Safety-Device-ESP8266-002.ino` first:

- Copy the MAC address from Serial Monitor.
- Update the MAC in the Watch module code.

5. Upload `Women-Safety-Device-ESP32-002.ino` with updated MAC address.

6. Update emergency contact numbers in the Shoe module code, replacing placeholders with real international format numbers.

7. Access the web applications by connecting to these WiFi networks:

- Watch module: `WomenSafetyWatch` (password: `safety123`) → [http://192.168.4.1](http://192.168.4.1)
- Shoe module: `WomenSafetyShoe` (password: `safety123`) → [http://192.168.4.1](http://192.168.4.1)

## 🧪 Testing & Usage

- Test button press, shaking emergency activation.
- Verify GPS fix and GSM SMS functionality outdoors.
- Use the web app for monitoring battery, heart rate, gas level and GPS location.
- Triple press button for emergency deactivation.

## 📋 License

This project is licensed under the MIT License.

## 👩‍💻 Author

- [JEYARAM B](https://github.com/jeyaram1023)
- [Call](tel:8309631052)
- [Email](mailto:jeyaram.reddy.ece@gmail.com)

---

Feel free to star ⭐ the repo if you find this project helpful!

