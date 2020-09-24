#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WifiHandler.h>
#include <MqttHandler.h>
#include <OTAUpdateHandler.h>

#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"

#ifndef WIFI_SSID
  #error "Missing WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
  #error "Missing WIFI_PASSWORD"
#endif

#ifndef VERSION
  #error "Missing VERSION"
#endif

const String CHIP_ID = String("ESP_") + String(ESP.getChipId());

void ping();
void onFooBar(char* payload);
void onCommutingStateUpdate(char* payload);
void onCommutingDurationUpdate(char* payload);
void onOtaUpdate(char* payload);
void onMqttConnected();
void onMqttMessage(char* topic, char* message);

WifiHandler wifiHandler(WIFI_SSID, WIFI_PASSWORD);
MqttHandler mqttHandler("192.168.178.28", CHIP_ID);
OTAUpdateHandler updateHandler("192.168.178.28:9042", VERSION);

Ticker pingTimer(ping, 60 * 1000);

SSD1306Wire display(0x3c, D5, D4);

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  wifiHandler.connect();
  mqttHandler.setup();
  mqttHandler.setOnConnectedCallback(onMqttConnected);
  mqttHandler.setOnMessageCallback(onMqttMessage);
  pingTimer.start();

  // start OTA update immediately
  updateHandler.startUpdate();

  display.init();
  display.flipScreenVertically();
}

void drawFontFaceDemo() {
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "AB 15:33 Uhr");
  display.drawString(0, 24, "AN 16:30 Uhr");

  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 50, "aktualisiert vor 10min");
}

void loop() {
  mqttHandler.loop();
  updateHandler.loop();

  pingTimer.update();

  display.clear();
  drawFontFaceDemo();
  display.display();
}

void ping() {
  const String channel = String("devices/") + CHIP_ID + String("/version");
  mqttHandler.publish(channel.c_str(), VERSION);
}

void onFooBar(char* payload) {
  if (strcmp(payload, "on") == 0) {
    digitalWrite(LED_BUILTIN, LOW);
  } else if (strcmp(payload, "off") == 0) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void onCommutingStateUpdate(char* payload) {
}

void onCommutingDurationUpdate(char* payload) {
}

void onOtaUpdate(char* payload) {
  updateHandler.startUpdate();
}

void onMqttConnected() {
  mqttHandler.subscribe("adesso-commuter-server/commuting/status");
  mqttHandler.subscribe("adesso-commuter-server/commuting/duration");
  mqttHandler.subscribe("foo/+/baz");
  mqttHandler.subscribe("otaUpdate/all");
}

void onMqttMessage(char* topic, char* message) {
  if (((std::string) topic).rfind("foo/", 0) == 0) {
    onFooBar(message);
  } else if (strcmp(topic, "otaUpdate/all") == 0) {
    onOtaUpdate(message);
  } else if (strcmp(topic, "adesso-commuter-server/commuting/status") == 0) {
    onCommutingStateUpdate(message);
  } else if (strcmp(topic, "adesso-commuter-server/commuting/duration") == 0) {
    onCommutingDurationUpdate(message);
  }
}