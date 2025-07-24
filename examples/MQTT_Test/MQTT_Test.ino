#include <WiFi.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <CustomJWT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiEventId_t wifiEventHandler;

// WiFi configuration
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

const char* mqtt_server = "103.82.241.46";
const int mqtt_port = 1883;
const char* mqtt_topic = "device/data";

char device_secret[] = "YOUR_JWT_SECRET";
String device_id = "YOUR_DEVICE_ID";

bool timeReady = false;
bool mqttStarted = false;
bool mqttConnectingLogged = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

CustomJWT jwt(device_secret, 512);

const int PH_SENSOR = 36;
const int FLOW_SENSOR = 39;
const int COD_SENSOR = 34;
const int TEMP_SENSOR = 35;
const int NH3N_SENSOR = 32;
const int NTU_SENSOR = 33;

AsyncMqttClient mqttClient;
unsigned long lastSensorSend = 0;
const unsigned long SENSOR_INTERVAL = 5000;
int messageCount = 0;

void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case IP_EVENT_STA_GOT_IP:
      Serial.println("✅ Got IP address, trying to connect to MQTT...");
      Serial.println("📡 Attempting MQTT connection...");  // Baru connect MQTT setelah dapat IP
      break;

    case WIFI_EVENT_STA_DISCONNECTED:
      Serial.println("❌ WiFi disconnected, retrying...");
      WiFi.begin(ssid, password);
      break;

    default:
      break;
  }
}

void connectToWiFi() {
  Serial.println("🔗 Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected in blocking mode");
    Serial.println("📡 IP: " + WiFi.localIP().toString());
    timeClient.begin(); // langsung mulai NTP
  } else {
    Serial.println("\n❌ Failed to connect to WiFi");
  }
}


void onMqttConnect(bool sessionPresent) {
  Serial.println("✅ Connected to MQTT broker");
  mqttConnectingLogged = false;
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.print("❌ MQTT disconnected: ");
  switch (reason) {
    case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
      Serial.println("TCP Disconnected");
      break;
    case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
      Serial.println("Unacceptable Protocol Version");
      break;
    case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
      Serial.println("Identifier Rejected");
      break;
    case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
      Serial.println("Server Unavailable");
      break;
    case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
      Serial.println("Malformed Credentials");
      break;
    default:
      Serial.println("Unknown Reason");
  }

  if (WiFi.isConnected()) {
    Serial.println("📡 WiFi still connected, will retry MQTT in 5 seconds");
    delay(5000);
    mqttClient.connect();
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PH_SENSOR, INPUT);
  pinMode(FLOW_SENSOR, INPUT);
  pinMode(COD_SENSOR, INPUT);
  pinMode(TEMP_SENSOR, INPUT);
  pinMode(NH3N_SENSOR, INPUT);
  pinMode(NTU_SENSOR, INPUT);

  wifiEventHandler = WiFi.onEvent(onWiFiEvent);

  // ✅ SET MQTT CALLBACKS SEBELUM WIFI CONNECT
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.onPublish([](uint16_t packetId) {
    Serial.println("📨 MQTT message published with packetId: " + String(packetId));
  });

  // WiFi.onEvent(onWifiEvent);
  connectToWiFi();

  jwt.allocateJWTMemory();
  Serial.println("✅ JWT memory allocated");

  lastSensorSend = millis() - SENSOR_INTERVAL;
}

void loop() {
  unsigned long now = millis();

  if (!mqttClient.connected() && WiFi.isConnected() && !mqttConnectingLogged) {
  Serial.println("❗ Waiting for MQTT to connect...");
  mqttConnectingLogged = true;
  }

  if (WiFi.isConnected() && !timeReady) {
    if (timeClient.update()) {
      timeReady = true;
      Serial.println("🕒 NTP synced: " + String(timeClient.getEpochTime()));
    } else {
      timeClient.forceUpdate();
    }
  }

  if (WiFi.isConnected() && !mqttClient.connected() && timeReady && !mqttStarted) {
    mqttStarted = true;
    Serial.println("📡 Connecting MQTT after time is synced...");
    mqttClient.connect();
  }

  if (timeReady && mqttClient.connected()) {
    if (now - lastSensorSend >= SENSOR_INTERVAL && messageCount < 10) {
      lastSensorSend = now;
      messageCount++;
      sendSensorDataMQTT();
    }

    if (messageCount >= 10) {
      Serial.println("🏁 MQTT Testing completed - published 10 messages");
      delay(60000);
      messageCount = 0;
    }
  }
}


void sendSensorDataMQTT() {
  Serial.println("\n📊 Reading sensors for MQTT (" + String(messageCount) + "/10)...");

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
  sensorDoc["timestamp"] = timeClient.getEpochTime();
  sensorDoc["device_id"] = device_id;

  String sensorPayload;
  serializeJson(sensorDoc, sensorPayload);

  Serial.println("📦 Sensor payload: " + sensorPayload);

  String jwt_token = createJWTWithCustomJWT(sensorPayload);

  if (jwt_token.length() > 0) {
    StaticJsonDocument<256> mqttDoc;
    mqttDoc["device_id"] = device_id;
    mqttDoc["jwt"] = jwt_token;
    mqttDoc["timestamp"] = timeClient.getEpochTime();
    mqttDoc["message_count"] = messageCount;

    String mqttPayload;
    serializeJson(mqttDoc, mqttPayload);

    Serial.println("📤 Publishing to MQTT topic: " + String(mqtt_topic));
    Serial.print("📏 Payload length: ");
    Serial.println(mqttPayload.length());

    mqttClient.publish(mqtt_topic, 0, false, mqttPayload.c_str());
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
  Serial.println("🔐 Creating MQTT JWT token with CustomJWT...");

  StaticJsonDocument<256> payloadDoc;
  payloadDoc["encryptedData"] = data;
  payloadDoc["deviceId"] = device_id;
  unsigned long currentTime = timeClient.getEpochTime();
  payloadDoc["iat"] = currentTime;
  payloadDoc["exp"] = currentTime + 3600;

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
