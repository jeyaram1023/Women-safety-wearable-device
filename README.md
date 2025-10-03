# 👩‍💻 Women Safety Device (Watch + Smart Slippers)

![Project Banner](https://via.placeholder.com/800x200.png?text=Women+Safety+Device)

[![Blogger](https://img.shields.io/badge/Blogger-Read_More-orange)](http://jeyaramb.blogspot.com/2025/)  
[![YouTube](https://img.shields.io/badge/YouTube-Watch_Demo-red)](https://www.youtube.com/watch?v=YOUR_VIDEO_LINK)  
[![License](https://img.shields.io/badge/License-Educational-blue)](LICENSE)

---

## 🔎 Project Overview

The **Women Safety Device** is an advanced dual-module IoT system designed to enhance personal safety for women. It consists of:  

1. **Smart Watch Module** – monitors user movement, health parameters, and triggers emergency alerts.  
2. **Smart Slippers Module** – receives alerts, activates deterrent systems, and sends GPS/GSM location to emergency contacts.  

The system uses **ESP32 and ESP8266 microcontrollers** and communicates via **ESP-NOW protocol** for fast, low-power, and reliable signal transfer between modules.  

> ⚠️ The static electric deterrent is **simulated for demonstration purposes**, and no harmful currents are used.

---

## 🧩 Technical Logic & How It Works

### 1. Emergency Activation Logic
Emergency mode can be activated by either:

- **Manual Activation:** Long press the watch button for 3 seconds  
- **Automatic Activation:** Abnormal shaking or sudden movement detected for >5 seconds via the **ADXL335 accelerometer**  

Once activated:  
- Watch LCD shows *“Women Safety Device Activated”*  
- Buzzer sounds for 3 seconds  
- Gas sensor readings and heart rate are recorded  
- Watch sends an emergency packet via **ESP-NOW** to the slippers  

---

### 2. Smart Slippers Module Logic
Upon receiving the emergency packet:  

- **Deterrent Activation:** Non-injurious deterrent simulation (alarm, strobe, vibration) representing a **static electric deterrent** to repel attackers and allow safe escape  
- **GPS Activation:** Acquires current location  
- **GSM Alerts:** Sends SMS with location and emergency details to pre-configured contacts  
- **Battery & Sensor Update:** Reports status back to the web dashboard  

If the watch detects **no movement post-activation**, an additional alert is sent:  
> *“User has fainted and fallen down.”*

---

### 3. Deactivation Logic
- Triple-press the watch button → disables emergency mode  
- Sends a **cancel packet** to slippers → turns off GPS, GSM, and deterrents  
- Web dashboard updates to reflect system deactivation  

---

### 4. Web App Integration
The local web app (HTML5, CSS3, JavaScript) provides:  

- Real-time **battery percentages** of watch and slippers  
- Current **GPS location** of the user  
- Heart rate and gas sensor data from the watch  
- History of all emergency notifications  
- Ability to **add/edit emergency contacts** and manage alerts  

---

## ⚡ Features Summary

| Module          | Features |
|-----------------|---------|
| **Smart Watch** | Emergency activation via button or accelerometer, LCD display, buzzer alert, heart rate & gas sensor readings, ESP-NOW communication |
| **Smart Slippers** | Receive emergency packet, activate static electric deterrent simulation, send GPS/GSM alerts, update web dashboard |
| **Web App**     | View real-time battery %, GPS, health data, emergency log, manage contacts |

---

## 🛠 Tech Stack & Components

- **Microcontrollers:** ESP32 (Watch), ESP8266 (Slippers)  
- **Sensors:** ADXL335 Accelerometer, Heart Rate Sensor, Gas Sensor (MQ-135)  
- **Modules:** GPS (NEO-6M), GSM (SIM800L), LCD (16x2 I2C), Buzzer  
- **Communication:** ESP-NOW, GSM SMS  
- **Frontend:** HTML5, CSS3, JavaScript  
- **Programming:** Arduino IDE (C/C++)  

---

## 🎬 Demo

![Demo GIF](https://via.placeholder.com/600x300.png?text=Demo+GIF)

**Demo Steps:**  
1. Long press watch button or simulate abnormal movement  
2. Watch triggers LCD message & buzzer  
3. Slippers activate deterrents and send GPS/GSM alerts  
4. Web app updates battery %, GPS location, heart rate, and gas levels  
5. Triple-press watch button to deactivate emergency mode  

---

## 🔗 Project Links

- **Blogger Post / Detailed Explanation:** [http://jeyaramb.blogspot.com/2025/](http://jeyaramb.blogspot.com/2025/)  
- **YouTube Demo Video:** [https://www.youtube.com/watch?v=YOUR_VIDEO_LINK](https://www.youtube.com/watch?v=YOUR_VIDEO_LINK)  

---

## 🚀 Future Upgrades

- Mobile app for real-time notifications and remote monitoring  
- Cloud-based monitoring for multiple users simultaneously  
- AI-based fall detection for higher accuracy  
- Improved deterrent simulation with more interactive feedback  

---

## 📜 License

This project is for **educational and personal use only**.  
The static electric deterrent is **simulated** and non-injurious.  
Please follow safety and legal guidelines when replicating this project.
