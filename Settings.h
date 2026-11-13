#pragma once
/**
 * Settings - every tunable value, kept in RAM, saved to flash (Preferences), and exchanged with the
 * web dashboard as JSON. Plain structs on purpose: easy to memcpy to flash and easy to explain.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"

struct JointParams {
  char    name[24];
  float   gearRatio;          // total reduction motor -> joint (planetary gearbox x belt/pulley)
  float   motorStepsPerRev;   // 200 (1.8 deg) or 400 (0.9 deg)
  uint8_t microstep;          // 1,2,4,8,16,32  (must match the jumpers under the driver)
  bool    invert;             // reverse positive direction
  float   minDeg, maxDeg;     // software limits, degrees from home
  float   maxSpeed;           // deg/s  at 100 % speed
  float   maxAccel;           // deg/s^2 at 100 % speed

  // How many motor microsteps make one degree of JOINT rotation.
  float stepsPerDeg() const { return motorStepsPerRev * microstep * gearRatio / 360.0f; }
};

struct NetParams {
  bool     useSta;
  char     staSsid[33];
  char     staPass[65];
  char     mqttHost[64];
  uint16_t mqttPort;
  char     mqttUser[33];
  char     mqttPass[65];
  char     topicTelemetry[48];
  char     topicCommand[48];
  char     topicStatus[48];
};

class Settings {
 public:
  JointParams joint[Cfg::N];
  NetParams   net;
  float       speedScale;     // 0.05 .. 1.0 (global speed multiplier)

  // Revision counters (RAM only). Other classes watch these to know when to re-apply.
  uint32_t cfgRev = 0;        // joint parameters changed
  uint32_t netRev = 0;        // Wi-Fi / MQTT parameters changed

  Settings() { loadDefaults(); }

  void loadDefaults();                              // factory values from Config.h
  bool load();                                      // flash -> RAM (falls back to defaults)
  bool save() const;                                // RAM -> flash

  void toJson(JsonDocument& d) const;               // for GET /api/settings
  bool fromJson(const JsonDocument& d, String& err);// for POST /api/settings (partial updates OK)
  bool validate(String& err) const;                 // range checks

  static bool sameNet(const NetParams& a, const NetParams& b);
};
