#include "Settings.h"
#include <Preferences.h>

namespace {
constexpr uint32_t MAGIC = 0xD16A0003;   // bump when the stored layout changes -> old data is ignored

struct Blob {                            // exactly what is written to flash
  uint32_t    magic;
  JointParams joint[Cfg::N];
  NetParams   net;
  float       speed;
};

void copyStr(char* dst, size_t n, const char* src) {
  if (!src) { dst[0] = 0; return; }
  strncpy(dst, src, n - 1);
  dst[n - 1] = 0;
}
bool validMicrostep(int m) { return m == 1 || m == 2 || m == 4 || m == 8 || m == 16 || m == 32; }
}  // namespace

void Settings::loadDefaults() {
  for (int i = 0; i < Cfg::N; i++) {
    JointParams& p = joint[i];
    copyStr(p.name, sizeof p.name, Cfg::DEF_NAME[i]);
    p.gearRatio        = Cfg::DEF_GEAR[i];
    p.motorStepsPerRev = Cfg::DEF_STEPS_PER_REV;
    p.microstep        = (uint8_t)Cfg::DEF_MICROSTEP;
    p.invert           = false;
    p.minDeg           = Cfg::DEF_MIN[i];
    p.maxDeg           = Cfg::DEF_MAX[i];
    p.maxSpeed         = Cfg::DEF_VMAX[i];
    p.maxAccel         = Cfg::DEF_AMAX[i];
  }
  memset(&net, 0, sizeof net);
  net.useSta   = true;
  net.mqttPort = Cfg::DEF_MQTT_PORT;
  copyStr(net.topicTelemetry, sizeof net.topicTelemetry, Cfg::DEF_TOPIC_TELEMETRY);
  copyStr(net.topicCommand,   sizeof net.topicCommand,   Cfg::DEF_TOPIC_COMMAND);
  copyStr(net.topicStatus,    sizeof net.topicStatus,    Cfg::DEF_TOPIC_STATUS);
  speedScale = Cfg::DEF_SPEED;
}

bool Settings::load() {
  Preferences prefs;
  prefs.begin("digiarm", false);
  Blob b;
  bool ok = prefs.getBytesLength("cfg") == sizeof(Blob) &&
            prefs.getBytes("cfg", &b, sizeof b) == sizeof(Blob) && b.magic == MAGIC;
  prefs.end();
  if (!ok) { loadDefaults(); return false; }
  Settings tmp = *this;
  memcpy(tmp.joint, b.joint, sizeof tmp.joint);
  tmp.net = b.net;
  tmp.speedScale = b.speed;
  String err;
  if (!tmp.validate(err)) { loadDefaults(); return false; }   // corrupted -> safe defaults
  *this = tmp;
  return true;
}

bool Settings::save() const {
  Blob b;
  memset(&b, 0, sizeof b);
  b.magic = MAGIC;
  memcpy(b.joint, joint, sizeof b.joint);
  b.net = net;
  b.speed = speedScale;
  Preferences prefs;
  prefs.begin("digiarm", false);
  size_t n = prefs.putBytes("cfg", &b, sizeof b);
  prefs.end();
  return n == sizeof b;
}

bool Settings::validate(String& err) const {
  for (int i = 0; i < Cfg::N; i++) {
    const JointParams& p = joint[i];
    String j = "J" + String(i) + ": ";
    if (!(p.gearRatio >= 0.01f && p.gearRatio <= 2000.0f))      { err = j + "gear ratio must be 0.01 - 2000"; return false; }
    if (!(p.motorStepsPerRev >= 100 && p.motorStepsPerRev <= 800)) { err = j + "motor steps/rev must be 100 - 800"; return false; }
    if (!validMicrostep(p.microstep))                           { err = j + "microstep must be 1,2,4,8,16 or 32"; return false; }
    if (!(p.minDeg < p.maxDeg) || p.minDeg < -720 || p.maxDeg > 720) { err = j + "need -720 <= min < max <= 720"; return false; }
    if (!(p.maxSpeed >= 0.5f && p.maxSpeed <= 720.0f))          { err = j + "max speed must be 0.5 - 720 deg/s"; return false; }
    if (!(p.maxAccel >= 1.0f && p.maxAccel <= 20000.0f))        { err = j + "max accel must be 1 - 20000 deg/s2"; return false; }
  }
  if (net.mqttPort == 0)                                        { err = "MQTT port must be 1 - 65535"; return false; }
  if (!net.topicTelemetry[0] || !net.topicCommand[0] || !net.topicStatus[0]) { err = "MQTT topics cannot be empty"; return false; }
  if (!(speedScale >= 0.05f && speedScale <= 1.0f))             { err = "speed must be 5 - 100 %"; return false; }
  return true;
}

