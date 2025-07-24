#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <CustomJWT.h>
#include <time.h>  // NTP time

// ---- SETUP VARIABEL ------
#define DEVICE_ID "YOUR_DEVICE_ID"
#define JWT_SECRET "YOUR_JWT_SECRET"

// WiFi configuration
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Server configuration
const char* server_url = "http://103.82.241.46:9800";

// NTP configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

// Device configuration
char device_secret[] = JWT_SECRET;
String device_id = DEVICE_ID;

// JWT configuration
CustomJWT jwt(device_secret, 512); // 512 bytes for payload

// Sensor pins (ESP32 analog-capable pins)
const int PH_SENSOR   = 36;
const int FLOW_SENSOR = 39;
const int COD_SENSOR  = 34;
const int TEMP_SENSOR = 35;
const int NH3N_SENSOR = 32;
const int NTU_SENSOR  = 33;

unsigned long lastSensorSend = 0;
const unsigned long SENSOR_INTERVAL = 5000; // 5 seconds
int messageCount = 0;

void initTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("⏳ Getting time from NTP...");

  struct tm timeinfo;
  int retry = 0;
  const int max_retries = 10;

  while (!getLocalTime(&timeinfo) && retry < max_retries) {
    Serial.print(".");
    delay(500);
    retry++;
  }

  if (retry < max_retries) {
    Serial.println("\n✅ Time synchronized!");
    Serial.print("🕒 Current time: ");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
  } else {
    Serial.println("\n❌ Failed to sync time with NTP server!");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Setup sensor pins
  pinMode(PH_SENSOR, INPUT);
  pinMode(FLOW_SENSOR, INPUT);
  pinMode(COD_SENSOR, INPUT);
  pinMode(TEMP_SENSOR, INPUT);
  pinMode(NH3N_SENSOR, INPUT);
  pinMode(NTU_SENSOR, INPUT);

  // Connect to WiFi
  Serial.println("🔗 Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
  Serial.print("📡 ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // Sync time via NTP
  initTime();

  // Random seed
  randomSeed(analogRead(0));

  // Initialize JWT memory
  jwt.allocateJWTMemory();
  Serial.println("✅ JWT memory allocated");

  Serial.println("🧪 SIMPLE HTTP Sensor Data Test with CustomJWT");
  lastSensorSend = millis() - SENSOR_INTERVAL;
}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorSend >= SENSOR_INTERVAL && messageCount < 10) {
    lastSensorSend = now;
    messageCount++;
    sendSensorDataHTTP();
  }

  if (messageCount >= 10) {
    Serial.println("🏁 Testing completed - sent 10 messages");
    delay(60000);
    messageCount = 0;
  }

  delay(100);
}

void sendSensorDataHTTP() {
  Serial.println("\n📊 Reading sensors (" + String(messageCount) + "/10)...");

  float phValue = readPHSensor();
  float flowValue = readFlowSensor();
  float codValue = readCODSensor();
  float tempValue = readTempSensor();
  float nh3nValue = readNH3NSensor();
  float ntuValue = readNTUSensor();

  StaticJsonDocument<512> sensorDoc;
  sensorDoc["V0"] = phValue;
  sensorDoc["V1"] = flowValue;
  sensorDoc["V2"] = codValue;
  sensorDoc["V3"] = tempValue;
  sensorDoc["V4"] = nh3nValue;
  sensorDoc["V5"] = ntuValue;
  sensorDoc["timestamp"] = millis();
  sensorDoc["device_id"] = device_id;

  String sensorPayload;
  serializeJson(sensorDoc, sensorPayload);

  Serial.println("📦 Sensor payload: " + sensorPayload);

  String jwt_token = createJWTWithCustomJWT(sensorPayload);

  if (jwt_token.length() > 0) {
    HTTPClient http;
    http.begin(server_url + String("/payload/http"));
    http.setTimeout(10000); // ⏱ Set timeout 10s
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-Id", device_id);
    http.addHeader("Authorization", "Bearer " + jwt_token);

    int httpResponseCode = http.POST("{}");

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("✅ Server Response (" + String(httpResponseCode) + "): " + response);

      if (response.indexOf("Berhasil") >= 0 || response.indexOf("success") >= 0) {
        Serial.println("🎉 SUCCESS: Data saved to database!");
      }
    } else {
      Serial.println("❌ HTTP Error: " + String(httpResponseCode));
      if (httpResponseCode == 401 || httpResponseCode == 403) {
        Serial.println("🔄 JWT might be invalid, trying secret renewal...");
        if (renewDeviceSecret()) {
          Serial.println("🔄 Retrying with new secret...");
          sendSensorDataHTTP();
          return;
        }
      }
    }

    http.end();
  } else {
    Serial.println("❌ Failed to create JWT token");
  }
}

float readPHSensor() {
  return 6.5 + (random(0, 200) / 100.0);
}

float readFlowSensor() {
  return 10.0 + (random(0, 4000) / 100.0);
}

float readCODSensor() {
  return 20.0 + (random(0, 6000) / 100.0);
}

float readTempSensor() {
  return 20.0 + (random(0, 1500) / 100.0);
}

float readNH3NSensor() {
  return 0.1 + (random(0, 490) / 100.0);
}

float readNTUSensor() {
  return 1.0 + (random(0, 1900) / 100.0);
}

String createJWTWithCustomJWT(String data) {
  Serial.println("🔐 Creating JWT token with CustomJWT...");

  time_t now;
  time(&now);

  StaticJsonDocument<256> payloadDoc;
  payloadDoc["encryptedData"] = data;
  payloadDoc["deviceId"] = device_id;
  payloadDoc["iat"] = now;
  payloadDoc["exp"] = now + 3600;

  String payloadStr;
  serializeJson(payloadDoc, payloadStr);
  Serial.println("📦 JWT Payload: " + payloadStr);

  bool success = jwt.encodeJWT((char*)payloadStr.c_str());

  if (success) {
    String token = String(jwt.out);
    Serial.println("✅ JWT Token created successfully");
    Serial.println("🔑 Token length: " + String(token.length()));
    return token;
  } else {
    Serial.println("❌ Failed to create JWT token");
    return "";
  }
}

bool renewDeviceSecret() {
  Serial.println("🔄 Attempting to renew device secret...");

  HTTPClient http;
  http.begin(server_url + String("/device/renew-secret/") + device_id);
  http.addHeader("Content-Type", "application/json");

  String requestBody = "{\"old_secret\":\"" + String(device_secret) + "\"}";

  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode == 200) {
    String response = http.getString();
    Serial.println("📋 Renewal response: " + response);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (!error && doc.containsKey("secret_key")) {
      String newSecret = doc["secret_key"];
      strcpy(device_secret, newSecret.c_str());

      CustomJWT newJwt(device_secret, 512);
      jwt = newJwt;
      jwt.allocateJWTMemory();

      Serial.println("✅ Secret renewed successfully!");
      Serial.println("🔑 New secret: " + String(device_secret));

      http.end();
      return true;
    } else {
      Serial.println("❌ Failed to parse renewal response");
    }
  } else {
    String error = http.getString();
    Serial.println("❌ Secret renewal failed (" + String(httpResponseCode) + "): " + error);
  }

  http.end();
  return false;
}
