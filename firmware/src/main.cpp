#include <Arduino.h>
#include <espnow.h>
#include <ESP8266WiFi.h>

#define SENDER // Comment this line if you want to use the receiver code

#ifdef SENDER

  uint8_t receiverAddress[] = {0x24, 0x6F, 0x28, 0x89, 0xB4, 0xA8};  // MAC address of receiver
  uint8_t deviceID = 1; // Sender device ID

  void setup() {
    pinMode(0, OUTPUT);
    digitalWrite(0, HIGH);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != 0) {
      delay(1000);
      ESP.restart();
    }

    esp_now_add_peer(receiverAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  }

  void loop() {
    esp_now_send(receiverAddress, &deviceID, sizeof(deviceID));
    digitalWrite(0, LOW);  // Set pin low before sleep
    ESP.deepSleep(0);  // Enter deep sleep indefinitely
  }

#else

  #include <WiFiManager.h>
  #include <PubSubClient.h>

  // MQTT Parameters
  const char* mqttServer = "f9245585683d4a0eb7352d37d63c080a.s1.eu.hivemq.cloud";
  int port = 8883;
  const char* mqttClientId = "duren-gateway";
  const char* mqttTopic = "duren";
  const char* mqttUsername = "";
  const char* mqttPassword = "";

  WiFiManager wm;
  WiFiClientSecure espClient;
  PubSubClient client(espClient);

  // ESPNOW callback function
  void onDataRecv(uint8_t* mac, uint8_t* incomingData, uint8_t len) {
    uint8_t deviceID = incomingData[0];
    char deviceIDStr[4];
    itoa(deviceID, deviceIDStr, 10);  // Convert device ID to string

    if (!client.connected()) reconnect();

    if (client.publish(mqttTopic, deviceIDStr)) {
      Serial.println("MQTT message sent");
    } else {
      Serial.println("Failed to send MQTT message");
    }
  }

  void reconnect() {
    while (!client.connected()) {
      if (client.connect(mqttClientId, mqttUsername, mqttPassword)) {
        Serial.println("Connected to MQTT broker");
      } else {
        delay(1000);
      }
    }
  }

  void setup() {
    Serial.begin(115200);
    wm.autoConnect("Gateway Duren", "");

    espClient.setInsecure();  // Disable SSL certificate verification
    client.setServer(mqttServer, port);

    WiFi.mode(WIFI_STA);

    if (esp_now_init() != 0) {
      Serial.println("ESP-NOW initialization failed");
      ESP.restart();
    }

    esp_now_register_recv_cb(onDataRecv);  // Register callback
  }

  void loop() {
    if (!client.connected()) reconnect();  // Reconnect if MQTT is disconnected
    client.loop();  // Maintain MQTT connection
  }

#endif