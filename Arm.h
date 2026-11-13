#pragma once
/**
 * Arm - owns the 5 Joint objects and the safety state (enable pin, E-STOP).
 * Its main job: time-synchronised multi-joint moves ("all joints arrive together").
 */
#include <Arduino.h>
#include <FastAccelStepper.h>
#include "Config.h"
#include "Settings.h"
#include "Joint.h"

class Arm {
 public:
  explicit Arm(Settings& st) : st_(st) {}

  bool begin();                       // false if a stepper could not be created
  void update();                      // call every loop: finishes smooth stops, homing flag, E-STOP button
  void applySettings();               // push Settings -> joints (stops motion first)

  // ---- motion ----
  void moveJoints(const float* deg, float scale);   // absolute; NAN = leave that joint alone
  void home(float scale);                           // go to 0 on every joint
  void stopSmooth();
  void haltImmediate();
  void setZero();                                   // current pose becomes 0 deg

  // ---- power / safety ----
  void setMotors(bool on);                          // drivers enabled (hold torque) or released (arm may fall!)
  void estop();                                     // stop now, keep holding torque, block motion
  void clearEstop() { estop_ = false; }
  bool motorsOn() const { return motorsOn_; }
  bool estopped() const { return estop_; }
  bool canMove() const  { return !estop_ && motorsOn_; }

  // ---- status ----
  bool atTarget() const;
  const char* stateName() const;
  Joint&       joint(int i)       { return joints_[i]; }
  const Joint& joint(int i) const { return joints_[i]; }

 private:
  Settings& st_;
  FastAccelStepperEngine engine_ = FastAccelStepperEngine();
  Joint   joints_[Cfg::N];
  bool    motorsOn_ = false, estop_ = false, homing_ = false, stopping_ = false;
  uint8_t btnCount_ = 0;
};
