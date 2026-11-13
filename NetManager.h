#pragma once
/**
 * NetManager - Wi-Fi. Always runs the phone access point (AP). Also joins your router/hotspot (STA)
 * when configured, so the ESP32 can reach the MQTT broker. STA credentials come from Settings.
 */
#include <Arduino.h>
#include <WiFi.h>
#include "Settings.h"

class NetManager {
 public:
  explicit NetManager(Settings& st) : st_(st) {}
  void begin();
  void update();                                   // call every loop

  bool   staConnected() const { return WiFi.status() == WL_CONNECTED; }
  bool   staConfigured() const { return st_.net.useSta && st_.net.staSsid[0]; }
  String staIp() const { return staConnected() ? WiFi.localIP().toString() : String(""); }
  String apIp() const  { return WiFi.softAPIP().toString(); }

 private:
  Settings& st_;
  uint32_t  seenRev_ = 0;
  uint32_t  lastTry_ = 0;
};
