# MiSREd IoT Arduino Examples

[![Visit MiSREd IoT](https://img.shields.io/badge/MiSREd%20IoT-Visit%20Platform-blue?logo=internet-explorer)](https://misred-iot.com)

Contoh penggunaan Arduino ESP32 untuk mengirim data sensor ke platform **[MiSREd IoT](https://misred-iot.com)**, menggunakan tiga jenis protokol:

- 🔗 HTTP
- 🔒 HTTPS
- 📡 MQTT

Repositori ini bertujuan membantu pengguna MiSREd IoT dalam memahami integrasi data dari ESP32 ke platform melalui metode yang aman dan efisien.

---

## 📁 Contoh Program

| Contoh        | Deskripsi                                                                 |
|---------------|---------------------------------------------------------------------------|
| `HTTP_Test`   | Mengirim data sensor menggunakan koneksi HTTP biasa                       |
| `HTTPS_Test`  | Mengirim data sensor secara aman melalui HTTPS dengan sertifikat TLS     |
| `MQTT_Test`   | Mengirim data menggunakan protokol MQTT Asinkron untuk komunikasi realtime|

---

## ⚙️ Persyaratan

- **Board**: ESP32
- **Library Arduino** (instal via Library Manager atau manual):
  - `WiFi.h`
  - `ArduinoJson.h`
  - `AsyncMqttClient.h` atau download di sini *https://github.com/marvinroger/async-mqtt-client/*
  - `NTPClient.h`
  - `CustomJWT.h` 

---

## 🔧 Cara Menggunakan

1. Clone repositori ini:
   ```bash
   git clone https://github.com/username/misred-iot-arduino-examples.git

---

## 🌐 Tentang MiSREd IoT

MiSREd IoT adalah platform monitoring IoT berbasis web yang memungkinkan pengguna memantau data dari berbagai perangkat secara real-time. Platform ini mendukung komunikasi melalui HTTP/HTTPS/MQTT dan menggunakan JWT untuk autentikasi data.

---

## 🧩 Kontribusi & Masukan

Silakan ajukan Issue untuk melaporkan masalah atau meminta fitur tambahan. Kontribusi juga sangat diterima!
