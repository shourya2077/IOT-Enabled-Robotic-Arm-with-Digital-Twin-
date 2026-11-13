#pragma once
/**
 * Joint - ONE stepper-driven joint. Knows its gear ratio / microstepping (from JointParams) and hides
 * the "steps vs degrees" maths. Wraps one FastAccelStepper (hardware pulse generator).
 *
 * Convention: position is counted in microsteps from the power-on pose (or the last "set home").
 * angle(deg) = steps / stepsPerDeg.
 */
#include <Arduino.h>
#include <FastAccelStepper.h>
#include "Settings.h"

class Joint {
 public:
  bool begin(FastAccelStepperEngine& engine, int stepPin, int dirPin, const JointParams& p);
  void applyParams(const JointParams& np);          // call when Settings change (motor must be stopped)
  const JointParams& params() const { return p_; }

  // ---- position ----
  int32_t steps() const       { return stp_->getCurrentPosition(); }
  int32_t targetSteps() const { return target_; }
  float   angle() const       { return steps() / spd_; }
  float   targetAngle() const { return target_ / spd_; }
  float   stepsPerDeg() const { return spd_; }
  float   clampAngle(float deg) const;
  int32_t angleToSteps(float deg) const;

  // ---- planning helpers (scale = global speed 0.05..1) ----
  float speedHz(float scale) const;                 // cruise speed in steps/s
  float accelHz(float scale) const;                 // acceleration in steps/s^2
  float estimateTime(float distSteps, float scale) const;  // seconds for a trapezoid move

  // ---- commands ----
  void moveTo(int32_t targetSteps, float vHz, float aHz);
  void stopSmooth();                                // decelerate to a stop
  void stopNow();                                   // immediate stop, keep position
  void syncTarget() { target_ = steps(); }
  void setZero();                                   // declare current pose = 0 deg

  bool running() const  { return stp_->isRunning(); }
  bool atTarget() const { return steps() == target_ && !running(); }

 private:
  FastAccelStepper* stp_ = nullptr;
  int         dirPin_ = 0;
  JointParams p_;
  float       spd_ = 1.0f;     // cached steps per degree
  int32_t     target_ = 0;
};
