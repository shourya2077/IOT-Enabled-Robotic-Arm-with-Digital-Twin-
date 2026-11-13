#pragma once
/**
 * Sequencer - LEARN / PLAY. Stores waypoints (a pose + a pause), keeps them in flash and plays them
 * back in order, optionally looping. Modes: Manual -> Learn (teach) or Play (replay).
 */
#include <Arduino.h>
#include "Config.h"
#include "Settings.h"
#include "Arm.h"

struct Waypoint {
  float    q[Cfg::N];   // joint angles in degrees
  uint16_t dwell;       // pause after arriving, ms
};

class Sequencer {
 public:
  enum Mode : uint8_t { Manual, Learn, Play };

  Sequencer(Arm& arm, Settings& st) : arm_(arm), st_(st) {}
  void begin();         // load waypoints from flash
  void update();        // call every loop (drives playback)

  Mode        mode() const { return mode_; }
  bool        playing() const { return mode_ == Play; }
  const char* modeName() const;

  void setLearn(bool on);
  bool saveWaypoint(uint16_t dwellMs);              // only in Learn mode
  bool removeWaypoint(int i);
  bool clearWaypoints();
  bool gotoWaypoint(int i);
  bool play(bool loop);
  void stop();

  int            count() const   { return n_; }
  const Waypoint& waypoint(int i) const { return wp_[i]; }
  uint32_t       version() const { return ver_; }   // bumps whenever the list changes
  int            index() const   { return idx_; }
  bool           loop() const    { return loop_; }

 private:
  void persist();

  Arm& arm_;
  Settings& st_;
  Mode     mode_ = Manual;
  Waypoint wp_[Cfg::MAX_WAYPOINTS];
  int      n_ = 0, idx_ = 0;
  bool     loop_ = false, moving_ = false;
  uint32_t ver_ = 0, dwellUntil_ = 0;
};
