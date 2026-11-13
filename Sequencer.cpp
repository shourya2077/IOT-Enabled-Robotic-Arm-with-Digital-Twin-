#include "Sequencer.h"
#include <Preferences.h>

const char* Sequencer::modeName() const {
  return mode_ == Learn ? "learn" : (mode_ == Play ? "play" : "manual");
}

void Sequencer::begin() {
  Preferences prefs;
  prefs.begin("digiarm", false);
  n_ = prefs.getUChar("n", 0);
  if (n_ > Cfg::MAX_WAYPOINTS || prefs.getBytesLength("wps") != sizeof(Waypoint) * n_) n_ = 0;
  if (n_ > 0) prefs.getBytes("wps", wp_, sizeof(Waypoint) * n_);
  prefs.end();
}

void Sequencer::persist() {
  Preferences prefs;
  prefs.begin("digiarm", false);
  prefs.putUChar("n", (uint8_t)n_);
  if (n_ > 0) prefs.putBytes("wps", wp_, sizeof(Waypoint) * n_); else prefs.remove("wps");
  prefs.end();
  ver_++;
}

void Sequencer::setLearn(bool on) {
  if (mode_ == Play) arm_.stopSmooth();
  mode_ = on ? Learn : Manual;
}

bool Sequencer::saveWaypoint(uint16_t dwellMs) {
  if (mode_ != Learn || n_ >= Cfg::MAX_WAYPOINTS) return false;
  for (int i = 0; i < Cfg::N; i++) wp_[n_].q[i] = roundf(arm_.joint(i).targetAngle() * 100.0f) / 100.0f;
  wp_[n_].dwell = dwellMs;
  n_++;
  persist();
  return true;
}

bool Sequencer::removeWaypoint(int i) {
  if (mode_ != Learn || i < 0 || i >= n_) return false;
  for (int k = i; k < n_ - 1; k++) wp_[k] = wp_[k + 1];
  n_--;
  persist();
  return true;
}

bool Sequencer::clearWaypoints() {
  if (mode_ != Learn) return false;
  n_ = 0;
  persist();
  return true;
}

bool Sequencer::gotoWaypoint(int i) {
  if (i < 0 || i >= n_ || !arm_.canMove() || mode_ == Play) return false;
  arm_.moveJoints(wp_[i].q, st_.speedScale);
  return true;
}

bool Sequencer::play(bool loop) {
  if (n_ == 0 || !arm_.canMove()) return false;
  loop_ = loop;
  idx_ = 0;
  mode_ = Play;
  arm_.moveJoints(wp_[0].q, st_.speedScale);
  moving_ = true;
  return true;
}

void Sequencer::stop() {
  if (mode_ != Play) return;
  mode_ = Manual;
  arm_.stopSmooth();
}

// Playback state machine:  MOVING -> (arrived) -> DWELL -> (time up) -> next waypoint -> MOVING ...
void Sequencer::update() {
  if (mode_ != Play) return;
  if (!arm_.canMove()) { mode_ = Manual; return; }       // E-STOP or motors off cancels playback
  if (moving_) {
    if (arm_.atTarget()) { moving_ = false; dwellUntil_ = millis() + wp_[idx_].dwell; }
  } else if ((int32_t)(millis() - dwellUntil_) >= 0) {
    idx_++;
    if (idx_ >= n_) {
      if (loop_) idx_ = 0; else { mode_ = Manual; idx_ = 0; return; }
    }
    arm_.moveJoints(wp_[idx_].q, st_.speedScale);
    moving_ = true;
  }
}
