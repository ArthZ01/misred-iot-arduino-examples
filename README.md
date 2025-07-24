# MiSREd IoT Arduino Examples

Contoh penggunaan Arduino ESP32 untuk mengirim data sensor ke platform **MiSREd IoT**, menggunakan tiga jenis protokol:

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