bool Settings::sameNet(const NetParams& a, const NetParams& b) {
  return a.useSta == b.useSta && a.mqttPort == b.mqttPort &&
         !strcmp(a.staSsid, b.staSsid) && !strcmp(a.staPass, b.staPass) &&
         !strcmp(a.mqttHost, b.mqttHost) && !strcmp(a.mqttUser, b.mqttUser) && !strcmp(a.mqttPass, b.mqttPass) &&
         !strcmp(a.topicTelemetry, b.topicTelemetry) && !strcmp(a.topicCommand, b.topicCommand) &&
         !strcmp(a.topicStatus, b.topicStatus);
}

void Settings::toJson(JsonDocument& d) const {
  JsonArray js = d["joints"].to<JsonArray>();
  for (int i = 0; i < Cfg::N; i++) {
    const JointParams& p = joint[i];
    JsonObject o = js.add<JsonObject>();
    o["name"] = p.name;  o["gear"] = p.gearRatio;  o["spr"] = p.motorStepsPerRev;  o["ms"] = p.microstep;
    o["inv"] = p.invert; o["min"] = p.minDeg;      o["max"] = p.maxDeg;
    o["vmax"] = p.maxSpeed;  o["amax"] = p.maxAccel;
  }
  d["speed"] = speedScale;
  d["maxHz"] = Cfg::MAX_STEP_HZ;
  JsonObject n = d["net"].to<JsonObject>();
  n["sta"] = net.useSta;   n["ssid"] = net.staSsid;   n["pass"] = net.staPass;
  n["host"] = net.mqttHost; n["port"] = net.mqttPort; n["user"] = net.mqttUser; n["mpass"] = net.mqttPass;
  n["tTel"] = net.topicTelemetry; n["tCmd"] = net.topicCommand; n["tSt"] = net.topicStatus;
}

bool Settings::fromJson(const JsonDocument& d, String& err) {
  JsonArrayConst js = d["joints"];
  for (size_t i = 0; i < js.size() && i < (size_t)Cfg::N; i++) {
    JsonObjectConst o = js[i];
    JointParams& p = joint[i];
    const char* nm = o["name"].as<const char*>();
    if (nm) copyStr(p.name, sizeof p.name, nm);
    if (!o["gear"].isNull()) p.gearRatio        = o["gear"].as<float>();
    if (!o["spr"].isNull())  p.motorStepsPerRev = o["spr"].as<float>();
    if (!o["ms"].isNull())   p.microstep        = (uint8_t)o["ms"].as<int>();
    if (!o["inv"].isNull())  p.invert           = o["inv"].as<bool>();
    if (!o["min"].isNull())  p.minDeg           = o["min"].as<float>();
    if (!o["max"].isNull())  p.maxDeg           = o["max"].as<float>();
    if (!o["vmax"].isNull()) p.maxSpeed         = o["vmax"].as<float>();
    if (!o["amax"].isNull()) p.maxAccel         = o["amax"].as<float>();
  }
  if (!d["speed"].isNull()) speedScale = d["speed"].as<float>();

  JsonObjectConst n = d["net"];
  if (!n.isNull()) {
    if (!n["sta"].isNull())  net.useSta = n["sta"].as<bool>();
    if (!n["port"].isNull()) net.mqttPort = (uint16_t)n["port"].as<int>();
    const char* s;
    if ((s = n["ssid"].as<const char*>()))  copyStr(net.staSsid, sizeof net.staSsid, s);
    if ((s = n["pass"].as<const char*>()))  copyStr(net.staPass, sizeof net.staPass, s);
    if ((s = n["host"].as<const char*>()))  copyStr(net.mqttHost, sizeof net.mqttHost, s);
    if ((s = n["user"].as<const char*>()))  copyStr(net.mqttUser, sizeof net.mqttUser, s);
    if ((s = n["mpass"].as<const char*>())) copyStr(net.mqttPass, sizeof net.mqttPass, s);
    if ((s = n["tTel"].as<const char*>()))  copyStr(net.topicTelemetry, sizeof net.topicTelemetry, s);
    if ((s = n["tCmd"].as<const char*>()))  copyStr(net.topicCommand, sizeof net.topicCommand, s);
    if ((s = n["tSt"].as<const char*>()))   copyStr(net.topicStatus, sizeof net.topicStatus, s);
  }
  return validate(err);
}
