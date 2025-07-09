# 🐠 Smart Aquarium Automation System

## 📌 Project Title
**A Real-Time Web-Controlled Smart Aquarium with Multi-Sensor Integration**  
*Enhancing Aquatic Life Through Automated Monitoring, Environmental Control, and Intelligent Scheduling Using ESP32*

## 📖 Overview

This project presents a smart, IoT-based aquarium management system using the ESP32 microcontroller. The system enables real-time monitoring and control of key aquatic parameters such as **temperature**, **pH**, and **water level**, while also automating tasks like **feeding** and **lighting** using relays and a servo motor.

The system features a **mobile-accessible web interface** in access point mode, enabling users to remotely monitor sensor data and manually or automatically control the aquarium environment.

---

## ⚙️ Features

- 🌡️ **Temperature Monitoring** (DS18B20)
- 🧪 **pH Level Monitoring**
- 💧 **Water Level Detection** (capacitive water sensor)
- 🕰️ **Real-Time Clock Integration** (DS3231)
- 🐟 **Feeding Automation** using 360° Servo Motor
- 💡 **Lighting and Equipment Control** via 6-Relay Module
- 🌐 **Web Interface** (Access Point mode) with control panel
- 🗓️ **Time-Based Scheduling** for relays and feeding
- 📺 **OLED Display** showing live temperature and pH
- ⚡ **Power Optimization** for continuous monitoring

---

## 🧰 Hardware Components

| Component                  | Description                             |
|---------------------------|-----------------------------------------|
| ESP32                     | Main microcontroller                    |
| DS18B20                   | Digital temperature sensor              |
| pH Sensor                 | Analog pH level sensor                  |
| Water Level Sensor        | Capacitive depth-level sensor           |
| DS3231 RTC Module         | Real-Time Clock for schedule syncing    |
| 360° Servo Motor          | For fish feeding                        |
| 6-Channel Relay Module    | Controls light, pumps, heater, etc.     |
| OLED Display (I2C, 1.3")  | Displays real-time values               |
| Power Supply              | 5V and 12V for components               |

---

## 🖥️ Web Interface Features

- Toggle switches for each relay
- Set ON/OFF time schedules for relays and servo (feeding)
- Real-time sensor readings (Temp, pH)
- Display of current time (synced with DS3231)
- Manual button to trigger feeding
- Mobile-friendly responsive layout

---

## 🧠 Software and Libraries Used

- **ESPAsyncWebServer** – for handling web interface
- **ESPAsyncTCP** – web server TCP support
- **OneWire** and **DallasTemperature** – for DS18B20
- **Wire** – for I2C devices (OLED, RTC)
- **RTClib** – for DS3231 timekeeping
- **Preferences** – for saving schedules and states
- **Adafruit GFX & SSD1306** – for OLED display
- **Servo.h / ESP32Servo** – for servo motor control

---

## 🔧 Setup Instructions

1. **Connect all hardware** according to the pin configuration.
2. Install the required libraries in Arduino IDE.
3. Upload the code to the ESP32 using USB.
4. Connect to the ESP32 Wi-Fi access point from your mobile.
5. Open the local IP address shown in the Serial Monitor (e.g., `192.168.4.1`).
6. Access the web UI to control and monitor your aquarium.

---

## 📸 Screenshots

*Coming soon – UI images and hardware setup.*

---

## 📈 Future Improvements

- Cloud-based control via Firebase or Blynk
- Mobile app for Android/iOS
- Auto pH balancing mechanism
- Water change notification system
- Voice assistant integration (Alexa/Google)

---

## 📄 License

This project is open-source and available under the [MIT License](LICENSE).

---

## 🤝 Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss your ideas.

---

## 🙌 Acknowledgments

Thanks to the open-source community and contributors who provided libraries, documentation, and inspiration for IoT-based smart systems.

---

