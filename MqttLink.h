#pragma once
/**
 * MqttLink - MQTT protocol v1 (same as the DigiArm digital-twin dashboard).
 *   publishes  <topicTelemetry>  state JSON every TELEMETRY_MS
 *   subscribes <topicCommand>    JSON commands -> Controller
 *   last-will  <topicStatus>     "online" (retained) / "offline"
 * Broker host/port/credentials/topics come from Settings (edit them on the web dashboard).
 */
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "Settings.h"
#include "Controller.h"
#include "NetManager.h"

class MqttLink {
 public:
  MqttLink(Controller& ctl, Settings& st, NetManager& net)
      : ctl_(ctl), st_(st), net_(net), mqtt_(wifi_) {}
  void begin();
  void update();                                   // call every loop
  const char* statusText();

 private:
  void onMessage(const char* payload, unsigned len);
  void connectIfDue();
  void publishTelemetry();

  Controller& ctl_;
  Settings&   st_;
  NetManager& net_;
  WiFiClient   wifi_;
  PubSubClient mqtt_;
  uint32_t seenRev_ = 0, lastTry_ = 0, lastTel_ = 0;
};
