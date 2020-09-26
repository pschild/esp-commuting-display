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

void updateDisplay();
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

SSD1306Wire display(0x3c, D5, D4); // Params: address, SDA, SCL

String leavingTime;
String arrivalTime;
unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();

  // display.clear();
  // display.setTextAlignment(TEXT_ALIGN_LEFT);
  // display.setFont(ArialMT_Plain_16);
  // display.drawString(0, 0, "Connecting...");
  // display.display();

  // pinMode(LED_BUILTIN, OUTPUT);
  // TODO: due to following line, the display won't work
  // digitalWrite(LED_BUILTIN, HIGH);

  wifiHandler.connect();
  mqttHandler.setup();
  mqttHandler.setOnConnectedCallback(onMqttConnected);
  mqttHandler.setOnMessageCallback(onMqttMessage);
  pingTimer.start();

  // start OTA update immediately
  updateHandler.startUpdate();
}

void updateDisplay() {
  display.clear();

  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.setFont(ArialMT_Plain_16);

  if (leavingTime != "") {
    display.drawString(0, 0, leavingTime);
  }

  if (arrivalTime != "") {
    display.drawString(0, 24, arrivalTime);
  }

  if (lastUpdate > 0) {
    unsigned long lastUpdatedMin = (millis() - lastUpdate) / 1000 / 60;
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 50, "aktualisiert vor " + String(lastUpdatedMin) + "min");

    if (lastUpdatedMin > 15) {
      // TODO: let red LED blink
    }
  }

  display.display();
}

void loop() {
  mqttHandler.loop();
  updateHandler.loop();

  pingTimer.update();

  updateDisplay();
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

void onCommutingStateUpdate(char* topic, char* payload) {
  if (((std::string) topic).find("/start") != std::string::npos) {
    leavingTime = "AB " + String(payload) + " Uhr";
    arrivalTime = "";
    lastUpdate = millis();
  } else if (((std::string) topic).find("/end") != std::string::npos) {
    leavingTime = "";
    arrivalTime = "";
    lastUpdate = 0;
  }
}

void onCommutingDurationUpdate(char* payload) {
  arrivalTime = "AN " + String(payload) + " Uhr";
  lastUpdate = millis();
}

void onOtaUpdate(char* payload) {
  updateHandler.startUpdate();
}

void onMqttConnected() {
  mqttHandler.subscribe("adesso-commuter-server/commuting/status/+");
  mqttHandler.subscribe("adesso-commuter-server/commuting/duration/eta");
  mqttHandler.subscribe("foo/+/baz");
  mqttHandler.subscribe("otaUpdate/all");
}

void onMqttMessage(char* topic, char* message) {
  if (((std::string) topic).find("foo/") != std::string::npos) {
    onFooBar(message);
  } else if (strcmp(topic, "otaUpdate/all") == 0) {
    onOtaUpdate(message);
  } else if (((std::string) topic).find("adesso-commuter-server/commuting/status/") != std::string::npos) {
    onCommutingStateUpdate(topic, message);
  } else if (strcmp(topic, "adesso-commuter-server/commuting/duration/eta") == 0) {
    onCommutingDurationUpdate(message);
  }
}