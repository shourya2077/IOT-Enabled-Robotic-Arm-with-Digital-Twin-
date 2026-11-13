#include "Controller.h"
#include <math.h>

static inline float r2(float v) { return roundf(v * 100.0f) / 100.0f; }

const char* Controller::gate() const {
  if (arm_.estopped())  return "E-STOP active - press Reset";
  if (!arm_.motorsOn()) return "Motors are off";
  if (seq_.playing())   return "Playback running";
  return nullptr;
}

const char* Controller::moveJoint(int j, float deg) {
  if (const char* e = gate()) return e;
  if (j < 0 || j >= Cfg::N) return "bad joint";
  float q[Cfg::N];
  for (int i = 0; i < Cfg::N; i++) q[i] = NAN;
  q[j] = deg;
  arm_.moveJoints(q, st_.speedScale);
  return nullptr;
}

const char* Controller::moveJoints(const float* deg, float scale) {
  if (const char* e = gate()) return e;
  arm_.moveJoints(deg, scale);
  return nullptr;
}

const char* Controller::jog(int j, float d) {
  if (j < 0 || j >= Cfg::N) return "bad joint";
  return moveJoint(j, arm_.joint(j).targetAngle() + d);
}

const char* Controller::home() {
  if (const char* e = gate()) return e;
  arm_.home(st_.speedScale);
  return nullptr;
}

const char* Controller::zero() {
  if (!arm_.estopped() && !arm_.atTarget()) return "Stop motion first";
  arm_.setZero();
  return nullptr;
}

const char* Controller::setSpeed(float s) {
  st_.speedScale = fminf(fmaxf(s, 0.05f), 1.0f);       // RAM only; saved with Settings
  return nullptr;
}

const char* Controller::estop() {
  seq_.stop();
  arm_.estop();
  return nullptr;
}

const char* Controller::enable(bool on) {
  if (on) arm_.clearEstop();
  else seq_.stop();
  arm_.setMotors(on);
  return nullptr;
}

const char* Controller::learn(bool on) { seq_.setLearn(on); return nullptr; }

const char* Controller::savePose(uint16_t dwellMs) {
  return seq_.saveWaypoint(dwellMs) ? nullptr : "Enter Learn mode (or list full)";
}
const char* Controller::deletePose(int i) { return seq_.removeWaypoint(i) ? nullptr : "Enter Learn mode first"; }
const char* Controller::clearPoses()      { return seq_.clearWaypoints() ? nullptr : "Enter Learn mode first"; }

const char* Controller::gotoPose(int i) {
  if (const char* e = gate()) return e;
  return seq_.gotoWaypoint(i) ? nullptr : "No such waypoint";
}

const char* Controller::play(bool loop) {
  if (seq_.count() == 0) return "No waypoints saved";
  if (arm_.estopped() || !arm_.motorsOn()) return "Reset / enable motors first";
  if (seq_.mode() == Sequencer::Learn) seq_.setLearn(false);
  return seq_.play(loop) ? nullptr : "Cannot start playback";
}

const char* Controller::stop() { seq_.stop(); return nullptr; }

const char* Controller::applySettings(const JsonDocument& body) {
  Settings tmp = st_;                                   // work on a copy; commit only if valid
  if (!tmp.fromJson(body, err_)) return err_.c_str();
  bool netChanged = !Settings::sameNet(tmp.net, st_.net);
  seq_.stop();
  arm_.haltImmediate();
  st_ = tmp;
  st_.cfgRev++;
  if (netChanged) st_.netRev++;
  arm_.applySettings();
  st_.save();
  return nullptr;
}

const char* Controller::resetSettings() {
  seq_.stop();
  arm_.haltImmediate();
  uint32_t c = st_.cfgRev + 1, n = st_.netRev + 1;
  st_.loadDefaults();
  st_.cfgRev = c;
  st_.netRev = n;
  arm_.applySettings();
  st_.save();
  return nullptr;
}

void Controller::fillState(JsonDocument& d) const {
  d["v"] = 1;
  d["t"] = millis();
  JsonArray q = d["q"].to<JsonArray>();
  JsonArray t = d["tgt"].to<JsonArray>();
  for (int i = 0; i < Cfg::N; i++) {
    q.add(r2(arm_.joint(i).angle()));
    t.add(r2(arm_.joint(i).targetAngle()));
  }
  d["state"]  = arm_.stateName();
  d["mode"]   = seq_.modeName();
  d["en"]     = arm_.motorsOn();
  d["spd"]    = r2(st_.speedScale);
  d["wpn"]    = seq_.count();
  d["wpver"]  = seq_.version();
  d["idx"]    = seq_.index();
  d["loop"]   = seq_.loop();
  d["mqctl"]  = remote_;
  d["cfgver"] = st_.cfgRev;
}
