#include "MqttLink.h"
#include <ArduinoJson.h>
#include <math.h>

void MqttLink::begin() {
  seenRev_ = st_.netRev;
  mqtt_.setServer(st_.net.mqttHost, st_.net.mqttPort);
  mqtt_.setBufferSize(768);
  mqtt_.setKeepAlive(20);
  mqtt_.setSocketTimeout(2);
  mqtt_.setCallback([this](char* topic, uint8_t* payload, unsigned int len) {
    onMessage((const char*)payload, len);
  });
}

const char* MqttLink::statusText() {
  if (!st_.net.useSta)              return "off (Wi-Fi client disabled)";
  if (!st_.net.staSsid[0])          return "Wi-Fi not configured";
  if (!net_.staConnected())         return "waiting for Wi-Fi";
  if (!st_.net.mqttHost[0])         return "no broker set";
  return mqtt_.connected() ? "connected" : "connecting";
}

void MqttLink::update() {
  if (seenRev_ != st_.netRev) {                    // broker/topics edited -> reconnect with new values
    seenRev_ = st_.netRev;
    if (mqtt_.connected()) mqtt_.disconnect();
    mqtt_.setServer(st_.net.mqttHost, st_.net.mqttPort);
    lastTry_ = 0;
  }
  if (!net_.staConnected() || !st_.net.mqttHost[0]) return;
  if (mqtt_.connected()) {
    mqtt_.loop();
    publishTelemetry();
  } else {
    connectIfDue();
  }
}

void MqttLink::connectIfDue() {
  if (lastTry_ && millis() - lastTry_ < 3000) return;   // blocking connect is short (2 s timeout); steppers are hardware-driven
  lastTry_ = millis();
  String id = "digiarm-esp32-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  const NetParams& n = st_.net;
  bool ok = n.mqttUser[0]
      ? mqtt_.connect(id.c_str(), n.mqttUser, n.mqttPass, n.topicStatus, 0, true, "offline")
      : mqtt_.connect(id.c_str(), n.topicStatus, 0, true, "offline");
  if (ok) {
    mqtt_.subscribe(n.topicCommand);
    mqtt_.publish(n.topicStatus, "online", true);
  }
}

void MqttLink::publishTelemetry() {
  if (millis() - lastTel_ < Cfg::TELEMETRY_MS) return;
  lastTel_ = millis();
  JsonDocument d;
  ctl_.fillState(d);
  d["ip"] = net_.staIp();
  char buf[640];
  size_t n = serializeJson(d, buf, sizeof buf);
  mqtt_.publish(st_.net.topicTelemetry, (const uint8_t*)buf, n, false);
}

// ---- incoming commands: {"cmd":"move","q":[..],"speed":0.5} etc. ----
void MqttLink::onMessage(const char* payload, unsigned len) {
  JsonDocument d;
  if (deserializeJson(d, payload, len)) return;
  const char* cmd = d["cmd"] | "";

  if (!strcmp(cmd, "ping")) {                      // latency probe from the dashboard
    JsonDocument r;
    r["cmd"] = "pong";
    r["t"] = d["t"];
    char b[64];
    size_t n = serializeJson(r, b, sizeof b);
    mqtt_.publish(st_.net.topicTelemetry, (const uint8_t*)b, n, false);
    return;
  }
  if (!strcmp(cmd, "estop")) { ctl_.estop(); return; }   // always honoured, even when remote is locked
  if (!ctl_.remoteControl()) return;                     // "allow remote control" unticked on the phone page

  if      (!strcmp(cmd, "enable")) ctl_.enable(d["on"] | true);
  else if (!strcmp(cmd, "home"))   ctl_.home();
  else if (!strcmp(cmd, "zero"))   ctl_.zero();
  else if (!strcmp(cmd, "learn"))  ctl_.learn(d["on"] | true);
  else if (!strcmp(cmd, "save"))   ctl_.savePose(d["dwell"] | 300);
  else if (!strcmp(cmd, "clear"))  ctl_.clearPoses();
  else if (!strcmp(cmd, "play"))   ctl_.play(d["loop"] | false);
  else if (!strcmp(cmd, "stop"))   ctl_.stop();
  else if (!strcmp(cmd, "jog"))    ctl_.jog(d["j"] | -1, d["d"] | 0.0f);
  else if (!strcmp(cmd, "move")) {
    JsonArrayConst qa = d["q"];
    float q[Cfg::N];
    for (int i = 0; i < Cfg::N; i++)
      q[i] = (i < (int)qa.size() && !qa[i].isNull()) ? qa[i].as<float>() : NAN;
    ctl_.moveJoints(q, d["speed"] | st_.speedScale);
  }
}
