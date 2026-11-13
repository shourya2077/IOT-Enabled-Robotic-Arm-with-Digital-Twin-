#pragma once
/**
 * Controller - the ONE place that decides whether a command is allowed and what it does.
 * Both the web page (WebApi) and MQTT (MqttLink) call these methods, so the rules (E-STOP lock,
 * "playback running", ...) can never differ between the two front-ends.
 *
 * Every command returns nullptr on success, or a short human-readable error text.
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include "Settings.h"
#include "Arm.h"
#include "Sequencer.h"

class Controller {
 public:
  Controller(Settings& st, Arm& arm, Sequencer& seq) : st_(st), arm_(arm), seq_(seq) {}

  // ---- motion ----
  const char* moveJoint(int j, float deg);
  const char* moveJoints(const float* deg, float scale);   // NAN = skip joint
  const char* jog(int j, float deltaDeg);
  const char* home();
  const char* zero();
  const char* setSpeed(float s);

  // ---- safety ----
  const char* estop();
  const char* enable(bool on);                             // on=true also clears E-STOP

  // ---- teach / play ----
  const char* learn(bool on);
  const char* savePose(uint16_t dwellMs);
  const char* deletePose(int i);
  const char* clearPoses();
  const char* gotoPose(int i);
  const char* play(bool loop);
  const char* stop();

  // ---- settings ----
  const char* applySettings(const JsonDocument& body);     // validate -> apply -> save to flash
  const char* resetSettings();

  // ---- remote (MQTT) lock ----
  void setRemoteControl(bool on) { remote_ = on; }
  bool remoteControl() const { return remote_; }

  // ---- status snapshot shared by web + MQTT ----
  void fillState(JsonDocument& d) const;

  Settings& settings() { return st_; }
  Sequencer& sequencer() { return seq_; }
  Arm& arm() { return arm_; }

 private:
  const char* gate() const;      // nullptr if manual motion is allowed right now
  Settings&  st_;
  Arm&       arm_;
  Sequencer& seq_;
  bool       remote_ = true;
  String     err_;
};
